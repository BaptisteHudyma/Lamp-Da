/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
#include <string.h>

#include "../../../../src/system/utils/print.h"

#include "usb_pd.h"
#include "task.h"
#include "tcpm/usb_pd_tcpm.h"
#include "tcpm/tcpm.h"
#include "drivers/usb_pd_driver.h"
#include <assert.h>

/* Flags to clear on a disconnect */
#define PD_FLAGS_RESET_ON_DISCONNECT_MASK                                                                       \
  (PD_FLAGS_PARTNER_DR_POWER | PD_FLAGS_PARTNER_DR_DATA | PD_FLAGS_CHECK_IDENTITY | PD_FLAGS_SNK_CAP_RECVD |    \
   PD_FLAGS_TCPC_DRP_TOGGLE | PD_FLAGS_EXPLICIT_CONTRACT | PD_FLAGS_PREVIOUS_PD_CONN | PD_FLAGS_CHECK_PR_ROLE | \
   PD_FLAGS_CHECK_DR_ROLE | PD_FLAGS_PARTNER_UNCONSTR | PD_FLAGS_VCONN_ON | PD_FLAGS_TRY_SRC |                  \
   PD_FLAGS_PARTNER_USB_COMM | PD_FLAGS_UPDATE_SRC_CAPS | PD_FLAGS_TS_DTS_PARTNER | PD_FLAGS_SNK_WAITING_BATT | \
   PD_FLAGS_CHECK_VCONN_STATE)

#ifdef CONFIG_COMMON_RUNTIME
#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ##args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ##args)

BUILD_ASSERT(1 <= EC_USB_PD_MAX_PORTS);

/*
 * Debug log level - higher number == more log
 *   Level 0: Log state transitions
 *   Level 1: Level 0, plus state name
 *   Level 2: Level 1, plus packet info
 *   Level 3: Level 2, plus ping packet and packet dump on error
 *
 * Note that higher log level causes timing changes and thus may affect
 * performance.
 *
 * Can be limited to constant debug_level by CONFIG_USB_PD_DEBUG_LEVEL
 */
#ifdef CONFIG_USB_PD_DEBUG_LEVEL
static const int debug_level = CONFIG_USB_PD_DEBUG_LEVEL;
#else
static int debug_level;
#endif

/*
 * PD communication enabled flag. When false, PD state machine still
 * detects source/sink connection and disconnection, and will still
 * provide VBUS, but never sends any PD communication.
 */
static uint8_t pd_comm_enabled;
#else /* CONFIG_COMMON_RUNTIME */

#if PD_DEBUG_LEVEL > 0
#define CPRINTF(format, args...) lampda_print(format, ##args)
#define CPRINTS(format, args...) lampda_print(format, ##args)
#else
#define CPRINTF(format, args...)
#define CPRINTS(format, args...)
#endif
static const int debug_level = PD_DEBUG_LEVEL;

#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE
#define DUAL_ROLE_IF_ELSE(sink_clause, src_clause) (pd.power_role == PD_ROLE_SINK ? (sink_clause) : (src_clause))
#else
#define DUAL_ROLE_IF_ELSE(sink_clause, src_clause) (src_clause)
#endif

#define READY_RETURN_STATE() DUAL_ROLE_IF_ELSE(PD_STATE_SNK_READY, PD_STATE_SRC_READY)

/* Type C supply voltage (mV) */
#define TYPE_C_VOLTAGE 5000 /* mV */

/* PD counter definitions */
#define PD_MESSAGE_ID_COUNT 7
#define PD_HARD_RESET_COUNT 2
#define PD_CAPS_COUNT       50
#define PD_SNK_CAP_RETRIES  3

/*
 * The time that we allow the port partner to send any messages after an
 * explicit contract is established.  200ms was chosen somewhat arbitrarily as
 * it should be long enough for sources to decide to send a message if they were
 * going to, but not so long that a "low power charger connected" notification
 * would be shown in the chrome OS UI.
 */
#define SNK_READY_HOLD_OFF_US (200 * MSEC_US)
/*
 * For the same purpose as SNK_READY_HOLD_OFF_US, but this delay can be longer
 * since the concern over "low power charger" is not relevant when connected as
 * a source and the additional delay avoids a race condition where the partner
 * port sends a power role swap request close to when the VDM discover identity
 * message gets sent.
 */
#define SRC_READY_HOLD_OFF_US (400 * MSEC_US)

#ifdef CONFIG_USB_PD_DUAL_ROLE
/* Port dual-role state */
enum pd_dual_role_states drp_state = CONFIG_USB_PD_INITIAL_DRP_STATE;

/* Enable variable for Try.SRC states */
static uint8_t pd_try_src_enable;
#endif

#ifdef CONFIG_USB_PD_REV30
/*
 * The spec. revision is used to index into this array.
 *  Rev 0 (PD 1.0) - return PD_CTRL_REJECT
 *  Rev 1 (PD 2.0) - return PD_CTRL_REJECT
 *  Rev 2 (PD 3.0) - return PD_CTRL_NOT_SUPPORTED
 */
static const uint8_t refuse[] = {PD_CTRL_REJECT, PD_CTRL_REJECT, PD_CTRL_NOT_SUPPORTED};
#define REFUSE(r) refuse[r]
#else
#define REFUSE(r) PD_CTRL_REJECT
#endif

#ifdef CONFIG_USB_PD_REV30
/*
 * The spec. revision is used to index into this array.
 *  Rev 0 (VDO 1.0) - return VDM_VER10
 *  Rev 1 (VDO 1.0) - return VDM_VER10
 *  Rev 2 (VDO 2.0) - return VDM_VER20
 */
static const uint8_t vdo_ver[] = {VDM_VER10, VDM_VER10, VDM_VER20};
#define VDO_VER(v) vdo_ver[v]
#else
#define VDO_VER(v) VDM_VER10
#endif

// variables that used to be pd_task, but had to be promoted
// so both pd_init and pd_run_state_machine can see them
static int head;
static uint32_t payload[7];
static int timeout = 10 * MSEC_US;
static enum tcpc_cc_voltage_status cc1, cc2;
static int res, incoming_packet = 0;
static int hard_reset_count = 0;
#ifdef CONFIG_USB_PD_DUAL_ROLE
static uint64_t next_role_swap = PD_T_DRP_SNK;
static uint8_t saved_flgs = 0;
#ifndef CONFIG_USB_PD_VBUS_DETECT_NONE
static int snk_hard_reset_vbus_off = 0;
#endif

#if defined(CONFIG_CHARGE_MANAGER)
static typec_current_t typec_curr = 0, typec_curr_change = 0;
#endif /* CONFIG_CHARGE_MANAGER */
#endif /* CONFIG_USB_PD_DUAL_ROLE */
static enum pd_states this_state;
static enum pd_cc_states new_cc_state;
static timestamp_t now;
uint64_t next_src_cap = 0;
static int caps_count = 0, hard_reset_sent = 0;
static int snk_cap_count = 0;

enum vdm_states
{
  VDM_STATE_ERR_BUSY = -3,
  VDM_STATE_ERR_SEND = -2,
  VDM_STATE_ERR_TMOUT = -1,
  VDM_STATE_DONE = 0,
  /* Anything >0 represents an active state */
  VDM_STATE_READY = 1,
  VDM_STATE_BUSY = 2,
  VDM_STATE_WAIT_RSP_BUSY = 3,
};

enum ams_seq
{
  AMS_START,
  AMS_RESPONSE,
};

struct pd_protocol
{
  /* current port power role (SOURCE or SINK) */
  uint8_t power_role;
  /* current port data role (DFP or UFP) */
  uint8_t data_role;
  /* 3-bit rolling message ID counter */
  uint8_t msg_id;
  /* Port polarity : 0 => CC1 is CC line, 1 => CC2 is CC line */
  uint8_t polarity;
  /* PD state for port */
  enum pd_states task_state;
  /* PD state when we run state handler the last time */
  enum pd_states last_state;
  /* bool: request state change to SUSPENDED */
  uint8_t req_suspend_state;
  /* The state to go to after timeout */
  enum pd_states timeout_state;
  /* port flags, see PD_FLAGS_* */
  uint32_t flags;
  /* Timeout for the current state. Set to 0 for no timeout. */
  uint64_t timeout;
  /* Time for source recovery after hard reset */
  uint64_t src_recover;
  /* Time for CC debounce end */
  uint64_t cc_debounce;
  /* The cc state */
  enum pd_cc_states cc_state;
  /* status of last transmit */
  uint8_t tx_status;

  /* Last received */
  uint8_t last_msg_id;

  /* last requested voltage PDO index */
  int requested_idx;
#ifdef CONFIG_USB_PD_DUAL_ROLE
  /* Current limit / voltage based on the last request message */
  uint32_t curr_limit;
  uint32_t supply_voltage;
  /* Signal charging update that affects the port */
  int new_power_request;
  /* Store previously requested voltage request */
  int prev_request_mv;
  /* Time for Try.SRC states */
  uint64_t try_src_marker;
  uint64_t try_timeout;
#endif

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
  /* Time to enter low power mode */
  uint64_t low_power_time;
  /* Time to debounce exit low power mode */
  uint64_t low_power_exit_time;
  /* Tasks preventing TCPC from entering low power mode */
  int tasks_preventing_lpm;
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
  /*
   * Timer for handling TOGGLE_OFF/FORCE_SINK mode when auto-toggle
   * enabled. See drp_auto_toggle_next_state() for details.
   */
  uint64_t drp_sink_time;
#endif

  /*
   * Time to ignore Vbus absence due to external IC debounce detection
   * logic immediately after a power role swap.
   */
  uint64_t vbus_debounce_time;

  /* PD state for Vendor Defined Messages */
  enum vdm_states vdm_state;
  /* Timeout for the current vdm state.  Set to 0 for no timeout. */
  timestamp_t vdm_timeout;
  /* next Vendor Defined Message to send */
  uint32_t vdo_data[VDO_MAX_SIZE];
  /* type of transmit message (SOP/SOP'/SOP'') */
  enum tcpm_transmit_type xmit_type;
  uint8_t vdo_count;
  /* VDO to retry if UFP responder replied busy. */
  uint32_t vdo_retry;

#ifdef CONFIG_USB_PD_CHROMEOS
  /* Attached ChromeOS device id, RW hash, and current RO / RW image */
  uint16_t dev_id;
  uint32_t dev_rw_hash[PD_RW_HASH_SIZE / 4];
  enum ec_image current_image;
#endif

#ifdef CONFIG_USB_PD_REV30
  /* protocol revision */
  uint8_t rev;
#endif
  /*
   * Some port partners are really chatty after an explicit contract is
   * established.  Therefore, we allow this time for the port partner to
   * send any messages in order to avoid a collision of sending messages
   * of our own.
   */
  uint64_t ready_state_holdoff_timer;
  /*
   * PD 2.0 spec, section 6.5.11.1
   * When we can give up on a HARD_RESET transmission.
   */
  uint64_t hard_reset_complete_timer;
} pd;

#ifdef CONFIG_COMMON_RUNTIME
static const char* const pd_state_names[] = {
        "DISABLED",
        "SUSPENDED",
#ifdef CONFIG_USB_PD_DUAL_ROLE
        "SNK_DISCONNECTED",
        "SNK_DISCONNECTED_DEBOUNCE",
        "SNK_HARD_RESET_RECOVER",
        "SNK_DISCOVERY",
        "SNK_REQUESTED",
        "SNK_TRANSITION",
        "SNK_READY",
        "SNK_SWAP_INIT",
        "SNK_SWAP_SNK_DISABLE",
        "SNK_SWAP_SRC_DISABLE",
        "SNK_SWAP_STANDBY",
        "SNK_SWAP_COMPLETE",
#endif /* CONFIG_USB_PD_DUAL_ROLE */
        "SRC_DISCONNECTED",
        "SRC_DISCONNECTED_DEBOUNCE",
        "SRC_HARD_RESET_RECOVER",
        "SRC_STARTUP",
        "SRC_DISCOVERY",
        "SRC_NEGOCIATE",
        "SRC_ACCEPTED",
        "SRC_POWERED",
        "SRC_TRANSITION",
        "SRC_READY",
        "SRC_GET_SNK_CAP",
        "DR_SWAP",
#ifdef CONFIG_USB_PD_DUAL_ROLE
        "SRC_SWAP_INIT",
        "SRC_SWAP_SNK_DISABLE",
        "SRC_SWAP_SRC_DISABLE",
        "SRC_SWAP_STANDBY",
#ifdef CONFIG_USBC_VCONN_SWAP
        "VCONN_SWAP_SEND",
        "VCONN_SWAP_INIT",
        "VCONN_SWAP_READY",
#endif /* CONFIG_USBC_VCONN_SWAP */
#endif /* CONFIG_USB_PD_DUAL_ROLE */
        "SOFT_RESET",
        "HARD_RESET_SEND",
        "HARD_RESET_EXECUTE",
        "BIST_RX",
        "BIST_TX",
#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
        "DRP_AUTO_TOGGLE",
#endif
};
BUILD_ASSERT(ARRAY_SIZE(pd_state_names) == PD_STATE_COUNT);
#endif

/*
 * 4 entry rw_hash table of type-C devices that AP has firmware updates for.
 */
#ifdef CONFIG_COMMON_RUNTIME
#define RW_HASH_ENTRIES 4
static struct ec_params_usb_pd_rw_hash_entry rw_hash_table[RW_HASH_ENTRIES];
#endif

int pd_comm_is_enabled()
{
#ifdef CONFIG_COMMON_RUNTIME
  return pd_comm_enabled;
#else
  return 1;
#endif
}

static inline void set_state_timeout(uint64_t timeout, enum pd_states timeout_state)
{
  pd.timeout = timeout;
  pd.timeout_state = timeout_state;
}

#ifdef CONFIG_USB_PD_REV30
int pd_get_rev() { return pd.rev; }

int pd_get_vdo_ver() { return vdo_ver[pd.rev]; }
#endif

/* Return flag for pd state is connected */
int pd_is_connected()
{
  if (pd.task_state == PD_STATE_DISABLED)
    return 0;

#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
  if (pd.task_state == PD_STATE_DRP_AUTO_TOGGLE)
    return 0;
#endif

  return DUAL_ROLE_IF_ELSE(/* sink */
                           pd.task_state != PD_STATE_SNK_DISCONNECTED &&
                                   pd.task_state != PD_STATE_SNK_DISCONNECTED_DEBOUNCE,
                           /* source */
                           pd.task_state != PD_STATE_SRC_DISCONNECTED &&
                                   pd.task_state != PD_STATE_SRC_DISCONNECTED_DEBOUNCE);
}

/**
 * Invalidate last message received at the port when the port gets disconnected
 * or reset(soft/hard). This is used to identify and handle the duplicate
 * messages.
 */
static void invalidate_last_message_id() { pd.last_msg_id = INVALID_MSG_ID_COUNTER; }

/*
 * Return true if partner port is a DTS or TS capable of entering debug
 * mode (eg. is presenting Rp/Rp or Rd/Rd).
 */
int pd_ts_dts_plugged() { return pd.flags & PD_FLAGS_TS_DTS_PARTNER; }

#ifdef CONFIG_USB_PD_DUAL_ROLE
void pd_vbus_low() { pd.flags &= ~PD_FLAGS_VBUS_NEVER_LOW; }

int pd_is_vbus_present()
{
#ifdef CONFIG_USB_PD_VBUS_DETECT_TCPC
  return tcpm_get_vbus_level(VBUS_PRESENT);
#else
  return pd_snk_is_vbus_provided();
#endif
}
#endif

uint8_t savedPortFlags;
int pd_get_saved_port_flags(uint8_t* flags)
{
  flags = &savedPortFlags;
#if 0
	if (system_get_bbram(get_bbram_idx(port), flags) != EC_SUCCESS) {
#ifndef CHIP_HOST
		ccprintf("PD NVRAM FAIL");
#endif
		return EC_ERROR_UNKNOWN;
	}
#endif
  return EC_SUCCESS;
}

void pd_update_saved_port_flags(uint8_t flag, uint8_t do_set)
{
  uint8_t saved_flags;

  if (pd_get_saved_port_flags(&saved_flags) != EC_SUCCESS)
    return;

  if (do_set)
    saved_flags |= flag;
  else
    saved_flags &= ~flag;

#if 0
  pd_set_saved_port_flags(saved_flags);
#endif
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
/* Last received source cap */
static uint32_t pd_src_caps[PDO_MAX_OBJECTS];
static uint8_t pd_src_cap_cnt;

const uint32_t* const pd_get_src_caps() { return pd_src_caps; }

void pd_set_src_caps(int cnt, uint32_t* src_caps)
{
  int i;

  pd_src_cap_cnt = cnt;

  for (i = 0; i < cnt; i++)
    pd_src_caps[i] = *src_caps++;
}

uint8_t pd_get_src_cap_cnt() { return pd_src_cap_cnt; }
#endif /* CONFIG_USB_PD_DUAL_ROLE */

static inline void set_state(enum pd_states next_state)
{
  enum pd_states last_state = pd.task_state;
#if defined(CONFIG_LOW_POWER_IDLE) && !defined(CONFIG_USB_PD_TCPC_ON_CHIP)
  int i;
#endif
  int not_auto_toggling = 1;

  set_state_timeout(0, 0);
  pd.task_state = next_state;

  if (last_state == next_state)
    return;

#if defined(CONFIG_USBC_PPC) && defined(CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE)
  /* If we're entering DRP_AUTO_TOGGLE, there is no sink connected. */
  if (next_state == PD_STATE_DRP_AUTO_TOGGLE)
  {
    ppc_dev_is_connected(PPC_DEV_DISCONNECTED);
    /* Disable Auto Discharge Disconnect */
    tcpm_enable_auto_discharge_disconnect(0);

    if (IS_ENABLED(CONFIG_USBC_OCP))
    {
      usbc_ocp_snk_is_connected(false);
      /*
       * Clear the overcurrent event counter
       * since we've detected a disconnect.
       */
      usbc_ocp_clear_event_counter();
    }
  }
#endif /* CONFIG_USBC_PPC &&  CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE */

#ifdef CONFIG_USB_PD_DUAL_ROLE
#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
  if (last_state != PD_STATE_DRP_AUTO_TOGGLE)
    /* Clear flag to allow DRP auto toggle when possible */
    pd.flags &= ~PD_FLAGS_TCPC_DRP_TOGGLE;
  else
    /* This is an auto toggle instead of disconnect */
    not_auto_toggling = 0;
#endif

  /* Ignore dual-role toggling between sink and source */
  if ((last_state == PD_STATE_SNK_DISCONNECTED && next_state == PD_STATE_SRC_DISCONNECTED) ||
      (last_state == PD_STATE_SRC_DISCONNECTED && next_state == PD_STATE_SNK_DISCONNECTED))
    return;

  if (next_state == PD_STATE_SRC_DISCONNECTED || next_state == PD_STATE_SNK_DISCONNECTED)
  {
#ifdef CONFIG_USBC_PPC
    enum tcpc_cc_voltage_status cc1, cc2;

    tcpm_get_cc(&cc1, &cc2);
    /*
     * Neither a debug accessory nor UFP attached.
     * Tell the PPC module that there is no device connected.
     */
    if (!cc_is_at_least_one_rd(cc1, cc2))
    {
      ppc_dev_is_connected(PPC_DEV_DISCONNECTED);

      if (IS_ENABLED(CONFIG_USBC_OCP))
      {
        usbc_ocp_snk_is_connected(false);
        /*
         * Clear the overcurrent event counter
         * since we've detected a disconnect.
         */
        usbc_ocp_clear_event_counter();
      }
    }
#endif /* CONFIG_USBC_PPC */

    /* Clear the holdoff timer since the port is disconnected. */
    pd.ready_state_holdoff_timer = 0;

    /*
     * We should not clear any flags when transitioning back to the
     * disconnected state from the debounce state as the two states
     * here are really the same states in the state diagram.
     */
    if (last_state != PD_STATE_SNK_DISCONNECTED_DEBOUNCE && last_state != PD_STATE_SRC_DISCONNECTED_DEBOUNCE)
    {
      pd.flags &= ~PD_FLAGS_RESET_ON_DISCONNECT_MASK;
      reset_pd_cable();
    }

    /* Clear the input current limit */
    pd_set_input_current_limit(0, 0);
#ifdef CONFIG_CHARGE_MANAGER
    typec_set_input_current_limit(0, 0);
    // TODO charge_manager_set_ceil( CEIL_REQUESTOR_PD, CHARGE_CEIL_NONE);
#endif
#ifdef CONFIG_BC12_DETECT_DATA_ROLE_TRIGGER
    /*
     * When data role set events are used to enable BC1.2, then CC
     * detach events are used to notify BC1.2 that it can be powered
     * down.
     */
    task_set_event(USB_CHG_EVENT_CC_OPEN);
#endif /* CONFIG_BC12_DETECT_DATA_ROLE_TRIGGER */
#ifdef CONFIG_USBC_VCONN
    set_vconn(0);
#endif /* defined(CONFIG_USBC_VCONN) */
    pd_update_saved_port_flags(PD_BBRMFLG_EXPLICIT_CONTRACT, 0);
#else /* CONFIG_USB_PD_DUAL_ROLE */
  if (next_state == PD_STATE_SRC_DISCONNECTED)
  {
#ifdef CONFIG_USBC_VCONN
    set_vconn(0);
#endif /* CONFIG_USBC_VCONN */
#endif /* !CONFIG_USB_PD_DUAL_ROLE */
    /* If we are source, make sure VBUS is off and restore RP */
    if (pd.power_role == PD_ROLE_SOURCE)
    {
      /* Restore non-active ports to CONFIG_USB_PD_PULLUP */
      pd_power_supply_reset();
      tcpm_set_cc(TYPEC_CC_RP);
    }
#ifdef CONFIG_USB_PD_REV30
    /* Adjust rev to highest level*/
    pd.rev = PD_REV30;
#endif
#ifdef CONFIG_USB_PD_CHROMEOS
    pd.dev_id = 0;
#endif
#ifdef CONFIG_CHARGE_MANAGER
    // TODO charge_manager_update_dualrole( CAP_UNKNOWN);
#endif
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
    if (pd_dfp_exit_mode(TCPC_TX_SOP, 0, 0))
      usb_mux_set_safe_mode();
#endif
    /*
     * Indicate that the port is disconnected by setting role to
     * DFP as SoCs have special signals when they are the UFP ports
     * (e.g. OTG signals)
     */
    pd_execute_data_swap(PD_ROLE_DFP);
#ifdef CONFIG_USBC_SS_MUX
    usb_mux_set(USB_PD_MUX_NONE, USB_SWITCH_DISCONNECT, pd.polarity);
#endif
    /* Disable TCPC RX */
    tcpm_set_rx_enable(0);

    /* Invalidate message IDs. */
    invalidate_last_message_id();

    /* Disable Auto Discharge Disconnect */
    if (not_auto_toggling)
    {
      tcpm_enable_auto_discharge_disconnect(0);
    }

    /* detect USB PD cc disconnect */
#ifdef CONFIG_COMMON_RUNTIME
    hook_notify(HOOK_USB_PD_DISCONNECT);
    calls->pe_handle_detach();
#endif
  }

#ifdef CONFIG_USB_PD_REV30
  /* Upon entering SRC_READY, it is safe for the sink to transmit */
  if (next_state == PD_STATE_SRC_READY)
  {
    if (pd.rev == PD_REV30 && pd.flags & PD_FLAGS_EXPLICIT_CONTRACT)
      sink_can_xmit(SINK_TX_OK);
  }
#endif

#if defined(CONFIG_LOW_POWER_IDLE) && !defined(CONFIG_USB_PD_TCPC_ON_CHIP)
  /* If a PD device is attached then disable deep sleep */
  for (i = 0; i < board_get_usb_pd_port_count(); i++)
  {
    if (pd_capable(i))
      break;
  }
  if (i == board_get_usb_pd_port_count())
    enable_sleep(SLEEP_MASK_USB_PD);
  else
    disable_sleep(SLEEP_MASK_USB_PD);
#endif

#ifdef CONFIG_USB_PD_TCPMV1_DEBUG
  if (debug_level > 0)
    CPRINTF("C%d st%d %s\n", next_state, pd_state_names[next_state]);
  else
#endif
    CPRINTF("C%d st%d\n", next_state);
}

/* increment message ID counter */
static void inc_id() { pd.msg_id = (pd.msg_id + 1) & PD_MESSAGE_ID_COUNT; }

#ifdef CONFIG_USB_PD_REV30
static void sink_can_xmit(int rp)
{
  tcpm_select_rp_value(rp);
  tcpm_set_cc(TYPEC_CC_RP);
}

static inline void pd_ca_reset() { pd.ca_buffered = 0; }
#endif

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER

/* 10 ms is enough time for any TCPC transaction to complete. */
#define PD_LPM_DEBOUNCE_US (10 * MSEC_US)
/* 25 ms on LPM exit to ensure TCPC is settled */
#define PD_LPM_EXIT_DEBOUNCE_US (25 * MSEC_US)

/* This is only called from the PD tasks that owns the port. */
static void handle_device_access()
{
  /* This should only be called from the PD task */
  pd.low_power_time = get_time().val + PD_LPM_DEBOUNCE_US;
  if (pd.flags & PD_FLAGS_LPM_ENGAGED)
  {
    // tcpc_prints("Exit Low Power Mode", port);
    pd.flags &= ~(PD_FLAGS_LPM_ENGAGED | PD_FLAGS_LPM_REQUESTED);
    pd.flags |= PD_FLAGS_LPM_EXIT;

    pd.low_power_exit_time = get_time().val + PD_LPM_EXIT_DEBOUNCE_US;
    /*
     * Wake to ensure we make another pass through the main task
     * loop after clearing the flags.
     */
    task_wake();
  }
}

static int pd_device_in_low_power()
{
  /*
   * If we are actively waking the device up in the PD task, do not
   * let TCPC operation wait or retry because we are in low power mode.
   */
  if (pd.flags & PD_FLAGS_LPM_TRANSITION)
    return 0;

  return pd.flags & PD_FLAGS_LPM_ENGAGED;
}

/* find the most significant bit. Not defined in n == 0. */
#define __fls(n) (31 - __builtin_clz(n))

static int reset_device_and_notify()
{
  int rv;
  int task, waiting_tasks;

  /* This should only be called from the PD task */
  pd.flags |= PD_FLAGS_LPM_TRANSITION;
  rv = tcpm_init();
  pd.flags &= ~PD_FLAGS_LPM_TRANSITION;

  /*
   * Before getting the other tasks that are waiting, clear the reset
   * event from this PD task to prevent multiple reset/init events
   * occurring.
   *
   * The double reset event happens when the higher priority PD interrupt
   * task gets an interrupt during the above tcpm_init function. When that
   * occurs, the higher priority task waits correctly for us to finish
   * waking the TCPC, but it has also set PD_EVENT_TCPC_RESET again, which
   * would result in a second, unnecessary init.
   */
  task_clear_event_bitmap(PD_EVENT_TCPC_RESET);

  /*
   * Now that we are done waking up the device, handle device access
   * manually because we ignored it while waking up device.
   */
  handle_device_access();

  /* Clear SW LPM state; the state machine will set it again if needed */
  pd.flags &= ~PD_FLAGS_LPM_REQUESTED;

  /* Wake up all waiting tasks. */
  while (waiting_tasks)
  {
    task = __fls(waiting_tasks);
    waiting_tasks &= ~(1 << task);
    task_set_event(task, TASK_EVENT_PD_AWAKE);
  }

  return rv;
}

static void pd_wait_for_wakeup()
{
  /* If we are in the PD task, we can directly reset */
  reset_device_and_notify();
}

void pd_wait_exit_low_power()
{
  if (pd_device_in_low_power())
    pd_wait_for_wakeup();
}

/*
 * This can be called from any task. If we are in the PD task, we can handle
 * immediately. Otherwise, we need to notify the PD task via event.
 */
void pd_device_accessed()
{
  /* Ignore any access to device while it is waking up */
  if (pd.flags & PD_FLAGS_LPM_TRANSITION)
    return;

  handle_device_access();
}

/* This is only called from the PD tasks that owns the port. */
static void exit_low_power_mode()
{
  if (pd.flags & PD_FLAGS_LPM_ENGAGED)
    reset_device_and_notify();
  else
    pd.flags &= ~PD_FLAGS_LPM_REQUESTED;
}

#else /* !CONFIG_USB_PD_TCPC_LOW_POWER */

/* We don't need to notify anyone if low power mode isn't involved. */
static int reset_device_and_notify()
{
  const int rv = tcpm_init();
  return rv;
}

#endif /* CONFIG_USB_PD_TCPC_LOW_POWER */

void pd_transmit_complete(int status)
{
  if (status == TCPC_TX_COMPLETE_SUCCESS)
    inc_id();

  pd.tx_status = status;
  task_set_event(PD_EVENT_TX);
}

/* Return true if partner port is known to be PD capable. */
int pd_capable()
{
  return
          // TODO: weird double negation
          !!(pd.flags & PD_FLAGS_PREVIOUS_PD_CONN);
}

static int pd_transmit(enum tcpm_transmit_type type, uint16_t header, const uint32_t* data, enum ams_seq ams)
{
  int evt;
  int res;
#ifdef CONFIG_USB_PD_REV30
  int sink_ng = 0;
#endif

  /* If comms are disabled, do not transmit, return error */
  if (!pd_comm_is_enabled())
    return -1;

  /* Don't try to transmit anything until we have processed
   * all RX messages.
   */
  if (tcpm_has_pending_message())
    return -1; // not an error

#ifdef CONFIG_USB_PD_REV30
  /* Source-coordinated collision avoidance */
  /*
   * USB PD Rev 3.0, Version 2.0: Section 2.7.3.2
   * Collision Avoidance - Protocol Layer
   *
   * In order to avoid message collisions due to asynchronous Messaging
   * sent from the Sink, the Source sets Rp to SinkTxOk (3A) to indicate
   * to the Sink that it is ok to initiate an AMS. When the Source wishes
   * to initiate an AMS, it sets Rp to SinkTxNG (1.5A).
   * When the Sink detects that Rp is set to SinkTxOk, it May initiate an
   * AMS. When the Sink detects that Rp is set to SinkTxNG it Shall Not
   * initiate an AMS and Shall only send Messages that are part of an AMS
   * the Source has initiated.
   * Note that this restriction applies to SOP* AMS’s i.e. for both Port
   * to Port and Port to Cable Plug communications.
   *
   * This starts after an Explicit Contract is in place (see section 2.5.2
   * SOP* Collision Avoidance).
   *
   * Note: a Sink can still send Hard Reset signaling at any time.
   */
  if ((pd.rev == PD_REV30) && ams == AMS_START && (pd.flags & PD_FLAGS_EXPLICIT_CONTRACT))
  {
    if (pd.power_role == PD_ROLE_SOURCE)
    {
      /*
       * Inform Sink that it can't transmit. If a sink
       * transmission is in progress and a collision occurs,
       * a reset is generated. This should be rare because
       * all extended messages are chunked. This effectively
       * defaults to PD REV 2.0 collision avoidance.
       */
      sink_can_xmit(SINK_TX_NG);
      sink_ng = 1;
    }
    else if (type != TCPC_TX_HARD_RESET)
    {
      enum tcpc_cc_voltage_status cc1, cc2;

      tcpm_get_cc(&cc1, &cc2);
      if (cc1 == TYPEC_CC_VOLT_RP_1_5 || cc2 == TYPEC_CC_VOLT_RP_1_5)
      {
        /* Sink can't transmit now. */
        /* Return failure, pd_task can retry later */
        return -1;
      }
    }
  }
#endif
  tcpm_transmit(type, header, data);

  /* Wait until TX is complete */
  evt = task_wait_event_mask(PD_EVENT_TX, PD_T_TCPC_TX_TIMEOUT);

  if (evt & TASK_EVENT_TIMER)
    return -1;

  /* TODO: give different error condition for failed vs discarded */
  res = pd.tx_status == TCPC_TX_COMPLETE_SUCCESS ? 1 : -1;

#ifdef CONFIG_USB_PD_REV30
  /* If the AMS transaction failed to start, reset CC to OK */
  if (res < 0 && sink_ng)
    sink_can_xmit(SINK_TX_OK);
#endif
  return res;
}

#ifdef CONFIG_USB_PD_REV30
static void pd_ca_send_pending()
{
  int cc1;
  int cc2;

  /* Check if a message has been buffered. */
  if (!pd.ca_buffered)
    return;

  tcpm_get_cc(&cc1, &cc2);
  if ((cc1 != TYPEC_CC_VOLT_RP_1_5) && (cc2 != TYPEC_CC_VOLT_RP_1_5))
    if (pd_transmit(pd.ca_type, pd.ca_header, pd.ca_buffer) < 0)
      return;

  /* Message was sent, so free up the buffer. */
  pd.ca_buffered = 0;
}
#endif

static void pd_update_roles()
{
  /* Notify TCPC of role update */
  tcpm_set_msg_header(pd.power_role, pd.data_role);
}

int is_sourcing() { return pd.task_state == PD_STATE_SRC_READY; }

int send_control(int type)
{
  int bit_len;
  uint16_t header = PD_HEADER(type, pd.power_role, pd.data_role, pd.msg_id, 0, pd_get_rev(TCPC_TX_SOP), 0);

  /*
   * For PD 3.0, collision avoidance logic needs to know if this message
   * will begin a new Atomic Message Sequence (AMS)
   */
  enum ams_seq ams = ((1 << type) & PD_CTRL_AMS_START_MASK) ? AMS_START : AMS_RESPONSE;

  bit_len = pd_transmit(TCPC_TX_SOP, header, NULL, ams);
  if (debug_level >= 2)
    CPRINTF("CTRL[%d]>%d\n", type, bit_len);

  return bit_len;
}

static int send_source_cap(enum ams_seq ams)
{
  int bit_len;
#if defined(CONFIG_USB_PD_DYNAMIC_SRC_CAP) || defined(CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT)
  const uint32_t* src_pdo;
  const int src_pdo_cnt = charge_manager_get_source_pdo(&src_pdo);
#else
  const uint32_t* src_pdo = pd_src_pdo;
  const int src_pdo_cnt = pd_src_pdo_cnt;
#endif
  uint16_t header;

  if (src_pdo_cnt == 0)
    /* No source capabilities defined, sink only */
    header = PD_HEADER(PD_CTRL_REJECT, pd.power_role, pd.data_role, pd.msg_id, 0, pd_get_rev(TCPC_TX_SOP), 0);
  else
    header = PD_HEADER(
            PD_DATA_SOURCE_CAP, pd.power_role, pd.data_role, pd.msg_id, src_pdo_cnt, pd_get_rev(TCPC_TX_SOP), 0);

  bit_len = pd_transmit(TCPC_TX_SOP, header, src_pdo, ams);
  if (debug_level >= 2)
    CPRINTF("C srcCAP>%d\n", bit_len);

  return bit_len;
}

#ifdef CONFIG_USB_PD_REV30
static int send_battery_cap(uint32_t* payload)
{
  int bit_len;
  uint16_t msg[6] = {0, 0, 0, 0, 0, 0};
  uint16_t header = PD_HEADER(PD_EXT_BATTERY_CAP,
                              pd.power_role,
                              pd.data_role,
                              pd.msg_id,
                              3, /* Number of Data Objects */
                              pd.rev,
                              1 /* This is an exteded message */
  );

  /* Set extended header */
  msg[0] = PD_EXT_HEADER(0, /* Chunk Number */
                         0, /* Request Chunk */
                         9  /* Data Size in bytes */
  );
  /* Set VID */
  msg[1] = USB_VID_GOOGLE;

  /* Set PID */
  msg[2] = CONFIG_USB_PID;

  if (battery_is_present())
  {
    /*
     * We only have one fixed battery,
     * so make sure batt cap ref is 0.
     */
    if (BATT_CAP_REF(payload[0]) != 0)
    {
      /* Invalid battery reference */
      msg[5] = 1;
    }
    else
    {
      uint32_t v;
      uint32_t c;

      /*
       * The Battery Design Capacity field shall return the
       * Battery’s design capacity in tenths of Wh. If the
       * Battery is Hot Swappable and is not present, the
       * Battery Design Capacity field shall be set to 0. If
       * the Battery is unable to report its Design Capacity,
       * it shall return 0xFFFF
       */
      msg[3] = 0xffff;

      /*
       * The Battery Last Full Charge Capacity field shall
       * return the Battery’s last full charge capacity in
       * tenths of Wh. If the Battery is Hot Swappable and
       * is not present, the Battery Last Full Charge Capacity
       * field shall be set to 0. If the Battery is unable to
       * report its Design Capacity, the Battery Last Full
       * Charge Capacity field shall be set to 0xFFFF.
       */
      msg[4] = 0xffff;

      if (battery_design_voltage(&v) == 0)
      {
        if (battery_design_capacity(&c) == 0)
        {
          /*
           * Wh = (c * v) / 1000000
           * 10th of a Wh = Wh * 10
           */
          msg[3] = DIV_ROUND_NEAREST((c * v), 100000);
        }

        if (battery_full_charge_capacity(&c) == 0)
        {
          /*
           * Wh = (c * v) / 1000000
           * 10th of a Wh = Wh * 10
           */
          msg[4] = DIV_ROUND_NEAREST((c * v), 100000);
        }
      }
    }
  }

  bit_len = pd_transmit(TCPC_TX_SOP, header, (uint32_t*)msg);
  if (debug_level >= 2)
    CPRINTF("batCap>%d\n", bit_len);
  return bit_len;
}

static int send_battery_status(uint32_t* payload)
{
  int bit_len;
  uint32_t msg = 0;
  uint16_t header = PD_HEADER(PD_DATA_BATTERY_STATUS,
                              pd.power_role,
                              pd.data_role,
                              pd.msg_id,
                              1, /* Number of Data Objects */
                              pd.rev,
                              0 /* This is NOT an extended message */
  );

  if (battery_is_present())
  {
    /*
     * We only have one fixed battery,
     * so make sure batt cap ref is 0.
     */
    if (BATT_CAP_REF(payload[0]) != 0)
    {
      /* Invalid battery reference */
      msg |= BSDO_INVALID;
    }
    else
    {
      uint32_t v;
      uint32_t c;

      if (battery_design_voltage(&v) != 0 || battery_remaining_capacity(&c) != 0)
      {
        msg |= BSDO_CAP(BSDO_CAP_UNKNOWN);
      }
      else
      {
        /*
         * Wh = (c * v) / 1000000
         * 10th of a Wh = Wh * 10
         */
        msg |= BSDO_CAP(DIV_ROUND_NEAREST((c * v), 100000));
      }

      /* Battery is present */
      msg |= BSDO_PRESENT;

      /*
       * For drivers that are not smart battery compliant,
       * battery_status() returns EC_ERROR_UNIMPLEMENTED and
       * the battery is assumed to be idle.
       */
      if (battery_status(&c) != 0)
      {
        msg |= BSDO_IDLE; /* assume idle */
      }
      else
      {
        if (c & STATUS_FULLY_CHARGED)
          /* Fully charged */
          msg |= BSDO_IDLE;
        else if (c & STATUS_DISCHARGING)
          /* Discharging */
          msg |= BSDO_DISCHARGING;
        /* else battery is charging.*/
      }
    }
  }
  else
  {
    msg = BSDO_CAP(BSDO_CAP_UNKNOWN);
  }

  bit_len = pd_transmit(TCPC_TX_SOP, header, &msg);
  if (debug_level >= 2)
    CPRINTF("batStat>%d\n", bit_len);

  return bit_len;
}
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE
static void send_sink_cap()
{
  int bit_len;
  uint16_t header = PD_HEADER(
          PD_DATA_SINK_CAP, pd.power_role, pd.data_role, pd.msg_id, pd_snk_pdo_cnt, pd_get_rev(TCPC_TX_SOP), 0);

  bit_len = pd_transmit(TCPC_TX_SOP, header, pd_snk_pdo, AMS_RESPONSE);
  if (debug_level >= 2)
    CPRINTF("snkCAP>%d\n", bit_len);
}

static int send_request(uint32_t rdo)
{
  int bit_len;
  uint16_t header = PD_HEADER(PD_DATA_REQUEST, pd.power_role, pd.data_role, pd.msg_id, 1, pd_get_rev(TCPC_TX_SOP), 0);

  bit_len = pd_transmit(TCPC_TX_SOP, header, &rdo, AMS_RESPONSE);
  if (debug_level >= 2)
    CPRINTF("REQ%d>\n", bit_len);

  return bit_len;
}
#ifdef CONFIG_BBRAM
static int pd_get_saved_active()
{
  uint8_t val;

  if (system_get_bbram(SYSTEM_BBRAM_IDX_PD0, &val))
  {
    CPRINTS("PD NVRAM FAIL");
    return 0;
  }
  return !!val;
}

static void pd_set_saved_active(int val)
{
  if (system_set_bbram(SYSTEM_BBRAM_IDX_PD0, val))
    CPRINTS("PD NVRAM FAIL");
}
#endif // CONFIG_BBRAM
#endif /* CONFIG_USB_PD_DUAL_ROLE */

#ifdef CONFIG_COMMON_RUNTIME
static int send_bist_cmd()
{
  /* currently only support sending bist carrier 2 */
  uint32_t bdo = BDO(BDO_MODE_CARRIER2, 0);
  int bit_len;
  uint16_t header = PD_HEADER(PD_DATA_BIST, pd.power_role, pd.data_role, pd.msg_id, 1, pd_get_rev(TCPC_TX_SOP), 0);

  bit_len = pd_transmit(TCPC_TX_SOP, header, &bdo);
  CPRINTF("BIST>%d\n", bit_len);

  return bit_len;
}
#endif

static void queue_vdm(uint32_t* header, const uint32_t* data, int data_cnt)
{
  pd.vdo_count = data_cnt + 1;
  pd.vdo_data[0] = header[0];
  memcpy(&pd.vdo_data[1], data, sizeof(uint32_t) * data_cnt);
  /* Set ready, pd task will actually send */
  pd.vdm_state = VDM_STATE_READY;
}

static void handle_vdm_request(int cnt, uint32_t* payload)
{
  int rlen = 0;
  uint32_t* rdata;

  if (pd.vdm_state == VDM_STATE_BUSY)
  {
    /* If UFP responded busy retry after timeout */
    if (PD_VDO_CMDT(payload[0]) == CMDT_RSP_BUSY)
    {
      pd.vdm_timeout.val = get_time().val + PD_T_VDM_BUSY;
      pd.vdm_state = VDM_STATE_WAIT_RSP_BUSY;
      pd.vdo_retry = (payload[0] & ~VDO_CMDT_MASK) | CMDT_INIT;
      return;
    }
    else
    {
      pd.vdm_state = VDM_STATE_DONE;
    }
  }

  if (PD_VDO_SVDM(payload[0]))
    rlen = pd_svdm(cnt, payload, &rdata);
  else
    rlen = pd_custom_vdm(cnt, payload, &rdata);

  if (rlen > 0)
  {
    queue_vdm(rdata, &rdata[1], rlen - 1);
    return;
  }
  if (debug_level >= 2)
    CPRINTF("Unhandled VDM VID %04x CMD %04x\n", PD_VDO_VID(payload[0]), payload[0] & 0xFFFF);
}

static void pd_set_data_role(enum pd_data_role role)
{
  pd.data_role = role;
#ifdef CONFIG_USB_PD_DUAL_ROLE
  pd_update_saved_port_flags(PD_BBRMFLG_DATA_ROLE, role);
#endif /* defined(CONFIG_USB_PD_DUAL_ROLE) */
  pd_execute_data_swap(role);

  // TODO set_usb_mux_with_current_data_role();
  pd_update_roles();
#ifdef CONFIG_BC12_DETECT_DATA_ROLE_TRIGGER
  /*
   * For BC1.2 detection that is triggered on data role change events
   * instead of VBUS changes, need to set an event to wake up the USB_CHG
   * task and indicate the current data role.
   */
  if (role == PD_ROLE_UFP)
    task_set_event(USB_CHG_EVENT_DR_UFP);
  else if (role == PD_ROLE_DFP)
    task_set_event(USB_CHG_EVENT_DR_DFP);
#endif /* CONFIG_BC12_DETECT_DATA_ROLE_TRIGGER */
}

void pd_execute_hard_reset()
{
  int hard_rst_tx = pd.last_state == PD_STATE_HARD_RESET_SEND;

  CPRINTF("HARD RST %cX %d\n", hard_rst_tx ? 'T' : 'R', pd.last_state);

  pd.msg_id = 0;
  invalidate_last_message_id();
  tcpm_set_rx_enable(0);
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
  if (pd_dfp_exit_mode(TCPC_TX_SOP, 0, 0))
    usb_mux_set_safe_mode();
#endif

#ifdef CONFIG_USB_PD_REV30
  pd.rev = PD_REV30;
#endif
  /*
   * Fake set last state to hard reset to make sure that the next
   * state to run knows that we just did a hard reset.
   */
  pd.last_state = PD_STATE_HARD_RESET_EXECUTE;

#ifdef CONFIG_USB_PD_DUAL_ROLE
  /*
   * If we are swapping to a source and have changed to Rp, restore back
   * to Rd and turn off vbus to match our power_role.
   */
  if (pd.task_state == PD_STATE_SNK_SWAP_STANDBY || pd.task_state == PD_STATE_SNK_SWAP_COMPLETE)
  {
    tcpm_set_cc(TYPEC_CC_RD);
    pd_power_supply_reset();
  }

  if (pd.power_role == PD_ROLE_SINK)
  {
    /* Initial data role for sink is UFP */
    pd_set_data_role(PD_ROLE_UFP);

    /* Clear the input current limit */
    pd_set_input_current_limit(0, 0);
#ifdef CONFIG_CHARGE_MANAGER
    // TODO charge_manager_set_ceil( CEIL_REQUESTOR_PD, CHARGE_CEIL_NONE);
#endif /* CONFIG_CHARGE_MANAGER */

#ifdef CONFIG_USBC_VCONN
    /*
     * Sink must turn off Vconn after a hard reset if it was being
     * sourced previously
     */
    if (pd.flags & PD_FLAGS_VCONN_ON)
    {
      set_vconn(0);
      pd_set_vconn_role(PD_ROLE_VCONN_OFF);
    }
#endif

    set_state(PD_STATE_SNK_HARD_RESET_RECOVER);
    return;
  }
  else
  {
    /* Initial data role for source is DFP */
    pd_set_data_role(PD_ROLE_DFP);
  }

#endif /* CONFIG_USB_PD_DUAL_ROLE */

  if (!hard_rst_tx)
    usleep(PD_T_PS_HARD_RESET);

  /* We are a source, cut power */
  pd_power_supply_reset();
  pd.src_recover = get_time().val + PD_T_SRC_RECOVER;
#ifdef CONFIG_USBC_VCONN
  set_vconn(0);
#endif
  set_state(PD_STATE_SRC_HARD_RESET_RECOVER);
}

static void execute_soft_reset()
{
  invalidate_last_message_id();
  set_state(DUAL_ROLE_IF_ELSE(PD_STATE_SNK_DISCOVERY, PD_STATE_SRC_DISCOVERY));
  CPRINTF("C Soft Rst\n");
}

void pd_soft_reset(void)
{
  int i;

  if (pd_is_connected())
  {
    set_state(PD_STATE_SOFT_RESET);
    task_wake();
  }
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
/*
 * Request desired charge voltage from source.
 * Returns EC_SUCCESS on success or non-zero on failure.
 */
static int pd_send_request_msg(int always_send_request)
{
  uint32_t rdo, curr_limit, supply_voltage;
  int res;

#ifdef CONFIG_CHARGE_MANAGER
  // int charging = (charge_manager_get_active_charge_port() == port);
  const int charging = 1;
#else
  const int charging = 1;
#endif

#ifdef CONFIG_USB_PD_CHECK_MAX_REQUEST_ALLOWED
  int max_request_allowed = pd_is_max_request_allowed();
#else
  const int max_request_allowed = 1;
#endif

  /* Clear new power request */
  pd.new_power_request = 0;

  /* Build and send request RDO */
  /*
   * If this port is not actively charging or we are not allowed to
   * request the max voltage, then select vSafe5V
   */
  res = pd_build_request(&rdo, &curr_limit, &supply_voltage, PD_REQUEST_MAX);
  /*charging && max_request_allowed ?
   PD_REQUEST_MAX : PD_REQUEST_VSAFE5V);*/

  if (res != EC_SUCCESS)
    /*
     * If fail to choose voltage, do nothing, let source re-send
     * source cap
     */
    return -1;

  if (!always_send_request)
  {
    /* Don't re-request the same voltage */
    if (pd.prev_request_mv == supply_voltage)
    {
      return EC_SUCCESS;
    }
#ifdef CONFIG_CHARGE_MANAGER
    /* Limit current to PD_MIN_MA during transition */
    // else
    //	charge_manager_force_ceil(PD_MIN_MA);
#endif
  }

  CPRINTF("Req C [%d] %dmV %dmA", RDO_POS(rdo), supply_voltage, curr_limit);
  if (rdo & RDO_CAP_MISMATCH)
    CPRINTF(" Mismatch");
  CPRINTF("\n");

  pd.curr_limit = curr_limit;
  pd.supply_voltage = supply_voltage;
  pd.prev_request_mv = supply_voltage;
  res = send_request(rdo);
  if (res < 0)
    return res;
  set_state(PD_STATE_SNK_REQUESTED);
  return EC_SUCCESS;
}
#endif

static void pd_update_pdo_flags(uint32_t pdo)
{
#ifdef CONFIG_CHARGE_MANAGER
  const int charge_whitelisted = 0;
#endif

  /* can only parse PDO flags if type is fixed */
  if ((pdo & PDO_TYPE_MASK) != PDO_TYPE_FIXED)
    return;

#ifdef CONFIG_USB_PD_DUAL_ROLE
  if (pdo & PDO_FIXED_DUAL_ROLE)
    pd.flags |= PD_FLAGS_PARTNER_DR_POWER;
  else
    pd.flags &= ~PD_FLAGS_PARTNER_DR_POWER;

  if (pdo & PDO_FIXED_EXTERNAL)
    pd.flags |= PD_FLAGS_PARTNER_UNCONSTR;
  else
    pd.flags &= ~PD_FLAGS_PARTNER_UNCONSTR;

  if (pdo & PDO_FIXED_COMM_CAP)
    pd.flags |= PD_FLAGS_PARTNER_USB_COMM;
  else
    pd.flags &= ~PD_FLAGS_PARTNER_USB_COMM;
#endif

  if (pdo & PDO_FIXED_DATA_SWAP)
    pd.flags |= PD_FLAGS_PARTNER_DR_DATA;
  else
  {
    pd.flags &= ~PD_FLAGS_PARTNER_DR_DATA;
  }

#ifdef CONFIG_CHARGE_MANAGER
  /*
   * Treat device as a dedicated charger (meaning we should charge
   * from it) if it does not support power swap, or if it is externally
   * powered, or if we are a sink and the device identity matches a
   * charging white-list.
   */
  /*
  if (!(pd.flags & PD_FLAGS_PARTNER_DR_POWER) ||
      (pd.flags & PD_FLAGS_PARTNER_UNCONSTR) ||
      charge_whitelisted)
      charge_manager_update_dualrole(CAP_DEDICATED);
  else
      charge_manager_update_dualrole(CAP_DUALROLE);
  */
#endif
}

static void handle_data_request(uint16_t head, uint32_t* payload)
{
  int type = PD_HEADER_TYPE(head);
  int cnt = PD_HEADER_CNT(head);

  switch (type)
  {
#ifdef CONFIG_USB_PD_DUAL_ROLE
    case PD_DATA_SOURCE_CAP:
      if ((pd.task_state == PD_STATE_SNK_DISCOVERY) || (pd.task_state == PD_STATE_SNK_TRANSITION) ||
          (pd.task_state == PD_STATE_SNK_REQUESTED)
#ifdef CONFIG_USB_PD_VBUS_DETECT_NONE
          || (pd.task_state == PD_STATE_SNK_HARD_RESET_RECOVER)
#endif
          || (pd.task_state == PD_STATE_SNK_READY))
      {
#ifdef CONFIG_USB_PD_REV30
        /*
         * Only adjust sink rev if source rev is higher.
         */
        if (PD_HEADER_REV(head) < pd.rev)
          pd.rev = PD_HEADER_REV(head);
#endif
        /* Port partner is now known to be PD capable */
        pd.flags |= PD_FLAGS_PREVIOUS_PD_CONN;

        /* src cap 0 should be fixed PDO */
        pd_update_pdo_flags(payload[0]);

        pd_process_source_cap(cnt, payload);

        /* Source will resend source cap on failure */
        pd_send_request_msg(1);

        // We call the callback after we send the request
        // because the timing on Request seems to be sensitive
        // User code can take the time until PS_RDY to do stuff
        pd_process_source_cap_callback(cnt, payload);
      }
      break;
#endif /* CONFIG_USB_PD_DUAL_ROLE */
    case PD_DATA_REQUEST:
      if ((pd.power_role == PD_ROLE_SOURCE) && (cnt == 1))
      {
#ifdef CONFIG_USB_PD_REV30
        /*
         * Adjust the rev level to what the sink supports. If
         * they're equal, no harm done.
         */
        pd.rev = PD_HEADER_REV(head);
#endif
        if (!pd_check_requested_voltage(payload[0]))
        {
          if (send_control(PD_CTRL_ACCEPT) < 0)
            /*
             * if we fail to send accept, do
             * nothing and let sink timeout and
             * send hard reset
             */
            return;

          /* explicit contract is now in place */
          pd.flags |= PD_FLAGS_EXPLICIT_CONTRACT;
#ifdef CONFIG_USB_PD_DUAL_ROLE
          pd_update_saved_port_flags(PD_BBRMFLG_EXPLICIT_CONTRACT, 1);
#endif /* CONFIG_USB_PD_DUAL_ROLE */
          pd.requested_idx = RDO_POS(payload[0]);
          set_state(PD_STATE_SRC_ACCEPTED);
          return;
        }
      }
      /* the message was incorrect or cannot be satisfied */
      send_control(PD_CTRL_REJECT);
      /* keep last contract in place (whether implicit or explicit) */
      set_state(PD_STATE_SRC_READY);
      break;
    case PD_DATA_BIST:
      /* If not in READY state, then don't start BIST */
      if (DUAL_ROLE_IF_ELSE(pd.task_state == PD_STATE_SNK_READY, pd.task_state == PD_STATE_SRC_READY))
      {
        /* currently only support sending bist carrier mode 2 */
        if ((payload[0] >> 28) == 5)
        {
          /* bist data object mode is 2 */
          pd_transmit(TCPC_TX_BIST_MODE_2, 0, NULL, AMS_RESPONSE);
          /* Set to appropriate port disconnected state */
          set_state(DUAL_ROLE_IF_ELSE(PD_STATE_SNK_DISCONNECTED, PD_STATE_SRC_DISCONNECTED));
        }
      }
      break;
    case PD_DATA_SINK_CAP:
      pd.flags |= PD_FLAGS_SNK_CAP_RECVD;
      /* snk cap 0 should be fixed PDO */
      pd_update_pdo_flags(payload[0]);
      if (pd.task_state == PD_STATE_SRC_GET_SINK_CAP)
        set_state(PD_STATE_SRC_READY);
      break;
#ifdef CONFIG_USB_PD_REV30
    case PD_DATA_BATTERY_STATUS:
      break;
      /* TODO : Add case PD_DATA_RESET for exiting USB4 */

      /*
       * TODO : Add case PD_DATA_ENTER_USB to accept or reject
       * Enter_USB request from port partner.
       */
#endif
    case PD_DATA_VENDOR_DEF:
      handle_vdm_request(cnt, payload);
      break;
    default:
      CPRINTF("C Unhandled data message type %d\n", type);
  }
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
void pd_request_power_swap()
{
  if (pd.task_state == PD_STATE_SRC_READY)
    set_state(PD_STATE_SRC_SWAP_INIT);
  else if (pd.task_state == PD_STATE_SNK_READY)
    set_state(PD_STATE_SNK_SWAP_INIT);
}

#ifdef CONFIG_USBC_VCONN_SWAP
static void pd_request_vconn_swap()
{
  if (pd.task_state == PD_STATE_SRC_READY || pd.task_state == PD_STATE_SNK_READY)
    set_state(PD_STATE_VCONN_SWAP_SEND);

  task_wake();
}

void pd_try_vconn_src()
{
  /*
   * If we don't currently provide vconn, and we can supply it, send
   * a vconn swap request.
   */
  if (!(pd.flags & PD_FLAGS_VCONN_ON))
  {
    if (pd_check_vconn_swap())
      pd_request_vconn_swap();
  }
}
#endif
#endif /* CONFIG_USB_PD_DUAL_ROLE */

int pd_is_disconnected()
{
  return pd.task_state == PD_STATE_SRC_DISCONNECTED
#ifdef CONFIG_USB_PD_DUAL_ROLE
         || pd.task_state == PD_STATE_SNK_DISCONNECTED
#endif
          ;
}

#ifdef CONFIG_USBC_VCONN
static void pd_set_vconn_role(int role)
{
  if (role == PD_ROLE_VCONN_ON)
    pd.flags |= PD_FLAGS_VCONN_ON;
  else
    pd.flags &= ~PD_FLAGS_VCONN_ON;

#ifdef CONFIG_USB_PD_DUAL_ROLE
  pd_update_saved_port_flags(PD_BBRMFLG_VCONN_ROLE, role);
#endif
}
#endif /* CONFIG_USBC_VCONN */

void pd_request_data_swap()
{
  if (DUAL_ROLE_IF_ELSE(pd.task_state == PD_STATE_SNK_READY, pd.task_state == PD_STATE_SRC_READY))
    set_state(PD_STATE_DR_SWAP);
  task_wake();
}

static void pd_set_power_role(enum pd_power_role role)
{
  pd.power_role = role;
#ifdef CONFIG_USB_PD_DUAL_ROLE
  pd_update_saved_port_flags(PD_BBRMFLG_POWER_ROLE, role);
#endif /* defined(CONFIG_USB_PD_DUAL_ROLE) */
}

static void pd_dr_swap()
{
  pd_set_data_role(!pd.data_role);
  pd.flags |= PD_FLAGS_CHECK_IDENTITY;
}

static void handle_ctrl_request(uint16_t head, uint32_t* payload)
{
  int type = PD_HEADER_TYPE(head);
  int res;

  switch (type)
  {
    case PD_CTRL_GOOD_CRC:
      /* should not get it */
      break;
    case PD_CTRL_PING:
      /* Nothing else to do */
      break;
    case PD_CTRL_GET_SOURCE_CAP:
      if (pd.task_state == PD_STATE_SRC_READY)
        set_state(PD_STATE_SRC_DISCOVERY);
      else
      {
        res = send_source_cap(AMS_RESPONSE);
        if ((res >= 0) && (pd.task_state == PD_STATE_SRC_DISCOVERY))
          set_state(PD_STATE_SRC_NEGOCIATE);
      }
      break;
    case PD_CTRL_GET_SINK_CAP:
#ifdef CONFIG_USB_PD_DUAL_ROLE
      send_sink_cap();
#else
      send_control(NOT_SUPPORTED(pd.rev));
#endif
      break;
#ifdef CONFIG_USB_PD_DUAL_ROLE
    case PD_CTRL_GOTO_MIN:
#ifdef CONFIG_USB_PD_GIVE_BACK
      if (pd.task_state == PD_STATE_SNK_READY)
      {
        /*
         * Reduce power consumption now!
         *
         * The source will restore power to this sink
         * by sending a new source cap message at a
         * later time.
         */
        pd_snk_give_back(&pd.curr_limit, &pd.supply_voltage);
        set_state(PD_STATE_SNK_TRANSITION);
      }
#endif

      break;
    case PD_CTRL_PS_RDY:
      if (pd.task_state == PD_STATE_SNK_SWAP_SRC_DISABLE)
      {
        set_state(PD_STATE_SNK_SWAP_STANDBY);
      }
      else if (pd.task_state == PD_STATE_SRC_SWAP_STANDBY)
      {
        /* reset message ID and swap roles */
        pd.msg_id = 0;
        invalidate_last_message_id();
        pd_set_power_role(PD_ROLE_SINK);
        pd_update_roles();
        /*
         * Give the state machine time to read VBUS as high.
         * Note: This is empirically determined, not strictly
         * part of the USB PD spec.
         */
        pd.vbus_debounce_time = get_time().val + PD_T_DEBOUNCE;
        set_state(PD_STATE_SNK_DISCOVERY);
#ifdef CONFIG_USBC_VCONN_SWAP
      }
      else if (pd.task_state == PD_STATE_VCONN_SWAP_INIT)
      {
        /*
         * If VCONN is on, then this PS_RDY tells us it's
         * ok to turn VCONN off
         */
        if (pd.flags & PD_FLAGS_VCONN_ON)
          set_state(PD_STATE_VCONN_SWAP_READY);
#endif
      }
      else if (pd.task_state == PD_STATE_SNK_DISCOVERY)
      {
        /* Don't know what power source is ready. Reset. */
        set_state(PD_STATE_HARD_RESET_SEND);
      }
      else if (pd.task_state == PD_STATE_SNK_SWAP_STANDBY)
      {
        /* Do nothing, assume this is a redundant PD_RDY */
      }
      else if (pd.power_role == PD_ROLE_SINK)
      {
        /*
         * Give the source some time to send any messages before
         * we start our interrogation.  Add some jitter of up to
         * ~192ms to prevent multiple collisions.
         */
        if (pd.task_state == PD_STATE_SNK_TRANSITION)
          pd.ready_state_holdoff_timer =
                  get_time().val + SNK_READY_HOLD_OFF_US + (get_time().le.lo & 0xf) * 12 * MSEC_US;

        set_state(PD_STATE_SNK_READY);
        pd_set_input_current_limit(pd.curr_limit, pd.supply_voltage);
#ifdef CONFIG_CHARGE_MANAGER
        /* Set ceiling based on what's negotiated */
        // TODO charge_manager_set_ceil( CEIL_REQUESTOR_PD, pd.curr_limit);
#endif
      }
      break;
#endif
    case PD_CTRL_REJECT:
      if (pd.task_state == PD_STATE_ENTER_USB)
      {
#ifndef CONFIG_USBC_SS_MUX
        break;
#endif
        /*
         * Since Enter USB sets the mux state to SAFE mode,
         * resetting the mux state back to USB mode on
         * recieveing a NACK.
         */
        // TODO usb_mux_set(USB_PD_MUX_USB_ENABLED, USB_SWITCH_CONNECT, pd.polarity);

        set_state(READY_RETURN_STATE());
        break;
      }
    case PD_CTRL_WAIT:
      if (pd.task_state == PD_STATE_DR_SWAP)
      {
        if (type == PD_CTRL_WAIT) /* try again ... */
          pd.flags |= PD_FLAGS_CHECK_DR_ROLE;
        set_state(READY_RETURN_STATE());
      }
#ifdef CONFIG_USBC_VCONN_SWAP
      else if (pd.task_state == PD_STATE_VCONN_SWAP_SEND)
        set_state(READY_RETURN_STATE());
#endif
#ifdef CONFIG_USB_PD_DUAL_ROLE
      else if (pd.task_state == PD_STATE_SRC_SWAP_INIT)
        set_state(PD_STATE_SRC_READY);
      else if (pd.task_state == PD_STATE_SNK_SWAP_INIT)
        set_state(PD_STATE_SNK_READY);
      else if (pd.task_state == PD_STATE_SNK_REQUESTED)
      {
        /*
         * On reception of a WAIT message, transition to
         * PD_STATE_SNK_READY after PD_T_SINK_REQUEST ms to
         * send another request.
         *
         * On reception of a REJECT message, transition to
         * PD_STATE_SNK_READY but don't resend the request if
         * we already have a contract in place.
         *
         * On reception of a REJECT message without a contract,
         * transition to PD_STATE_SNK_DISCOVERY instead.
         */
        if (type == PD_CTRL_WAIT)
        {
          /*
           * Trigger a new power request when
           * we enter PD_STATE_SNK_READY
           */
          pd.new_power_request = 1;

          /*
           * After the request is triggered,
           * make sure the request is sent.
           */
          pd.prev_request_mv = 0;

          /*
           * Transition to PD_STATE_SNK_READY
           * after PD_T_SINK_REQUEST ms.
           */
          set_state_timeout(get_time().val + PD_T_SINK_REQUEST, PD_STATE_SNK_READY);
        }
        else
        {
          /* The request was rejected */
          const int in_contract = pd.flags & PD_FLAGS_EXPLICIT_CONTRACT;
          set_state(in_contract ? PD_STATE_SNK_READY : PD_STATE_SNK_DISCOVERY);
        }
      }
#endif
      break;
    case PD_CTRL_ACCEPT:
      if (pd.task_state == PD_STATE_SOFT_RESET)
      {
        /*
         * For the case that we sent soft reset in SNK_DISCOVERY
         * on startup due to VBUS never low, clear the flag.
         */
        pd.flags &= ~PD_FLAGS_VBUS_NEVER_LOW;
        execute_soft_reset();
      }
      else if (pd.task_state == PD_STATE_DR_SWAP)
      {
        /* switch data role */
        pd_dr_swap();
        set_state(READY_RETURN_STATE());
#ifdef CONFIG_USB_PD_DUAL_ROLE
#ifdef CONFIG_USBC_VCONN_SWAP
      }
      else if (pd.task_state == PD_STATE_VCONN_SWAP_SEND)
      {
        /* switch vconn */
        set_state(PD_STATE_VCONN_SWAP_INIT);
#endif
      }
      else if (pd.task_state == PD_STATE_SRC_SWAP_INIT)
      {
        /* explicit contract goes away for power swap */
        pd.flags &= ~PD_FLAGS_EXPLICIT_CONTRACT;
        pd_update_saved_port_flags(PD_BBRMFLG_EXPLICIT_CONTRACT, 0);
        set_state(PD_STATE_SRC_SWAP_SNK_DISABLE);
      }
      else if (pd.task_state == PD_STATE_SNK_SWAP_INIT)
      {
        /* explicit contract goes away for power swap */
        pd.flags &= ~PD_FLAGS_EXPLICIT_CONTRACT;
        pd_update_saved_port_flags(PD_BBRMFLG_EXPLICIT_CONTRACT, 0);
        set_state(PD_STATE_SNK_SWAP_SNK_DISABLE);
      }
      else if (pd.task_state == PD_STATE_SNK_REQUESTED)
      {
        /* explicit contract is now in place */
        pd.flags |= PD_FLAGS_EXPLICIT_CONTRACT;
        pd_update_saved_port_flags(PD_BBRMFLG_EXPLICIT_CONTRACT, 1);
        set_state(PD_STATE_SNK_TRANSITION);
#endif
      }
      break;
    case PD_CTRL_SOFT_RESET:
      execute_soft_reset();
      pd.msg_id = 0;
      /* We are done, acknowledge with an Accept packet */
      send_control(PD_CTRL_ACCEPT);
      break;
    case PD_CTRL_PR_SWAP:
#ifdef CONFIG_USB_PD_DUAL_ROLE
      if (pd_is_power_swap_succesful())
      {
        send_control(PD_CTRL_ACCEPT);
        /*
         * Clear flag for checking power role to avoid
         * immediately requesting another swap.
         */
        pd.flags &= ~PD_FLAGS_CHECK_PR_ROLE;
        set_state(DUAL_ROLE_IF_ELSE(PD_STATE_SNK_SWAP_SNK_DISABLE, PD_STATE_SRC_SWAP_SNK_DISABLE));
      }
      else
      {
        send_control(PD_CTRL_REJECT);
      }
#else
      send_control(NOT_SUPPORTED(pd.rev));
#endif
      break;
    case PD_CTRL_DR_SWAP:
      if (pd_check_data_swap(pd.data_role))
      {
        /*
         * Accept switch and perform data swap. Clear
         * flag for checking data role to avoid
         * immediately requesting another swap.
         */
        pd.flags &= ~PD_FLAGS_CHECK_DR_ROLE;
        if (send_control(PD_CTRL_ACCEPT) >= 0)
          pd_dr_swap();
      }
      else
      {
        send_control(PD_CTRL_REJECT);
      }
      break;
    case PD_CTRL_VCONN_SWAP:
#ifdef CONFIG_USBC_VCONN_SWAP
      if (pd.task_state == PD_STATE_SRC_READY || pd.task_state == PD_STATE_SNK_READY)
      {
        if (pd_check_vconn_swap())
        {
          if (send_control(PD_CTRL_ACCEPT) > 0)
            set_state(PD_STATE_VCONN_SWAP_INIT);
        }
        else
        {
          send_control(PD_CTRL_REJECT);
        }
      }
#else
      send_control(REFUSE(pd.rev));
#endif
      break;
    default:
#ifdef CONFIG_USB_PD_REV30
      send_control(PD_CTRL_NOT_SUPPORTED);
#endif
      CPRINTF("C Unhandled ctrl message type %d\n", type);
  }
}

#ifdef CONFIG_USB_PD_REV30
static void handle_ext_request(uint16_t head, uint32_t* payload)
{
  int type = PD_HEADER_TYPE(head);

  switch (type)
  {
    case PD_EXT_GET_BATTERY_CAP:
      send_battery_cap(payload);
      break;
    case PD_EXT_GET_BATTERY_STATUS:
      send_battery_status(payload);
      break;
    case PD_EXT_BATTERY_CAP:
      break;
    default:
      send_control(PD_CTRL_NOT_SUPPORTED);
  }
}
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
enum pd_drp_next_states drp_auto_toggle_next_state(uint64_t* drp_sink_time,
                                                   enum pd_power_role power_role,
                                                   enum pd_dual_role_states drp_state,
                                                   enum tcpc_cc_voltage_status cc1,
                                                   enum tcpc_cc_voltage_status cc2,
                                                   int auto_toggle_supported)
{
  const int hardware_debounced_unattached = ((drp_state == PD_DRP_TOGGLE_ON) && auto_toggle_supported);

  /* Set to appropriate port state */
  if (cc_is_open(cc1, cc2))
  {
    /*
     * If nothing is attached then use drp_state to determine next
     * state. If DRP auto toggle is still on, then remain in the
     * DRP_AUTO_TOGGLE state. Otherwise, stop dual role toggling
     * and go to a disconnected state.
     */
    switch (drp_state)
    {
      case PD_DRP_TOGGLE_OFF:
        return DRP_TC_DEFAULT;
      case PD_DRP_FREEZE:
        if (power_role == PD_ROLE_SINK)
          return DRP_TC_UNATTACHED_SNK;
        else
          return DRP_TC_UNATTACHED_SRC;
      case PD_DRP_FORCE_SINK:
        return DRP_TC_UNATTACHED_SNK;
      case PD_DRP_FORCE_SOURCE:
        return DRP_TC_UNATTACHED_SRC;
      case PD_DRP_TOGGLE_ON:
      default:
        if (!auto_toggle_supported)
        {
          if (power_role == PD_ROLE_SINK)
            return DRP_TC_UNATTACHED_SNK;
          else
            return DRP_TC_UNATTACHED_SRC;
        }

        return DRP_TC_DRP_AUTO_TOGGLE;
    }
  }
  else if ((cc_is_rp(cc1) || cc_is_rp(cc2)) && drp_state != PD_DRP_FORCE_SOURCE)
  {
    /* SNK allowed unless ForceSRC */
    if (hardware_debounced_unattached)
      return DRP_TC_ATTACHED_WAIT_SNK;
    return DRP_TC_UNATTACHED_SNK;
  }
  else if (cc_is_at_least_one_rd(cc1, cc2) || cc_is_audio_acc(cc1, cc2))
  {
    /*
     * SRC allowed unless ForceSNK or Toggle Off
     *
     * Ideally we wouldn't use auto-toggle when drp_state is
     * TOGGLE_OFF/FORCE_SINK, but for some TCPCs, auto-toggle can't
     * be prevented in low power mode. Try being a sink in case the
     * connected device is dual-role (this ensures reliable charging
     * from a hub, b/72007056). 100 ms is enough time for a
     * dual-role partner to switch from sink to source. If the
     * connected device is sink-only, then we will attempt
     * TC_UNATTACHED_SNK twice (due to debounce time), then return
     * to low power mode (and stay there). After 200 ms, reset
     * ready for a new connection.
     */
    if (drp_state == PD_DRP_TOGGLE_OFF || drp_state == PD_DRP_FORCE_SINK)
    {
      if (get_time().val > *drp_sink_time + 200 * MSEC_US)
        *drp_sink_time = get_time().val;
      if (get_time().val < *drp_sink_time + 100 * MSEC_US)
        return DRP_TC_UNATTACHED_SNK;
      else
        return DRP_TC_DRP_AUTO_TOGGLE;
    }
    else
    {
      if (hardware_debounced_unattached)
        return DRP_TC_ATTACHED_WAIT_SRC;
      return DRP_TC_UNATTACHED_SRC;
    }
  }
  else
  {
    /* Anything else, keep toggling */
    if (!auto_toggle_supported)
    {
      if (power_role == PD_ROLE_SINK)
        return DRP_TC_UNATTACHED_SNK;
      else
        return DRP_TC_UNATTACHED_SRC;
    }

    return DRP_TC_DRP_AUTO_TOGGLE;
  }
}
#endif

static void handle_request(uint16_t head, uint32_t* payload)
{
  int cnt = PD_HEADER_CNT(head);
  int data_role = PD_HEADER_DROLE(head);
  int p;

  /* dump received packet content (only dump ping at debug level 3) */
  if ((debug_level == 2 && PD_HEADER_TYPE(head) != PD_CTRL_PING) || debug_level >= 3)
  {
    CPRINTF("RECV %04x/%d ", head, cnt);
    for (p = 0; p < cnt; p++)
      CPRINTF("[%d]%08x ", p, payload[p]);
    CPRINTF("\n");
  }

  /*
   * If we are in disconnected state, we shouldn't get a request. Do
   * a hard reset if we get one.
   */
  if (!pd_is_connected())
    set_state(PD_STATE_HARD_RESET_SEND);

  /*
   * When a data role conflict is detected, USB-C ErrorRecovery
   * actions shall be performed, and transitioning to unattached state
   * is one such legal action.
   */
  if (pd.data_role == data_role)
  {
    /*
     * If the port doesn't support removing the terminations, just
     * go to the unattached state.
     */
    if (tcpm_set_cc(TYPEC_CC_OPEN) == EC_SUCCESS)
    {
      /* Do not drive VBUS or VCONN. */
      pd_power_supply_reset();
#ifdef CONFIG_USBC_VCONN
      set_vconn(port, 0);
#endif /* defined(CONFIG_USBC_VCONN) */
      usleep(PD_T_ERROR_RECOVERY);

      /* Restore terminations. */
      tcpm_set_cc(DUAL_ROLE_IF_ELSE(TYPEC_CC_RD, TYPEC_CC_RP));
    }
    set_state(DUAL_ROLE_IF_ELSE(PD_STATE_SNK_DISCONNECTED, PD_STATE_SRC_DISCONNECTED));
    return;
  }

#ifdef CONFIG_USB_PD_REV30
  /* Check if this is an extended chunked data message. */
  if (pd.rev == PD_REV30 && PD_HEADER_EXT(head))
  {
    handle_ext_request(head, payload);
    return;
  }
#endif
  if (cnt)
    handle_data_request(head, payload);
  else
    handle_ctrl_request(head, payload);
}

void pd_send_vdm(uint32_t vid, int cmd, const uint32_t* data, int count)
{
  if (count > VDO_MAX_SIZE - 1)
  {
    CPRINTF("VDM over max size\n");
    return;
  }

  /* set VDM header with VID & CMD */
  pd.vdo_data[0] = VDO(vid, ((vid & USB_SID_PD) == USB_SID_PD) ? 1 : (PD_VDO_CMD(cmd) <= CMD_ATTENTION), cmd);
#ifdef CONFIG_USB_PD_REV30
  pd.vdo_data[0] |= VDO_SVDM_VERS(vdo_ver[pd.rev]);
#endif
  queue_vdm(pd.vdo_data, data, count);

  task_wake();
}

static inline int pdo_busy()
{
  /*
   * Note, main PDO state machine (pd_task) uses READY state exclusively
   * to denote port partners have successfully negociated a contract.  All
   * other protocol actions force state transitions.
   */
  int rv = (pd.task_state != PD_STATE_SRC_READY);
#ifdef CONFIG_USB_PD_DUAL_ROLE
  rv &= (pd.task_state != PD_STATE_SNK_READY);
#endif
  return rv;
}

static uint64_t vdm_get_ready_timeout(uint32_t vdm_hdr)
{
  uint64_t timeout;
  int cmd = PD_VDO_CMD(vdm_hdr);

  /* its not a structured VDM command */
  if (!PD_VDO_SVDM(vdm_hdr))
    return 500 * MSEC_US;

  switch (PD_VDO_CMDT(vdm_hdr))
  {
    case CMDT_INIT:
      if ((cmd == CMD_ENTER_MODE) || (cmd == CMD_EXIT_MODE))
        timeout = PD_T_VDM_WAIT_MODE_E;
      else
        timeout = PD_T_VDM_SNDR_RSP;
      break;
    default:
      if ((cmd == CMD_ENTER_MODE) || (cmd == CMD_EXIT_MODE))
        timeout = PD_T_VDM_E_MODE;
      else
        timeout = PD_T_VDM_RCVR_RSP;
      break;
  }
  return timeout;
}

static void pd_vdm_send_state_machine()
{
  int res;
  uint16_t header;
  enum tcpm_transmit_type msg_type = pd.xmit_type;

  switch (pd.vdm_state)
  {
    case VDM_STATE_READY:
      /* Only transmit VDM if connected. */
      if (!pd_is_connected())
      {
        pd.vdm_state = VDM_STATE_ERR_BUSY;
        break;
      }

      /*
       * if there's traffic or we're not in PDO ready state don't send
       * a VDM.
       */
      if (pdo_busy())
        break;

      /*
       * To communicate with the cable plug, an explicit contract
       * should be established, VCONN should be enabled and data role
       * that can communicate with the cable plug should be in place.
       * For USB3.0, UFP/DFP can communicate whereas in case of
       * USB2.0 only DFP can talk to the cable plug.
       *
       * For communication between USB2.0 UFP and cable plug,
       * data role swap takes place during source and sink
       * negotiation and in case of failure, a soft reset is issued.
       */
      if ((msg_type == TCPC_TX_SOP_PRIME) || (msg_type == TCPC_TX_SOP_PRIME_PRIME))
      {
        /* Prepare SOP'/SOP'' header and send VDM */
        header = PD_HEADER(
                PD_DATA_VENDOR_DEF, PD_PLUG_FROM_DFP_UFP, 0, pd.msg_id, (int)pd.vdo_count, pd_get_rev(TCPC_TX_SOP), 0);
        res = pd_transmit(msg_type, header, pd.vdo_data, AMS_START);
        /*
         * In the case of SOP', if there is no response from
         * the cable, it's a non-emark cable and therefore the
         * pd flow should continue irrespective of cable
         * response, sending discover_identity so the pd flow
         * remains intact.
         *
         * In the case of SOP'', if there is no response from
         * the cable, exit Thunderbolt-Compatible mode
         * discovery, reset the mux state since, the mux will
         * be set to a safe state before entering
         * Thunderbolt-Compatible mode and enter the default
         * mode.
         */
        if (res < 0)
        {
          header = PD_HEADER(PD_DATA_VENDOR_DEF,
                             pd.power_role,
                             pd.data_role,
                             pd.msg_id,
                             (int)pd.vdo_count,
                             pd_get_rev(TCPC_TX_SOP),
                             0);
#ifdef CONFIG_USBC_SS_MUX
          if ((msg_type == TCPC_TX_SOP_PRIME_PRIME))
          {
            exit_tbt_mode_sop_prime();
          }
          else
#endif
                  if (msg_type == TCPC_TX_SOP_PRIME)
          {
            pd.vdo_data[0] = VDO(USB_SID_PD, 1, CMD_DISCOVER_SVID);
          }
          res = pd_transmit(TCPC_TX_SOP, header, pd.vdo_data, AMS_START);
          reset_pd_cable();
        }
      }
      else
      {
        /* Prepare SOP header and send VDM */
        header = PD_HEADER(PD_DATA_VENDOR_DEF,
                           pd.power_role,
                           pd.data_role,
                           pd.msg_id,
                           (int)pd.vdo_count,
                           pd_get_rev(TCPC_TX_SOP),
                           0);
        res = pd_transmit(TCPC_TX_SOP, header, pd.vdo_data, AMS_START);
      }

      if (res < 0)
      {
        pd.vdm_state = VDM_STATE_ERR_SEND;
      }
      else
      {
        pd.vdm_state = VDM_STATE_BUSY;
        pd.vdm_timeout.val = get_time().val + vdm_get_ready_timeout(pd.vdo_data[0]);
      }
      break;
    case VDM_STATE_WAIT_RSP_BUSY:
      /* wait and then initiate request again */
      if (get_time().val > pd.vdm_timeout.val)
      {
        pd.vdo_data[0] = pd.vdo_retry;
        pd.vdo_count = 1;
        pd.vdm_state = VDM_STATE_READY;
      }
      break;
    case VDM_STATE_BUSY:
      /* Wait for VDM response or timeout */
      if (pd.vdm_timeout.val && (get_time().val > pd.vdm_timeout.val))
      {
        pd.vdm_state = VDM_STATE_ERR_TMOUT;
      }
      break;
    default:
      break;
  }
}

#ifdef CONFIG_CMD_PD_DEV_DUMP_INFO
static inline void pd_dev_dump_info(uint16_t dev_id, uint8_t* hash)
{
  int j;
  ccprintf("DevId:%d.%d Hash:", HW_DEV_ID_MAJ(dev_id), HW_DEV_ID_MIN(dev_id));
  for (j = 0; j < PD_RW_HASH_SIZE; j += 4)
  {
    ccprintf(" 0x%02x%02x%02x%02x", hash[j + 3], hash[j + 2], hash[j + 1], hash[j]);
  }
  ccprintf("\n");
}
#endif /* CONFIG_CMD_PD_DEV_DUMP_INFO */

int pd_dev_store_rw_hash(uint16_t dev_id, uint32_t* rw_hash, uint32_t current_image)
{
#ifdef CONFIG_COMMON_RUNTIME
  int i;
#endif

#ifdef CONFIG_USB_PD_CHROMEOS
  pd.dev_id = dev_id;
  memcpy(pd.dev_rw_hash, rw_hash, PD_RW_HASH_SIZE);
#endif
#ifdef CONFIG_CMD_PD_DEV_DUMP_INFO
  if (debug_level >= 2)
    pd_dev_dump_info(dev_id, (uint8_t*)rw_hash);
#endif
#ifdef CONFIG_USB_PD_CHROMEOS
  pd.current_image = current_image;
#endif

#ifdef CONFIG_COMMON_RUNTIME
  /* Search table for matching device / hash */
  for (i = 0; i < RW_HASH_ENTRIES; i++)
    if (dev_id == rw_hash_table[i].dev_id)
      return !memcmp(rw_hash, rw_hash_table[i].dev_rw_hash, PD_RW_HASH_SIZE);
#endif
  return 0;
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
enum pd_dual_role_states pd_get_dual_role(void) { return drp_state; }

#ifdef CONFIG_USB_PD_TRY_SRC
static void pd_update_try_source(void)
{
  int i;

#ifndef CONFIG_CHARGER
  int batt_soc = board_get_battery_soc();
#else
  int batt_soc = charge_get_percent();
#endif

  /*
   * Enable try source when dual-role toggling AND battery is present
   * and at some minimum percentage.
   */
  pd_try_src_enable = drp_state == PD_DRP_TOGGLE_ON && batt_soc >= CONFIG_USB_PD_TRY_SRC_MIN_BATT_SOC;
#if defined(CONFIG_BATTERY_PRESENT_CUSTOM) || defined(CONFIG_BATTERY_PRESENT_GPIO)
  /*
   * When battery is cutoff in ship mode it may not be reliable to
   * check if battery is present with its state of charge.
   * Also check if battery is initialized and ready to provide power.
   */
  pd_try_src_enable &= (battery_is_present() == BP_YES);
#endif

  /*
   * Clear this flag to cover case where a TrySrc
   * mode went from enabled to disabled and trying_source
   * was active at that time.
   */
  pd.flags &= ~PD_FLAGS_TRY_SRC;
}
// TODO issue #135
// DECLARE_HOOK(HOOK_BATTERY_SOC_CHANGE, pd_update_try_source, HOOK_PRIO_DEFAULT);
#endif

void pd_set_dual_role_no_wakeup(enum pd_dual_role_states state)
{
  drp_state = state;

#ifdef CONFIG_USB_PD_TRY_SRC
  pd_update_try_source();
#endif
}

static int pd_is_power_swapping()
{
  /* return true if in the act of swapping power roles */
  return pd.task_state == PD_STATE_SNK_SWAP_SNK_DISABLE || pd.task_state == PD_STATE_SNK_SWAP_SRC_DISABLE ||
         pd.task_state == PD_STATE_SNK_SWAP_STANDBY || pd.task_state == PD_STATE_SNK_SWAP_COMPLETE ||
         pd.task_state == PD_STATE_SRC_SWAP_SNK_DISABLE || pd.task_state == PD_STATE_SRC_SWAP_SRC_DISABLE ||
         pd.task_state == PD_STATE_SRC_SWAP_STANDBY;
}

void pd_set_dual_role(enum pd_dual_role_states state)
{
  pd_set_dual_role_no_wakeup(state);

  /* Inform PD tasks of dual role change. */
  task_set_event(PD_EVENT_UPDATE_DUAL_ROLE);
}

void pd_update_dual_role_config()
{
  /*
   * Change to sink if port is currently a source AND (new DRP
   * state is force sink OR new DRP state is toggle off and we are in the
   * source disconnected state).
   */
  if (pd.power_role == PD_ROLE_SOURCE &&
      (drp_state == PD_DRP_FORCE_SINK ||
       (drp_state == PD_DRP_TOGGLE_OFF && pd.task_state == PD_STATE_SRC_DISCONNECTED)))
  {
    pd_set_power_role(PD_ROLE_SINK);
    set_state(PD_STATE_SNK_DISCONNECTED);
    tcpm_set_cc(TYPEC_CC_RD);
    /* Make sure we're not sourcing VBUS. */
    pd_power_supply_reset();
  }

  /*
   * Change to source if port is currently a sink and the
   * new DRP state is force source. If we are performing
   * power swap we won't change anything because
   * changing state will disrupt power swap process
   * and we are power swapping to desired power role.
   */
  if (pd.power_role == PD_ROLE_SINK && drp_state == PD_DRP_FORCE_SOURCE && !pd_is_power_swapping())
  {
    pd_set_power_role(PD_ROLE_SOURCE);
    set_state(PD_STATE_SRC_DISCONNECTED);
    tcpm_set_cc(TYPEC_CC_RP);
  }
}

int pd_get_role() { return pd.power_role; }

/*
 * Provide Rp to ensure the partner port is in a known state (eg. not
 * PD negotiated, not sourcing 20V).
 */
static void pd_partner_port_reset()
{
  uint64_t timeout;
  uint8_t flags;

  /*
   * If there is no contract in place (or if we fail to read the BBRAM
   * flags), there is no need to reset the partner.
   */
  if (pd_get_saved_port_flags(&flags) != EC_SUCCESS || !(flags & PD_BBRMFLG_EXPLICIT_CONTRACT))
    return;

  /*
   * If we reach here, an explicit contract is in place.
   *
   * If PD communications are allowed, don't apply Rp.  We'll issue a
   * SoftReset later on and renegotiate our contract.  This particular
   * condition only applies to unlocked RO images with an explicit
   * contract in place.
   */
  if (pd_comm_is_enabled())
  {
    return;
  }

  /* If we just lost power, don't apply Rp. */
#if 0
	if (system_get_reset_flags() &
	    (EC_RESET_FLAG_BROWNOUT | EC_RESET_FLAG_POWER_ON))
		return;
#endif

  /*
   * Clear the active contract bit before we apply Rp in case we
   * intentionally brown out because we cut off our only power supply.
   */
  pd_update_saved_port_flags(PD_BBRMFLG_EXPLICIT_CONTRACT, 0);

  /* Provide Rp for 200 msec. or until we no longer have VBUS. */
  CPRINTF("Apply Rp!\n");
  // TODO cflush();
  tcpm_set_cc(TYPEC_CC_RP);
  timeout = get_time().val + 200 * MSEC_US;

  while (get_time().val < timeout && pd_is_vbus_present())
    msleep(10);
}
#endif /* CONFIG_USB_PD_DUAL_ROLE */

enum pd_power_role pd_get_power_role() { return pd.power_role; }

enum pd_data_role pd_get_data_role() { return pd.data_role; }

enum pd_cc_states pd_get_task_cc_state() { return pd.cc_state; }

uint8_t pd_get_task_state() { return pd.task_state; }

const char* pd_get_task_state_name()
{
#ifdef CONFIG_USB_PD_TCPMV1_DEBUG
  if (debug_level > 0)
    return pd_state_names[pd.task_state];
#endif
  return "";
}

int pd_get_vconn_state() { return !!(pd.flags & PD_FLAGS_VCONN_ON); }

int pd_get_partner_dual_role_power() { return !!(pd.flags & PD_FLAGS_PARTNER_DR_POWER); }

int pd_get_partner_unconstr_power() { return !!(pd.flags & PD_FLAGS_PARTNER_UNCONSTR); }

int pd_get_polarity() { return pd.polarity; }

int pd_get_partner_data_swap_capable()
{
  /* return data swap capable status of port partner */
  return pd.flags & PD_FLAGS_PARTNER_DR_DATA;
}

#ifdef CONFIG_COMMON_RUNTIME
void pd_comm_enable(int enable)
{
  /* We don't check port >= 1 deliberately */
  pd_comm_enabled = enable;

  /* If type-C connection, then update the TCPC RX enable */
  if (pd_is_connected())
    tcpm_set_rx_enable(enable);

#ifdef CONFIG_USB_PD_DUAL_ROLE
  /*
   * If communications are enabled, start hard reset timer for
   * any port in PD_SNK_DISCOVERY.
   */
  if (enable && pd.task_state == PD_STATE_SNK_DISCOVERY)
    set_state_timeout(get_time().val + PD_T_SINK_WAIT_CAP, PD_STATE_HARD_RESET_SEND);
#endif
}
#endif

void pd_ping_enable(int enable)
{
  if (enable)
    pd.flags |= PD_FLAGS_PING_ENABLED;
  else
    pd.flags &= ~PD_FLAGS_PING_ENABLED;
}

/*
 * CC values for regular sources and Debug sources (aka DTS)
 *
 * Source type  Mode of Operation   CC1    CC2
 * ---------------------------------------------
 * Regular      Default USB Power   RpUSB  Open
 * Regular      USB-C @ 1.5 A       Rp1A5  Open
 * Regular      USB-C @ 3 A         Rp3A0  Open
 * DTS          Default USB Power   Rp3A0  Rp1A5
 * DTS          USB-C @ 1.5 A       Rp1A5  RpUSB
 * DTS          USB-C @ 3 A         Rp3A0  RpUSB
 */

/**
 * Returns the polarity of a Sink.
 */
enum tcpc_cc_polarity get_snk_polarity(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
{
  /* The following assumes:
   *
   * TYPEC_CC_VOLT_RP_3_0 > TYPEC_CC_VOLT_RP_1_5
   * TYPEC_CC_VOLT_RP_1_5 > TYPEC_CC_VOLT_RP_DEF
   * TYPEC_CC_VOLT_RP_DEF > TYPEC_CC_VOLT_OPEN
   */
  if (cc_is_src_dbg_acc(cc1, cc2))
    return (cc1 > cc2) ? POLARITY_CC1_DTS : POLARITY_CC2_DTS;

  return (cc1 > cc2) ? POLARITY_CC1 : POLARITY_CC2;
}

enum tcpc_cc_polarity get_src_polarity(enum tcpc_cc_voltage_status cc1, enum tcpc_cc_voltage_status cc2)
{
  return (cc1 == TYPEC_CC_VOLT_RD) ? POLARITY_CC1 : POLARITY_CC2;
}

void pd_set_polarity(enum tcpc_cc_polarity polarity)
{
  tcpm_set_polarity(polarity);

#ifdef CONFIG_USBC_PPC_POLARITY
  ppc_set_polarity(polarity);
#endif
}

#if defined(CONFIG_CHARGE_MANAGER)
/*
 * CC values for regular sources and Debug sources (aka DTS)
 *
 * Source type  Mode of Operation   CC1    CC2
 * ---------------------------------------------
 * Regular      Default USB Power   RpUSB  Open
 * Regular      USB-C @ 1.5 A       Rp1A5  Open
 * Regular      USB-C @ 3 A	    Rp3A0  Open
 * DTS		Default USB Power   Rp3A0  Rp1A5
 * DTS		USB-C @ 1.5 A       Rp1A5  RpUSB
 * DTS		USB-C @ 3 A	    Rp3A0  RpUSB
 */

typec_current_t usb_get_typec_current_limit(enum tcpc_cc_polarity polarity,
                                            enum tcpc_cc_voltage_status cc1,
                                            enum tcpc_cc_voltage_status cc2)
{
  typec_current_t charge = 0;
  enum tcpc_cc_voltage_status cc;
  enum tcpc_cc_voltage_status cc_alt;

  cc = polarity_rm_dts(polarity) ? cc2 : cc1;
  cc_alt = polarity_rm_dts(polarity) ? cc1 : cc2;

  switch (cc)
  {
    case TYPEC_CC_VOLT_RP_3_0:
      if (!cc_is_rp(cc_alt) || cc_alt == TYPEC_CC_VOLT_RP_DEF)
        charge = 3000;
      else if (cc_alt == TYPEC_CC_VOLT_RP_1_5)
        charge = 500;
      break;
    case TYPEC_CC_VOLT_RP_1_5:
      charge = 1500;
      break;
    case TYPEC_CC_VOLT_RP_DEF:
      charge = 500;
      break;
    default:
      break;
  }

#ifdef CONFIG_USBC_DISABLE_CHARGE_FROM_RP_DEF
  if (charge == 500)
    charge = 0;
#endif

  if (cc_is_rp(cc_alt))
    charge |= TYPEC_CURRENT_DTS_MASK;

  return charge;
}

/**
 * Signal power request to indicate a charger update that affects the port.
 */
void pd_set_new_power_request()
{
  pd.new_power_request = 1;

  task_wake();
}
#endif /* CONFIG_CHARGE_MANAGER */

#if defined(CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP) && defined(CONFIG_USBC_SS_MUX)
/*
 * Backwards compatible DFP does not support USB SS because it applies VBUS
 * before debouncing CC and setting USB SS muxes, but SS detection will fail
 * before we are done debouncing CC.
 */
#error "Backwards compatible DFP does not support USB"
#endif

#ifdef CONFIG_COMMON_RUNTIME

/* Initialize globals based on system state. */
static void pd_init_tasks(void)
{
  static int initialized;
  int enable = 1;
  int i;

  /* Initialize globals once, for all PD tasks.  */
  if (initialized)
    return;

#if defined(HAS_TASK_CHIPSET) && defined(CONFIG_USB_PD_DUAL_ROLE)
  /* Set dual-role state based on chipset power state */
  if (chipset_in_state(CHIPSET_STATE_ANY_OFF))
    drp_state = PD_DRP_FORCE_SINK;
  else if (chipset_in_state(CHIPSET_STATE_SUSPEND))
    drp_state = PD_DRP_TOGGLE_OFF;
  else /* CHIPSET_STATE_ON */
    drp_state = PD_DRP_TOGGLE_ON;
#endif

#if defined(CONFIG_USB_PD_COMM_DISABLED)
  enable = 0;
#elif defined(CONFIG_USB_PD_COMM_LOCKED)
  /* Disable PD communication at init if we're in RO and locked. */
  if (!system_is_in_rw() && system_is_locked())
    enable = 0;
#endif
  pd_comm_enabled = enable;
  CPRINTS("PD comm %sabled", enable ? "en" : "dis");

  initialized = 1;
}
#endif /* CONFIG_COMMON_RUNTIME */

#ifndef CONFIG_USB_PD_TCPC
static int pd_restart_tcpc()
{
  if (board_set_tcpc_power_mode)
  {
    /* force chip reset */
    board_set_tcpc_power_mode(0);
  }
  return tcpm_init();
}
#endif

void pd_init()
{
  // set initial status
  pd.tx_status = TCPC_TX_UNSET;

#ifdef CONFIG_COMMON_RUNTIME
  pd_init_tasks();
#endif

  /* Ensure the power supply is in the default state */
  pd_power_supply_reset();

#ifdef CONFIG_USB_PD_TCPC_BOARD_INIT
  /* Board specific TCPC init */
  board_tcpc_init();
#endif

  /* Initialize TCPM driver and wait for TCPC to be ready */
  res = reset_device_and_notify();
  invalidate_last_message_id();

#ifdef CONFIG_USB_PD_DUAL_ROLE
  pd_partner_port_reset();
#endif

  CPRINTS("TCPC init %s", res ? "failed" : "ready");
  this_state = PD_STATE_SUSPENDED;
#ifndef CONFIG_USB_PD_TCPC
  if (!res)
  {
    struct ec_response_pd_chip_info* info;
    tcpm_get_chip_info(0, &info);
    CPRINTS("TCPC VID:0x%x PID:0x%x DID:0x%x FWV:0x%lx",
            info->vendor_id,
            info->product_id,
            info->device_id,
            info->fw_version_number);
  }
#endif

#ifdef CONFIG_USB_PD_REV30
  /* Set Revision to highest */
  pd.rev = PD_REV30;
  pd_ca_reset();
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE
  /*
   * If VBUS is high, then initialize flag for VBUS has always been
   * present. This flag is used to maintain a PD connection after a
   * reset by sending a soft reset.
   */
  pd.flags = pd_is_vbus_present() ? PD_FLAGS_VBUS_NEVER_LOW : 0;
#endif

  /* Disable TCPC RX until connection is established */
  tcpm_set_rx_enable(0);

#ifdef CONFIG_USBC_SS_MUX
  /* Initialize USB mux to its default state */
  usb_mux_init();
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE
  /*
   * If there's an explicit contract in place, let's restore the data and
   * power roles such that any messages we send to the port partner will
   * still be valid.
   */
  if (pd_comm_is_enabled() && (pd_get_saved_port_flags(&saved_flgs) == EC_SUCCESS) &&
      (saved_flgs & PD_BBRMFLG_EXPLICIT_CONTRACT))
  {
    /* Only attempt to maintain previous sink contracts */
    if ((saved_flgs & PD_BBRMFLG_POWER_ROLE) == PD_ROLE_SINK)
    {
      pd_set_power_role((saved_flgs & PD_BBRMFLG_POWER_ROLE) ? PD_ROLE_SOURCE : PD_ROLE_SINK);
      pd_set_data_role((saved_flgs & PD_BBRMFLG_DATA_ROLE) ? PD_ROLE_DFP : PD_ROLE_UFP);
#ifdef CONFIG_USBC_VCONN
      pd_set_vconn_role(port, (saved_flgs & PD_BBRMFLG_VCONN_ROLE) ? PD_ROLE_VCONN_ON : PD_ROLE_VCONN_OFF);
#endif /* CONFIG_USBC_VCONN */

      /*
       * Since there is an explicit contract in place, let's
       * issue a SoftReset such that we can renegotiate with
       * our port partner in order to synchronize our state
       * machines.
       */
      this_state = PD_STATE_SOFT_RESET;

      /*
       * Re-discover any alternate modes we may have been
       * using with this port partner.
       */
      pd.flags |= PD_FLAGS_CHECK_IDENTITY;
    }
    else
    {
      /*
       * Vbus was turned off during the power supply reset
       * earlier, so clear the contract flag and re-start as
       * default role
       */
      pd_update_saved_port_flags(PD_BBRMFLG_EXPLICIT_CONTRACT, 0);
    }
    /*
     * Set the TCPC reset event such that we can set our CC
     * terminations, determine polarity, and enable RX so we
     * can hear back from our port partner if maintaining our old
     * connection.
     */
    task_set_event(PD_EVENT_TCPC_RESET);
  }
#endif /* defined(CONFIG_USB_PD_DUAL_ROLE) */
       /* Set the power role if we haven't already. */
  if (this_state != PD_STATE_SOFT_RESET)
    pd_set_power_role(PD_ROLE_DEFAULT());

  /* Initialize PD protocol state variables for each port. */
  pd.vdm_state = VDM_STATE_DONE;
  set_state(this_state);
#ifdef CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT
  ASSERT(PD_ROLE_DEFAULT() == PD_ROLE_SINK);
  tcpm_select_rp_value(CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT);
#else
  tcpm_select_rp_value(CONFIG_USB_PD_PULLUP);
#endif
  tcpm_set_cc(PD_ROLE_DEFAULT() == PD_ROLE_SOURCE ? TYPEC_CC_RP : TYPEC_CC_RD);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
  /* Initialize PD Policy engine */
  pd_dfp_pe_init();
#endif

#ifdef CONFIG_CHARGE_MANAGER
  /* Initialize PD and type-C supplier current limits to 0 */
  pd_set_input_current_limit(0, 0);
  typec_set_input_current_limit(0, 0);
  // charge_manager_update_dualrole( CAP_UNKNOWN);
#endif
}

static int consume_sop_repeat_message(uint8_t msg_id)
{
  if (pd.last_msg_id != msg_id)
  {
    pd.last_msg_id = msg_id;
    return 0;
  }
  CPRINTF("Repeat msg_id %d\n", msg_id);
  return 1;
}

/**
 * Identify and drop any duplicate messages received at the port.
 *
 * @param port USB PD TCPC port number
 * @param msg_header Message Header containing the RX message ID
 * @return True if the received message is a duplicate one, False otherwise.
 *
 * From USB PD version 1.3 section 6.7.1, the port which communicates
 * using SOP* Packets Shall maintain copies of the last MessageID for
 * each type of SOP* it uses.
 */
static int consume_repeat_message(uint32_t msg_header)
{
  uint8_t msg_id = PD_HEADER_ID(msg_header);
  enum tcpm_transmit_type sop = PD_HEADER_GET_SOP(msg_header);

  /* If repeat message ignore, except softreset control request. */
  if (PD_HEADER_TYPE(msg_header) == PD_CTRL_SOFT_RESET && PD_HEADER_CNT(msg_header) == 0)
  {
    return 0;
  }
  else if (sop == TCPC_TX_SOP_PRIME)
  {
    return consume_sop_prime_repeat_msg(msg_id);
  }
  else if (sop == TCPC_TX_SOP_PRIME_PRIME)
  {
    return consume_sop_prime_prime_repeat_msg(msg_id);
  }
  else
  {
    return consume_sop_repeat_message(msg_id);
  }
}

/**
 * Returns true if the port is currently in the try src state.
 */
static inline int is_try_src() { return pd.flags & PD_FLAGS_TRY_SRC; }

void pd_run_state_machine()
{
#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
  // TODO: should be static
  const int auto_toggle_supported = tcpm_auto_toggle_supported();
#endif

#ifdef CONFIG_USB_PD_REV30
  /* send any pending messages */
  pd_ca_send_pending();
#endif
  /* process VDM messages last */
  pd_vdm_send_state_machine();

  /* Verify board specific health status : current, voltages... */
  res = pd_board_checks();
  if (res != EC_SUCCESS)
  {
    CPRINTF("HARD RST board not success\n", pd.last_state);
    /* cut the power */
    pd_execute_hard_reset();
    /* notify the other side of the issue */
    pd_transmit(TCPC_TX_HARD_RESET, 0, NULL, AMS_START);
  }

  /* wait for next event/packet or timeout expiration */
  int evt = task_wait_event(timeout);

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
  if (evt & (PD_EXIT_LOW_POWER_EVENT_MASK | TASK_EVENT_WAKE))
    exit_low_power_mode();
  if (evt & PD_EVENT_DEVICE_ACCESSED)
    handle_device_access();
#endif
#ifdef CONFIG_POWER_COMMON
  if (evt & PD_EVENT_POWER_STATE_CHANGE)
    handle_new_power_state();
#endif

#if defined(CONFIG_USB_PD_ALT_MODE_DFP)
  if (evt & PD_EVENT_SYSJUMP)
  {
    exit_supported_alt_mode();
    notify_sysjump_ready();
  }
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE
  if (evt & PD_EVENT_UPDATE_DUAL_ROLE)
    pd_update_dual_role_config();
#endif

#ifdef CONFIG_USB_PD_TCPC
  /*
   * run port controller task to check CC and/or read incoming
   * messages
   */
  tcpc_run(evt);
#else
  /* if TCPC has reset, then need to initialize it again */
  if (evt & PD_EVENT_TCPC_RESET)
  {
    CPRINTS("TCPC reset!");
    if (tcpm_init() != EC_SUCCESS)
      CPRINTS("TCPC init failed");
#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
  }

  if ((evt & PD_EVENT_TCPC_RESET) && (pd.task_state != PD_STATE_DRP_AUTO_TOGGLE))
  {
#endif
#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
  }

  if ((evt & PD_EVENT_TCPC_RESET) && (pd[port].task_state != PD_STATE_DRP_AUTO_TOGGLE))
  {
#endif
#ifdef CONFIG_USB_PD_DUAL_ROLE
    if (pd.task_state == PD_STATE_SOFT_RESET)
    {
      enum tcpc_cc_voltage_status cc1, cc2;

      /*
       * Set the terminations to match our power
       * role.
       */
      tcpm_set_cc(pd.power_role ? TYPEC_CC_RP : TYPEC_CC_RD);

      /* Determine the polarity. */
      tcpm_get_cc(&cc1, &cc2);
      if (pd.power_role == PD_ROLE_SINK)
      {
        pd.polarity = get_snk_polarity(cc1, cc2);
      }
      else if (cc_is_snk_dbg_acc(cc1, cc2))
      {
        pd.polarity = board_get_src_dts_polarity();
      }
      else
      {
        pd.polarity = get_src_polarity(cc1, cc2);
      }
    }
    else
#endif /* CONFIG_USB_PD_DUAL_ROLE */
    {
      /* Ensure CC termination is default */
      tcpm_set_cc(PD_ROLE_DEFAULT() == PD_ROLE_SOURCE ? TYPEC_CC_RP : TYPEC_CC_RD);
    }
    /*
     * If we have a stable contract in the default role,
     * then simply update TCPC with some missing info
     * so that we can continue without resetting PD comms.
     * Otherwise, go to the default disconnected state
     * and force renegotiation.
     */
    if (pd.vdm_state == VDM_STATE_DONE &&
        (
#ifdef CONFIG_USB_PD_DUAL_ROLE
                (PD_ROLE_DEFAULT() == PD_ROLE_SINK && pd.task_state == PD_STATE_SNK_READY) ||
                (pd.task_state == PD_STATE_SOFT_RESET) ||
#endif
                (PD_ROLE_DEFAULT() == PD_ROLE_SOURCE && pd.task_state == PD_STATE_SRC_READY)))
    {
      pd_set_polarity(pd.polarity);
      tcpm_set_msg_header(pd.power_role, pd.data_role);
      tcpm_set_rx_enable(1);
    }
    else
    {
      /* Ensure state variables are at default */
      pd_set_power_role(PD_ROLE_DEFAULT());
      pd.vdm_state = VDM_STATE_DONE;
      set_state(PD_DEFAULT_STATE());
#ifdef CONFIG_USB_PD_DUAL_ROLE
      pd_update_dual_role_config();
#endif
    }
  }
#endif

#ifdef CONFIG_USBC_PPC
  /*
   * TODO: Useful for non-PPC cases as well, but only needed
   * for PPC cases right now. Revisit later.
   */
  if (evt & PD_EVENT_SEND_HARD_RESET)
    set_state(PD_STATE_HARD_RESET_SEND);
#endif /* defined(CONFIG_USBC_PPC) */

  if (evt & PD_EVENT_RX_HARD_RESET)
  {
    CPRINTF("HARD RST event, last state %s\n", pd.last_state);
    pd_execute_hard_reset();
  }

  /* process any potential incoming message */
  incoming_packet = tcpm_has_pending_message();
  if (incoming_packet)
  {
    /* Dequeue and consume duplicate message ID. */
    if (tcpm_dequeue_message(payload, &head) == EC_SUCCESS && !consume_repeat_message(head))
      handle_request(head, payload);

    /* Check if there are any more messages */
    if (tcpm_has_pending_message())
      task_set_event(TASK_EVENT_WAKE);
  }

  if (pd.req_suspend_state)
    set_state(PD_STATE_SUSPENDED);

  /* if nothing to do, verify the state of the world in 500ms */
  this_state = pd.task_state;
  timeout = 500 * MSEC_US;
  switch (this_state)
  {
    case PD_STATE_DISABLED:
      /* Nothing to do */
      break;
    case PD_STATE_SRC_DISCONNECTED:
      timeout = 10 * MSEC_US;

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
      /*
       * If SW decided we should be in a low power state and
       * the CC lines did not change, then don't talk with the
       * TCPC otherwise we might wake it up.
       */
      if (pd.flags & PD_FLAGS_LPM_REQUESTED && !(evt & PD_EVENT_CC))
        break;
#endif /* CONFIG_USB_PD_TCPC_LOW_POWER */

      tcpm_get_cc(&cc1, &cc2);

#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
      /*
       * Attempt TCPC auto DRP toggle if it is
       * not already auto toggling and not try.src
       */
      if (auto_toggle_supported && !(pd.flags & PD_FLAGS_TCPC_DRP_TOGGLE) && !is_try_src() && cc_is_open(cc1, cc2))
      {
        set_state(PD_STATE_DRP_AUTO_TOGGLE);
        timeout = 2 * MSEC_US;
        break;
      }
#endif

      /*
       * Transition to DEBOUNCE if we detect appropriate
       * signals
       *
       * (from 4.5.2.2.10.2 Exiting from Try.SRC State)
       * If try_src -and-
       *    have only one Rd (not both) => DEBOUNCE
       *
       * (from 4.5.2.2.7.2 Exiting from Unattached.SRC State)
       * If not try_src -and-
       *    have at least one Rd => DEBOUNCE -or-
       *    have audio access => DEBOUNCE
       *
       * try_src should not exit if both pins are Rd
       */
      if ((is_try_src() && cc_is_only_one_rd(cc1, cc2)) ||
          (!is_try_src() && (cc_is_at_least_one_rd(cc1, cc2) || cc_is_audio_acc(cc1, cc2))))
      {
#ifdef CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP
        /* Enable VBUS */
        if (pd_set_power_supply_ready())
          break;
#endif
        pd.cc_state = PD_CC_NONE;
        set_state(PD_STATE_SRC_DISCONNECTED_DEBOUNCE);
        break;
      }
#if defined(CONFIG_USB_PD_DUAL_ROLE)
      now = get_time();
      /*
       * Try.SRC state is embedded here. The port
       * shall transition to TryWait.SNK after
       * tDRPTry (PD_T_DRP_TRY) and Vbus is within
       * vSafe0V, or after tTryTimeout
       * (PD_T_TRY_TIMEOUT). Otherwise we should stay
       * within Try.SRC (break).
       */
      if (is_try_src())
      {
        if (now.val < pd.try_src_marker)
        {
          break;
        }
        else if (now.val < pd.try_timeout)
        {
          if (pd_is_vbus_present())
            break;
        }

        /*
         * Transition to TryWait.SNK now, so set
         * state and update src marker time.
         */
        set_state(PD_STATE_SNK_DISCONNECTED);
        pd_set_power_role(PD_ROLE_SINK);
        tcpm_set_cc(TYPEC_CC_RD);
        pd.try_src_marker = get_time().val + PD_T_DEBOUNCE;
        timeout = 2 * MSEC_US;
        break;
      }

      /*
       * If Try.SRC state is not active, then handle
       * the normal DRP toggle from SRC->SNK.
       */
      if (now.val < next_role_swap || drp_state == PD_DRP_FORCE_SOURCE || drp_state == PD_DRP_FREEZE)
        break;

      /*
       * Transition to SNK now, so set state and
       * update next role swap time.
       */
      set_state(PD_STATE_SNK_DISCONNECTED);
      pd_set_power_role(PD_ROLE_SINK);
      tcpm_set_cc(TYPEC_CC_RD);
      next_role_swap = get_time().val + PD_T_DRP_SNK;
      /* Swap states quickly */
      timeout = 2 * MSEC_US;
#endif
      break;
    case PD_STATE_SRC_DISCONNECTED_DEBOUNCE:
      timeout = 20 * MSEC_US;
      tcpm_get_cc(&cc1, &cc2);

      if (cc_is_snk_dbg_acc(cc1, cc2))
      {
        /* Debug accessory */
        new_cc_state = PD_CC_UFP_DEBUG_ACC;
      }
      else if (cc_is_at_least_one_rd(cc1, cc2))
      {
        /* UFP attached */
        new_cc_state = PD_CC_UFP_ATTACHED;
      }
      else if (cc_is_audio_acc(cc1, cc2))
      {
        /* Audio accessory */
        new_cc_state = PD_CC_UFP_AUDIO_ACC;
      }
      else
      {
        /* No UFP */
        set_state(PD_STATE_SRC_DISCONNECTED);
        timeout = 5 * MSEC_US;
        break;
      }

      /* Set debounce timer */
      if (new_cc_state != pd.cc_state)
      {
        pd.cc_debounce = get_time().val + (is_try_src() ? PD_T_DEBOUNCE : PD_T_CC_DEBOUNCE);
        pd.cc_state = new_cc_state;
        break;
      }

      /* Debounce the cc state */
      if (get_time().val < pd.cc_debounce)
      {
        break;
      }

#ifdef CONFIG_COMMON_RUNTIME
      /* Debounce complete */
      hook_notify(HOOK_USB_PD_CONNECT);
#endif

#ifdef CONFIG_USBC_PPC
      /*
       * If the port is latched off, just continue to
       * monitor for a detach.
       */
      if (usbc_ocp_is_port_latched_off())
        break;
#endif /* CONFIG_USBC_PPC */

      /* UFP is attached */
      if (new_cc_state == PD_CC_UFP_ATTACHED || new_cc_state == PD_CC_UFP_DEBUG_ACC)
      {
#ifdef CONFIG_USBC_PPC
        /* Inform PPC that a sink is connected. */
        ppc_dev_is_connected(PPC_DEV_SNK);
#endif /* CONFIG_USBC_PPC */
        if (new_cc_state == PD_CC_UFP_DEBUG_ACC)
        {
          pd.polarity = 0;
        }
        else
        {
          pd.polarity = get_src_polarity(cc1, cc2);
        }
        pd_set_polarity(pd.polarity);

        /* initial data role for source is DFP */
        pd_set_data_role(PD_ROLE_DFP);

        /* Enable Auto Discharge Disconnect */
        tcpm_enable_auto_discharge_disconnect(1);

        if (new_cc_state == PD_CC_UFP_DEBUG_ACC)
          pd.flags |= PD_FLAGS_TS_DTS_PARTNER;

#ifdef CONFIG_USBC_VCONN
        /*
         * Do not source Vconn when debug accessory is
         * detected. Section 4.5.2.2.17.1 in USB spec
         * v1-3
         */
        if (new_cc_state != PD_CC_UFP_DEBUG_ACC)
        {
          /*
           * Start sourcing Vconn before Vbus to
           * ensure we are within USB Type-C
           * Spec 1.3 tVconnON.
           */
          set_vconn(1);
          pd_set_vconn_role(PD_ROLE_VCONN_ON);
        }
#endif

#ifndef CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP
        /* Enable VBUS */
        if (pd_set_power_supply_ready())
        {
#ifdef CONFIG_USBC_VCONN
          /* Stop sourcing Vconn if Vbus failed */
          set_vconn(0);
          pd_set_vconn_role(PD_ROLE_VCONN_OFF);
#endif /* CONFIG_USBC_VCONN */
#ifdef CONFIG_USBC_SS_MUX
          usb_mux_set(USB_PD_MUX_NONE, USB_SWITCH_DISCONNECT, pd.polarity);
#endif /* CONFIG_USBC_SS_MUX */
          break;
        }
        /*
         * Set correct Rp value determined during
         * pd_set_power_supply_ready.  This should be
         * safe because Vconn is being sourced,
         * preventing incorrect CCD detection.
         */
        tcpm_set_cc(TYPEC_CC_RP);
#endif /* CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP */
        /* If PD comm is enabled, enable TCPC RX */
        if (pd_comm_is_enabled())
          tcpm_set_rx_enable(1);

        pd.flags |= PD_FLAGS_CHECK_PR_ROLE | PD_FLAGS_CHECK_DR_ROLE;
        hard_reset_count = 0;
        timeout = 5 * MSEC_US;

        set_state(PD_STATE_SRC_STARTUP);
      }
      /*
       * AUDIO_ACC will remain in this state indefinitely
       * until disconnect.
       */
      break;
    case PD_STATE_SRC_HARD_RESET_RECOVER:
      /* Do not continue until hard reset recovery time */
      if (get_time().val < pd.src_recover)
      {
        timeout = 50 * MSEC_US;
        break;
      }

#ifdef CONFIG_USBC_VCONN
      /*
       * Start sourcing Vconn again and set the flag, in case
       * it was 0 due to a previous swap
       */
      set_vconn(1);
      pd_set_vconn_role(PD_ROLE_VCONN_ON);
#endif

      /* Enable VBUS */
      timeout = 10 * MSEC_US;
      if (pd_set_power_supply_ready())
      {
        set_state(PD_STATE_SRC_DISCONNECTED);
        break;
      }
#ifdef CONFIG_USB_PD_TCPM_TCPCI
      /*
       * After transmitting hard reset, TCPM writes
       * to RECEIVE_DETECT register to enable
       * PD message passing.
       */
      if (pd_comm_is_enabled())
        tcpm_set_rx_enable(1);
#endif /* CONFIG_USB_PD_TCPM_TCPCI */

      set_state(PD_STATE_SRC_STARTUP);
      break;
    case PD_STATE_SRC_STARTUP:
      /* Reset cable attributes and flags */
      reset_pd_cable();
      /* Wait for power source to enable */
      if (pd.last_state != pd.task_state)
      {
        pd.flags |= PD_FLAGS_CHECK_IDENTITY;
        /* reset various counters */
        caps_count = 0;
        pd.msg_id = 0;
        snk_cap_count = 0;
        set_state_timeout(
#ifdef CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP
                /*
                 * delay for power supply to start up.
                 * subtract out debounce time if coming
                 * from debounce state since vbus is
                 * on during debounce.
                 */
                get_time().val + PD_POWER_SUPPLY_TURN_ON_DELAY -
                        (pd.last_state == PD_STATE_SRC_DISCONNECTED_DEBOUNCE ? PD_T_CC_DEBOUNCE : 0),
#else
                get_time().val + PD_POWER_SUPPLY_TURN_ON_DELAY,
#endif
                PD_STATE_SRC_DISCOVERY);
      }
      break;
    case PD_STATE_SRC_DISCOVERY:
      now = get_time();
      if (pd.last_state != pd.task_state)
      {
        caps_count = 0;
        next_src_cap = now.val;
        /*
         * If we have had PD connection with this port
         * partner, then start NoResponseTimer.
         */
        if (pd_capable())
          set_state_timeout(get_time().val + PD_T_NO_RESPONSE,
                            hard_reset_count < PD_HARD_RESET_COUNT ? PD_STATE_HARD_RESET_SEND :
                                                                     PD_STATE_SRC_DISCONNECTED);
      }

      /* Send source cap some minimum number of times */
      if (caps_count < PD_CAPS_COUNT && next_src_cap <= now.val)
      {
        /* Query capabilities of the other side */
        res = send_source_cap(AMS_START);
        /* packet was acked => PD capable device) */
        if (res >= 0)
        {
          set_state(PD_STATE_SRC_NEGOCIATE);
          timeout = 10 * MSEC_US;
          hard_reset_count = 0;
          caps_count = 0;
          /* Port partner is PD capable */
          pd.flags |= PD_FLAGS_PREVIOUS_PD_CONN;
        }
        else
        { /* failed, retry later */
          invalidate_last_message_id();
          timeout = PD_T_SEND_SOURCE_CAP;
          next_src_cap = now.val + PD_T_SEND_SOURCE_CAP;
#if 0
          // TODO : this is not in specks, but allows for negociations
          caps_count++;
#endif
        }
      }
      else if (caps_count < PD_CAPS_COUNT)
      {
        timeout = next_src_cap - now.val;
      }
      break;
    case PD_STATE_SRC_NEGOCIATE:
      /* wait for a "Request" message */
      if (pd.last_state != pd.task_state)
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, PD_STATE_HARD_RESET_SEND);
      break;
    case PD_STATE_SRC_ACCEPTED:
      /* Accept sent, wait for enabling the new voltage */
      if (pd.last_state != pd.task_state)
        set_state_timeout(get_time().val + PD_T_SINK_TRANSITION, PD_STATE_SRC_POWERED);
      break;
    case PD_STATE_SRC_POWERED:
      /* Switch to the new requested voltage */
      if (pd.last_state != pd.task_state)
      {
        pd.flags |= PD_FLAGS_CHECK_VCONN_STATE;
        pd_transition_voltage(pd.requested_idx);
        set_state_timeout(get_time().val + PD_POWER_SUPPLY_TURN_ON_DELAY, PD_STATE_SRC_TRANSITION);
      }
      break;
    case PD_STATE_SRC_TRANSITION:
      /* the voltage output is good, notify the source */
      res = send_control(PD_CTRL_PS_RDY);
      if (res >= 0)
      {
        timeout = 10 * MSEC_US;

        /*
         * Give the sink some time to send any messages
         * before we may send messages of our own.  Add
         * some jitter of up to ~192ms, to prevent
         * multiple collisions. This delay also allows
         * the sink device to request power role swap
         * and allow the the accept message to be sent
         * prior to CMD_DISCOVER_IDENT being sent in the
         * SRC_READY state.
         */
        pd.ready_state_holdoff_timer = get_time().val + SRC_READY_HOLD_OFF_US + (get_time().le.lo & 0xf) * 12 * MSEC_US;

        /* it's time to ping regularly the sink */
        set_state(PD_STATE_SRC_READY);
      }
      else
      {
        /* The sink did not ack, cut the power... */
        set_state(PD_STATE_SRC_DISCONNECTED);
      }
      break;
    case PD_STATE_SRC_READY:
      timeout = PD_T_SOURCE_ACTIVITY;

      /*
       * Don't send any traffic yet until our holdoff timer
       * has expired.  Some devices are chatty once we reach
       * the SRC_READY state and we may end up in a collision
       * of messages if we try to immediately send our
       * interrogations.
       */
      if (get_time().val <= pd.ready_state_holdoff_timer)
        break;

      /*
       * Don't send any PD traffic if we woke up due to
       * incoming packet or if VDO response pending to avoid
       * collisions.
       */
      if (incoming_packet || (pd.vdm_state == VDM_STATE_BUSY))
        break;

      /* Send updated source capabilities to our partner */
      if (pd.flags & PD_FLAGS_UPDATE_SRC_CAPS)
      {
        res = send_source_cap(AMS_START);
        if (res >= 0)
        {
          set_state(PD_STATE_SRC_NEGOCIATE);
          pd.flags &= ~PD_FLAGS_UPDATE_SRC_CAPS;
        }
        break;
      }

      /* Send get sink cap if haven't received it yet */
      if (!(pd.flags & PD_FLAGS_SNK_CAP_RECVD))
      {
        if (++snk_cap_count <= PD_SNK_CAP_RETRIES)
        {
          /* Get sink cap to know if dual-role device */
          send_control(PD_CTRL_GET_SINK_CAP);
          set_state(PD_STATE_SRC_GET_SINK_CAP);
          break;
        }
        else if (debug_level >= 2 && snk_cap_count == PD_SNK_CAP_RETRIES + 1)
        {
          CPRINTF("ERR SNK_CAP\n");
        }
      }

      /* Check power role policy, which may trigger a swap */
      if (pd.flags & PD_FLAGS_CHECK_PR_ROLE)
      {
        pd_check_pr_role(PD_ROLE_SOURCE, pd.flags);
        pd.flags &= ~PD_FLAGS_CHECK_PR_ROLE;
      }

      /* Check data role policy, which may trigger a swap */
      if (pd.flags & PD_FLAGS_CHECK_DR_ROLE)
      {
        pd_check_dr_role(pd.data_role, pd.flags);
        pd.flags &= ~PD_FLAGS_CHECK_DR_ROLE;
        break;
      }

      /* Check for Vconn source, which may trigger a swap */
      if (pd.flags & PD_FLAGS_CHECK_VCONN_STATE)
      {
        /*
         * Ref: Section 2.6.1 of both
         * USB-PD Spec Revision 2.0, Version 1.3 &
         * USB-PD Spec Revision 3.0, Version 2.0
         * During Explicit contract the Sink can
         * initiate or receive a request an exchange
         * of VCONN Source.
         */
        // pd_try_execute_vconn_swap( pd.flags);
        pd.flags &= ~PD_FLAGS_CHECK_VCONN_STATE;
        break;
      }

      /* Send discovery SVDMs last */
      if (pd.data_role == PD_ROLE_DFP && (pd.flags & PD_FLAGS_CHECK_IDENTITY))
      {
#ifndef CONFIG_USB_PD_SIMPLE_DFP
        pd_send_vdm(USB_SID_PD, CMD_DISCOVER_IDENT, NULL, 0);
#endif
        pd.flags &= ~PD_FLAGS_CHECK_IDENTITY;
        break;
      }

      if (!(pd.flags & PD_FLAGS_PING_ENABLED))
        break;

      /* Verify that the sink is alive */
      res = send_control(PD_CTRL_PING);
      if (res >= 0)
        break;

      /* Ping dropped. Try soft reset. */
      set_state(PD_STATE_SOFT_RESET);
      timeout = 10 * MSEC_US;
      break;
    case PD_STATE_SRC_GET_SINK_CAP:
      if (pd.last_state != pd.task_state)
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, PD_STATE_SRC_READY);
      break;
    case PD_STATE_DR_SWAP:
      if (pd.last_state != pd.task_state)
      {
        res = send_control(PD_CTRL_DR_SWAP);
        if (res < 0)
        {
          timeout = 10 * MSEC_US;
          /*
           * If failed to get goodCRC, send
           * soft reset, otherwise ignore
           * failure.
           */
          set_state(res == -1 ? PD_STATE_SOFT_RESET : READY_RETURN_STATE());
          break;
        }
        /* Wait for accept or reject */
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, READY_RETURN_STATE());
      }
      break;
#ifdef CONFIG_USB_PD_DUAL_ROLE
    case PD_STATE_SRC_SWAP_INIT:
      if (pd.last_state != pd.task_state)
      {
        res = send_control(PD_CTRL_PR_SWAP);
        if (res < 0)
        {
          timeout = 10 * MSEC_US;
          /*
           * If failed to get goodCRC, send
           * soft reset, otherwise ignore
           * failure.
           */
          set_state(res == -1 ? PD_STATE_SOFT_RESET : PD_STATE_SRC_READY);
          break;
        }
        /* Wait for accept or reject */
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, PD_STATE_SRC_READY);
      }
      break;
    case PD_STATE_SRC_SWAP_SNK_DISABLE:
      /* Give time for sink to stop drawing current */
      if (pd.last_state != pd.task_state)
        set_state_timeout(get_time().val + PD_T_SINK_TRANSITION, PD_STATE_SRC_SWAP_SRC_DISABLE);
      break;
    case PD_STATE_SRC_SWAP_SRC_DISABLE:
      /* Turn power off */
      if (pd.last_state != pd.task_state)
      {
        pd_power_supply_reset();

        /*
         * Switch to Rd and swap roles to sink
         *
         * The reason we do this as early as possible is
         * to help prevent CC disconnection cases where
         * both partners are applying an Rp.  Certain PD
         * stacks (e.g. qualcomm), reflexively apply
         * their Rp once VBUS falls beneath
         * ~3.67V. (b/77827528).
         */
        tcpm_set_cc(TYPEC_CC_RD);
        pd.power_role = PD_ROLE_SINK;

        /* Inform TCPC of power role update. */
        pd_update_roles();

        set_state_timeout(get_time().val + PD_POWER_SUPPLY_TURN_OFF_DELAY, PD_STATE_SRC_SWAP_STANDBY);
      }
      break;
    case PD_STATE_SRC_SWAP_STANDBY:
      /* Send PS_RDY to let sink know our power is off */
      if (pd.last_state != pd.task_state)
      {
        /* Send PS_RDY */
        res = send_control(PD_CTRL_PS_RDY);
        if (res < 0)
        {
          timeout = 10 * MSEC_US;
          set_state(PD_STATE_SRC_DISCONNECTED);
          break;
        }
        /* Wait for PS_RDY from new source */
        set_state_timeout(get_time().val + PD_T_PS_SOURCE_ON, PD_STATE_SNK_DISCONNECTED);
      }
      break;
    case PD_STATE_SUSPENDED:
      {
#ifndef CONFIG_USB_PD_TCPC
        int rstatus;
#endif
        CPRINTS("TCPC suspended!");
        pd.req_suspend_state = 0;
#ifdef CONFIG_USB_PD_TCPC
        pd_rx_disable_monitoring();
        pd_hw_release();
        pd_power_supply_reset();
#else
        pd_power_supply_reset();
#ifdef CONFIG_USBC_VCONN
        set_vconn(0);
#endif
        rstatus = tcpm_release();
        if (rstatus != 0 && rstatus != EC_ERROR_UNIMPLEMENTED)
          CPRINTS("TCPC release failed!");
#endif
        /* Drain any outstanding software message queues. */
        tcpm_clear_pending_messages();

        /* Wait for resume */
        while (pd.task_state == PD_STATE_SUSPENDED)
        {
#ifdef CONFIG_USB_PD_ALT_MODE_DFP
          int evt = task_wait_event(-1);

          if (evt & PD_EVENT_SYSJUMP)
            /* Nothing to do for sysjump prep */
            notify_sysjump_ready();
#else
          task_wait_event(-1);
#endif
        }

#ifdef CONFIG_USB_PD_TCPC
        pd_hw_init(PD_ROLE_DEFAULT());
        tcpc_prints("resumed!", port);
#else
        if (rstatus != EC_ERROR_UNIMPLEMENTED && pd_restart_tcpc() != 0)
        {
          /* stay in PD_STATE_SUSPENDED */
          CPRINTS("TCPC restart failed!");
          break;
        }
        /* Set the CC termination and state back to default */
        tcpm_set_cc(PD_ROLE_DEFAULT() == PD_ROLE_SOURCE ? TYPEC_CC_RP : TYPEC_CC_RD);
        set_state(PD_DEFAULT_STATE());
        // TODO: not standard : do not send get source capabilities here
        send_control(PD_CTRL_GET_SOURCE_CAP);
        // ENDOFTODO
        CPRINTS("TCPC resumed!");
#endif
        break;
      }
    case PD_STATE_SNK_DISCONNECTED:
#ifdef CONFIG_USB_PD_LOW_POWER
      timeout = (drp_state != PD_DRP_TOGGLE_ON ? SECOND_US : 10 * MSEC_US);
#else
      timeout = 10 * MSEC_US;
#endif

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
      /*
       * If SW decided we should be in a low power state and
       * the CC lines did not change, then don't talk with the
       * TCPC otherwise we might wake it up.
       */
      if (pd.flags & PD_FLAGS_LPM_REQUESTED && !(evt & PD_EVENT_CC))
        break;
#endif /* CONFIG_USB_PD_TCPC_LOW_POWER */

      tcpm_get_cc(&cc1, &cc2);

#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
      /*
       * Attempt TCPC auto DRP toggle if it is not already
       * auto toggling and not try.src, and dual role toggling
       * is allowed.
       */
      if (auto_toggle_supported && !(pd.flags & PD_FLAGS_TCPC_DRP_TOGGLE) && !is_try_src() && cc_is_open(cc1, cc2) &&
          (drp_state == PD_DRP_TOGGLE_ON))
      {
        set_state(PD_STATE_DRP_AUTO_TOGGLE);
        timeout = 2 * MSEC_US;
        break;
      }
#endif

      /* Source connection monitoring */
      if (!cc_is_open(cc1, cc2))
      {
        pd.cc_state = PD_CC_NONE;
        hard_reset_count = 0;
        new_cc_state = PD_CC_NONE;
        pd.cc_debounce = get_time().val + PD_T_CC_DEBOUNCE;
        set_state(PD_STATE_SNK_DISCONNECTED_DEBOUNCE);
        timeout = 10 * MSEC_US;
        break;
      }

      /*
       * If Try.SRC is active and failed to detect a SNK,
       * then it transitions to TryWait.SNK. Need to prevent
       * normal dual role toggle until tDRPTryWait timer
       * expires.
       */
      if (pd.flags & PD_FLAGS_TRY_SRC)
      {
        if (get_time().val > pd.try_src_marker)
          pd.flags &= ~PD_FLAGS_TRY_SRC;
        break;
      }

      /* If no source detected, check for role toggle. */
      if (drp_state == PD_DRP_TOGGLE_ON && get_time().val >= next_role_swap)
      {
        /* Swap roles to source */
        pd_set_power_role(PD_ROLE_SOURCE);
        set_state(PD_STATE_SRC_DISCONNECTED);
        tcpm_set_cc(TYPEC_CC_RP);
        next_role_swap = get_time().val + PD_T_DRP_SRC;

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
        /*
         * Clear low power mode flag as we are swapping
         * states quickly.
         */
        pd.flags &= ~PD_FLAGS_LPM_REQUESTED;
#endif

        /* Swap states quickly */
        timeout = 2 * MSEC_US;
        break;
      }

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
      /*
       * If we are remaining in the SNK_DISCONNECTED state,
       * let's go into low power mode and wait for a change on
       * CC status.
       */
      pd.flags |= PD_FLAGS_LPM_REQUESTED;
#endif /* CONFIG_USB_PD_TCPC_LOW_POWER */
      break;

    case PD_STATE_SNK_DISCONNECTED_DEBOUNCE:
      tcpm_get_cc(&cc1, &cc2);

      if (cc_is_rp(cc1) && cc_is_rp(cc2))
      {
        /* Debug accessory */
        new_cc_state = PD_CC_DFP_DEBUG_ACC;
      }
      else if (cc_is_rp(cc1) || cc_is_rp(cc2))
      {
        new_cc_state = PD_CC_DFP_ATTACHED;
      }
      else
      {
        /* No connection any more */
        set_state(PD_STATE_SNK_DISCONNECTED);
        timeout = 5 * MSEC_US;
        break;
      }

      timeout = 20 * MSEC_US;

      /* Debounce the cc state */
      if (new_cc_state != pd.cc_state)
      {
        pd.cc_debounce = get_time().val + PD_T_CC_DEBOUNCE;
        pd.cc_state = new_cc_state;
        break;
      }
      /* Wait for CC debounce and VBUS present */
      if (get_time().val < pd.cc_debounce || !pd_is_vbus_present())
        break;

      if (pd_try_src_enable && !(pd.flags & PD_FLAGS_TRY_SRC))
      {
        /*
         * If TRY_SRC is enabled, but not active,
         * then force attempt to connect as source.
         */
        pd.try_src_marker = get_time().val + PD_T_DRP_TRY;
        pd.try_timeout = get_time().val + PD_T_TRY_TIMEOUT;
        /* Swap roles to source */
        pd_set_power_role(PD_ROLE_SOURCE);
        tcpm_set_cc(TYPEC_CC_RP);
        timeout = 2 * MSEC_US;
        set_state(PD_STATE_SRC_DISCONNECTED);
        /* Set flag after the state change */
        pd.flags |= PD_FLAGS_TRY_SRC;
        break;
      }

      /* We are attached */
#ifdef CONFIG_COMMON_RUNTIME
      hook_notify(HOOK_USB_PD_CONNECT);
#endif
      pd.polarity = get_snk_polarity(cc1, cc2);
      pd_set_polarity(pd.polarity);
      /* reset message ID  on connection */
      pd.msg_id = 0;
      /* initial data role for sink is UFP */
      pd_set_data_role(PD_ROLE_UFP);
      /* Enable Auto Discharge Disconnect */
      tcpm_enable_auto_discharge_disconnect(1);
#if defined(CONFIG_CHARGE_MANAGER)
      typec_curr = usb_get_typec_current_limit(pd.polarity, cc1, cc2);
      typec_set_input_current_limit(typec_curr, TYPE_C_VOLTAGE);
#endif

#ifdef CONFIG_USBC_PPC
      /* Inform PPC that a source is connected. */
      ppc_dev_is_connected(PPC_DEV_SRC);
#endif /* CONFIG_USBC_PPC */
#ifdef CONFIG_USBC_OCP
      usbc_ocp_snk_is_connected(false);
#endif
      /* If PD comm is enabled, enable TCPC RX */
      if (pd_comm_is_enabled())
        tcpm_set_rx_enable(1);

      /* DFP is attached */
      if (new_cc_state == PD_CC_DFP_ATTACHED || new_cc_state == PD_CC_DFP_DEBUG_ACC)
      {
        pd.flags |= PD_FLAGS_CHECK_PR_ROLE | PD_FLAGS_CHECK_DR_ROLE | PD_FLAGS_CHECK_IDENTITY;
        /* Reset cable attributes and flags */
        reset_pd_cable();

        if (new_cc_state == PD_CC_DFP_DEBUG_ACC)
          pd.flags |= PD_FLAGS_TS_DTS_PARTNER;
        set_state(PD_STATE_SNK_DISCOVERY);
        timeout = 10 * MSEC_US;
        // hook_call_deferred(&pd_usb_billboard_deferred_data, PD_T_AME);
      }
      break;
    case PD_STATE_SNK_HARD_RESET_RECOVER:
      if (pd.last_state != pd.task_state)
        pd.flags |= PD_FLAGS_CHECK_IDENTITY;
#ifdef CONFIG_USB_PD_VBUS_DETECT_NONE
      /*
       * Can't measure vbus state so this is the maximum
       * recovery time for the source.
       */
      if (pd.last_state != pd.task_state)
        set_state_timeout(get_time().val + PD_T_SAFE_0V + PD_T_SRC_RECOVER_MAX + PD_T_SRC_TURN_ON,
                          PD_STATE_SNK_DISCONNECTED);
#else
      /* Wait for VBUS to go low and then high*/
      if (pd.last_state != pd.task_state)
      {
        snk_hard_reset_vbus_off = 0;
        set_state_timeout(get_time().val + PD_T_SAFE_0V,
                          hard_reset_count < PD_HARD_RESET_COUNT ? PD_STATE_HARD_RESET_SEND : PD_STATE_SNK_DISCOVERY);
      }

      if (!pd_is_vbus_present() && !snk_hard_reset_vbus_off)
      {
        /* VBUS has gone low, reset timeout */
        snk_hard_reset_vbus_off = 1;
        set_state_timeout(get_time().val + PD_T_SRC_RECOVER_MAX + PD_T_SRC_TURN_ON, PD_STATE_SNK_DISCONNECTED);
      }
      if (pd_is_vbus_present() && snk_hard_reset_vbus_off)
      {
        /* VBUS went high again */
        set_state(PD_STATE_SNK_DISCOVERY);
        timeout = 10 * MSEC_US;
      }

      /*
       * Don't need to set timeout because VBUS changing
       * will trigger an interrupt and wake us up.
       */
#endif
      break;
    case PD_STATE_SNK_DISCOVERY:
      /* Wait for source cap expired only if we are enabled */
      if ((pd.last_state != pd.task_state) && pd_comm_is_enabled())
      {
#ifdef CONFIG_USB_PD_TCPM_TCPCI
        /*
         * If we come from hard reset recover state,
         * then we can process the source capabilities
         * form partner now, so enable PHY layer
         * receiving function.
         */
        if (pd.last_state == PD_STATE_SNK_HARD_RESET_RECOVER)
          tcpm_set_rx_enable(1);
#endif /* CONFIG_USB_PD_TCPM_TCPCI */
#ifdef CONFIG_USB_PD_RESET_MIN_BATT_SOC
        /*
         * If the battery has not met a configured safe
         * level for hard resets, refrain from starting
         * reset timers as a hard reset could brown out
         * the board.  Note this may mean that
         * high-power chargers will stay at 15W until a
         * reset is sent, depending on boot timing.
         */
        int batt_soc = usb_get_battery_soc();

        if (batt_soc < CONFIG_USB_PD_RESET_MIN_BATT_SOC || battery_get_disconnect_state() != BATTERY_NOT_DISCONNECTED)
          pd.flags |= PD_FLAGS_SNK_WAITING_BATT;
        else
          pd.flags &= ~PD_FLAGS_SNK_WAITING_BATT;
#endif

        if (pd.flags & PD_FLAGS_SNK_WAITING_BATT)
        {
#ifdef CONFIG_CHARGE_MANAGER
          /*
           * Configure this port as dedicated for
           * now, so it won't be de-selected by
           * the charge manager leaving safe mode.
           */
          // TODO charge_manager_update_dualrole( CAP_DEDICATED);
#endif
          CPRINTS("Battery low. "
                  "Hold reset timer");
          /*
           * If VBUS has never been low, and we timeout
           * waiting for source cap, try a soft reset
           * first, in case we were already in a stable
           * contract before this boot.
           */
        }
        else if (pd.flags & PD_FLAGS_VBUS_NEVER_LOW)
        {
          set_state_timeout(get_time().val + PD_T_SINK_WAIT_CAP, PD_STATE_SOFT_RESET);
          /*
           * If we haven't passed hard reset counter,
           * start SinkWaitCapTimer, otherwise start
           * NoResponseTimer.
           */
        }
        else if (hard_reset_count < PD_HARD_RESET_COUNT)
        {
          set_state_timeout(get_time().val + PD_T_SINK_WAIT_CAP, PD_STATE_HARD_RESET_SEND);
        }
        else if (pd_capable())
        {
          /* ErrorRecovery */
          set_state_timeout(get_time().val + PD_T_NO_RESPONSE, PD_STATE_SNK_DISCONNECTED);
        }
#if defined(CONFIG_CHARGE_MANAGER)
        /*
         * If we didn't come from disconnected, must
         * have come from some path that did not set
         * typec current limit. So, set to 0 so that
         * we guarantee this is revised below.
         */
        if (pd.last_state != PD_STATE_SNK_DISCONNECTED_DEBOUNCE)
          typec_curr = 0;
#endif
      }

#if defined(CONFIG_CHARGE_MANAGER)
      timeout = PD_T_SINK_ADJ - PD_T_DEBOUNCE;

      /* Check if CC pull-up has changed */
      tcpm_get_cc(&cc1, &cc2);
      if (typec_curr != usb_get_typec_current_limit(pd.polarity, cc1, cc2))
      {
        /* debounce signal by requiring two reads */
        if (typec_curr_change)
        {
          /* set new input current limit */
          typec_curr = usb_get_typec_current_limit(pd.polarity, cc1, cc2);
          typec_set_input_current_limit(typec_curr, TYPE_C_VOLTAGE);
        }
        else
        {
          /* delay for debounce */
          timeout = PD_T_DEBOUNCE;
        }
        typec_curr_change = !typec_curr_change;
      }
      else
      {
        typec_curr_change = 0;
      }
#endif
      break;
    case PD_STATE_SNK_REQUESTED:
      /* Wait for ACCEPT or REJECT */
      if (pd.last_state != pd.task_state)
      {
        pd.flags |= PD_FLAGS_CHECK_VCONN_STATE;
        hard_reset_count = 0;
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, PD_STATE_HARD_RESET_SEND);
      }
      break;
    case PD_STATE_SNK_TRANSITION:
      /* Wait for PS_RDY */
      if (pd.last_state != pd.task_state)
        set_state_timeout(get_time().val + PD_T_PS_TRANSITION, PD_STATE_HARD_RESET_SEND);
      break;
    case PD_STATE_SNK_READY:
      timeout = 20 * MSEC_US;

      /*
       * Don't send any traffic yet until our holdoff timer
       * has expired.  Some devices are chatty once we reach
       * the SNK_READY state and we may end up in a collision
       * of messages if we try to immediately send our
       * interrogations.
       */
      if (get_time().val <= pd.ready_state_holdoff_timer)
        break;

      /*
       * Don't send any PD traffic if we woke up due to
       * incoming packet or if VDO response pending to avoid
       * collisions.
       */
      if (incoming_packet || (pd.vdm_state == VDM_STATE_BUSY))
        break;

      /* Check for new power to request */
      if (pd.new_power_request)
      {
        if (pd_send_request_msg(0) != EC_SUCCESS)
          set_state(PD_STATE_SOFT_RESET);
        break;
      }

      /* Check power role policy, which may trigger a swap */
      if (pd.flags & PD_FLAGS_CHECK_PR_ROLE)
      {
        pd_check_pr_role(PD_ROLE_SINK, pd.flags);
        pd.flags &= ~PD_FLAGS_CHECK_PR_ROLE;
        break;
      }

      /* Check data role policy, which may trigger a swap */
      if (pd.flags & PD_FLAGS_CHECK_DR_ROLE)
      {
        pd_check_dr_role(pd.data_role, pd.flags);
        pd.flags &= ~PD_FLAGS_CHECK_DR_ROLE;
        break;
      }

      /* Check for Vconn source, which may trigger a swap */
      if (pd.flags & PD_FLAGS_CHECK_VCONN_STATE)
      {
        /*
         * Ref: Section 2.6.2 of both
         * USB-PD Spec Revision 2.0, Version 1.3 &
         * USB-PD Spec Revision 3.0, Version 2.0
         * During Explicit contract the Sink can
         * initiate or receive a request an exchange
         * of VCONN Source.
         */
        // pd_try_execute_vconn_swap( pd.flags);
        pd.flags &= ~PD_FLAGS_CHECK_VCONN_STATE;
        break;
      }

      /* If DFP, send discovery SVDMs */
      if (pd.data_role == PD_ROLE_DFP && (pd.flags & PD_FLAGS_CHECK_IDENTITY))
      {
        pd_send_vdm(USB_SID_PD, CMD_DISCOVER_IDENT, NULL, 0);
        pd.flags &= ~PD_FLAGS_CHECK_IDENTITY;
        break;
      }

      /* Sent all messages, don't need to wake very often */
      timeout = 200 * MSEC_US;
      break;
    case PD_STATE_SNK_SWAP_INIT:
      if (pd.last_state != pd.task_state)
      {
        res = send_control(PD_CTRL_PR_SWAP);
        if (res < 0)
        {
          timeout = 10 * MSEC_US;
          /*
           * If failed to get goodCRC, send
           * soft reset, otherwise ignore
           * failure.
           */
          set_state(res == -1 ? PD_STATE_SOFT_RESET : PD_STATE_SNK_READY);
          break;
        }
        /* Wait for accept or reject */
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, PD_STATE_SNK_READY);
      }
      break;
    case PD_STATE_SNK_SWAP_SNK_DISABLE:
      /* Stop drawing power */
      pd_set_input_current_limit(0, 0);
#ifdef CONFIG_CHARGE_MANAGER
      typec_set_input_current_limit(0, 0);
      // charge_manager_set_ceil(
      //			CEIL_REQUESTOR_PD,
      //			CHARGE_CEIL_NONE);
#endif
      set_state(PD_STATE_SNK_SWAP_SRC_DISABLE);
      timeout = 10 * MSEC_US;
      break;
    case PD_STATE_SNK_SWAP_SRC_DISABLE:
      /* Wait for PS_RDY */
      if (pd.last_state != pd.task_state)
        set_state_timeout(get_time().val + PD_T_PS_SOURCE_OFF, PD_STATE_HARD_RESET_SEND);
      break;
    case PD_STATE_SNK_SWAP_STANDBY:
      if (pd.last_state != pd.task_state)
      {
        /* Switch to Rp and enable power supply */
        tcpm_set_cc(TYPEC_CC_RP);
        if (pd_set_power_supply_ready())
        {
          /* Restore Rd */
          tcpm_set_cc(TYPEC_CC_RD);
          timeout = 10 * MSEC_US;
          set_state(PD_STATE_SNK_DISCONNECTED);
          break;
        }
        /* Wait for power supply to turn on */
        set_state_timeout(get_time().val + PD_POWER_SUPPLY_TURN_ON_DELAY, PD_STATE_SNK_SWAP_COMPLETE);
      }
      break;
    case PD_STATE_SNK_SWAP_COMPLETE:
      /* Send PS_RDY and change to source role */
      res = send_control(PD_CTRL_PS_RDY);
      if (res < 0)
      {
        /* Restore Rd */
        tcpm_set_cc(TYPEC_CC_RD);
        pd_power_supply_reset();
        timeout = 10 * MSEC_US;
        set_state(PD_STATE_SNK_DISCONNECTED);
        break;
      }

      /* Don't send GET_SINK_CAP on swap */
      snk_cap_count = PD_SNK_CAP_RETRIES + 1;
      caps_count = 0;
      pd.msg_id = 0;
      pd_set_power_role(PD_ROLE_SOURCE);
      pd_update_roles();
      set_state(PD_STATE_SRC_DISCOVERY);
      timeout = 10 * MSEC_US;
      break;
#ifdef CONFIG_USBC_VCONN_SWAP
    case PD_STATE_VCONN_SWAP_SEND:
      if (pd.last_state != pd.task_state)
      {
        res = send_control(PD_CTRL_VCONN_SWAP);
        if (res < 0)
        {
          timeout = 10 * MSEC_US;
          /*
           * If failed to get goodCRC, send
           * soft reset, otherwise ignore
           * failure.
           */
          set_state(res == -1 ? PD_STATE_SOFT_RESET : READY_RETURN_STATE());
          break;
        }
        /* Wait for accept or reject */
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, READY_RETURN_STATE());
      }
      break;
    case PD_STATE_VCONN_SWAP_INIT:
      if (pd.last_state != pd.task_state)
      {
        if (!(pd.flags & PD_FLAGS_VCONN_ON))
        {
          /* Turn VCONN on and wait for it */
          tcpm_set_vconn(1);
          set_state_timeout(get_time().val + PD_VCONN_SWAP_DELAY, PD_STATE_VCONN_SWAP_READY);
        }
        else
        {
          set_state_timeout(get_time().val + PD_T_VCONN_SOURCE_ON, READY_RETURN_STATE());
        }
      }
      break;
    case PD_STATE_VCONN_SWAP_READY:
      if (pd.last_state != pd.task_state)
      {
        if (!(pd.flags & PD_FLAGS_VCONN_ON))
        {
          /* VCONN is now on, send PS_RDY */
          pd.flags |= PD_FLAGS_VCONN_ON;
          res = send_control(PD_CTRL_PS_RDY);
          if (res == -1)
          {
            timeout = 10 * MSEC_US;
            /*
             * If failed to get goodCRC,
             * send soft reset
             */
            set_state(PD_STATE_SOFT_RESET);
            break;
          }
          set_state(READY_RETURN_STATE());
        }
        else
        {
          /* Turn VCONN off and wait for it */
          tcpm_set_vconn(0);
          pd.flags &= ~PD_FLAGS_VCONN_ON;
          set_state_timeout(get_time().val + PD_VCONN_SWAP_DELAY, READY_RETURN_STATE());
        }
      }
      break;
#endif /* CONFIG_USBC_VCONN_SWAP */
#endif /* CONFIG_USB_PD_DUAL_ROLE */
    case PD_STATE_SOFT_RESET:
      if (pd.last_state != pd.task_state)
      {
        /* Message ID of soft reset is always 0 */
        invalidate_last_message_id();
        pd.msg_id = 0;
        res = send_control(PD_CTRL_SOFT_RESET);

        /* if soft reset failed, try hard reset. */
        if (res < 0)
        {
          set_state(PD_STATE_HARD_RESET_SEND);
          timeout = 5 * MSEC_US;
          break;
        }

        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, PD_STATE_HARD_RESET_SEND);
      }
      break;
    case PD_STATE_HARD_RESET_SEND:
      hard_reset_count++;
      if (pd.last_state != pd.task_state)
      {
        hard_reset_sent = 0;
        pd.hard_reset_complete_timer = 0;
      }
#ifdef CONFIG_CHARGE_MANAGER
      if (pd.last_state == PD_STATE_SNK_DISCOVERY ||
          (pd.last_state == PD_STATE_SOFT_RESET && (pd.flags & PD_FLAGS_VBUS_NEVER_LOW)))
      {
        pd.flags &= ~PD_FLAGS_VBUS_NEVER_LOW;
        /*
         * If discovery timed out, assume that we
         * have a dedicated charger attached. This
         * may not be a correct assumption according
         * to the specification, but it generally
         * works in practice and the harmful
         * effects of a wrong assumption here
         * are minimal.
         */
        // TODO charge_manager_update_dualrole( CAP_DEDICATED);
      }
#endif
      if (hard_reset_sent)
        break;

      if (pd_transmit(TCPC_TX_HARD_RESET, 0, NULL, AMS_START) < 0)
      {
        /*
         * likely a non-idle channel
         * TCPCI r2.0 v1.0 4.4.15:
         * the TCPC does not retry HARD_RESET
         * but we can try periodically until the timer
         * expires.
         */
        now = get_time();
        if (pd.hard_reset_complete_timer == 0)
        {
          pd.hard_reset_complete_timer = now.val + PD_T_HARD_RESET_COMPLETE;
          timeout = PD_T_HARD_RESET_RETRY;
          break;
        }
        if (now.val < pd.hard_reset_complete_timer)
        {
          CPRINTS("Retrying hard reset");
          timeout = PD_T_HARD_RESET_RETRY;
          break;
        }
        /*
         * PD 2.0 spec, section 6.5.11.1
         * Pretend TX_HARD_RESET succeeded after
         * timeout.
         */
      }

      hard_reset_sent = 1;
      /*
       * If we are source, delay before cutting power
       * to allow sink time to get hard reset.
       */
      if (pd.power_role == PD_ROLE_SOURCE)
      {
        set_state_timeout(get_time().val + PD_T_PS_HARD_RESET, PD_STATE_HARD_RESET_EXECUTE);
      }
      else
      {
        set_state(PD_STATE_HARD_RESET_EXECUTE);
        timeout = 10 * MSEC_US;
      }
      break;
    case PD_STATE_HARD_RESET_EXECUTE:
#ifdef CONFIG_USB_PD_DUAL_ROLE
      /*
       * If hard reset while in the last stages of power
       * swap, then we need to restore our CC resistor.
       */
      if (pd.last_state == PD_STATE_SNK_SWAP_STANDBY)
        tcpm_set_cc(TYPEC_CC_RD);
#endif
      CPRINTF("HARD RST execute\n", pd.last_state);

      /* reset our own state machine */
      pd_execute_hard_reset();
      timeout = 10 * MSEC_US;
      break;
#ifdef CONFIG_COMMON_RUNTIME
    case PD_STATE_BIST_RX:
      send_bist_cmd();
      /* Delay at least enough for partner to finish BIST */
      timeout = PD_T_BIST_RECEIVE + 20 * MSEC_US;
      /* Set to appropriate port disconnected state */
      set_state(DUAL_ROLE_IF_ELSE(PD_STATE_SNK_DISCONNECTED, PD_STATE_SRC_DISCONNECTED));
      break;
    case PD_STATE_BIST_TX:
      pd_transmit(TCPC_TX_BIST_MODE_2, 0, NULL);
      /* Delay at least enough to finish sending BIST */
      timeout = PD_T_BIST_TRANSMIT + 20 * MSEC_US;
      /* Set to appropriate port disconnected state */
      set_state(DUAL_ROLE_IF_ELSE(PD_STATE_SNK_DISCONNECTED, PD_STATE_SRC_DISCONNECTED));
      break;
#endif
#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
    case PD_STATE_DRP_AUTO_TOGGLE:
      {
        enum pd_drp_next_states next_state;

        assert(auto_toggle_supported);

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
        /*
         * If SW decided we should be in a low power state and
         * the CC lines did not change, then don't talk with the
         * TCPC otherwise we might wake it up.
         */
        if (pd.flags & PD_FLAGS_LPM_REQUESTED && !(evt & PD_EVENT_CC))
          break;

        /*
         * Debounce low power mode exit.  Some TCPCs need time
         * for the CC_STATUS register to be stable after exiting
         * low power mode.
         */
        if (pd.flags & PD_FLAGS_LPM_EXIT)
        {
          uint64_t now;

          now = get_time().val;
          if (now < pd.low_power_exit_time)
            break;

          CPRINTS("TCPC Exit Low Power Mode done");
          pd.flags &= ~PD_FLAGS_LPM_EXIT;
        }
#endif

        /*
         * Check for connection
         *
         * Send FALSE for supports_auto_toggle to not change
         * the current return value of UNATTACHED instead of
         * the auto-toggle ATTACHED_WAIT response for TCPMv1.
         */
        tcpm_get_cc(&cc1, &cc2);

        next_state = drp_auto_toggle_next_state(&pd.drp_sink_time, pd.power_role, drp_state, cc1, cc2, 1);

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
        /*
         * The next state is not determined just by what is
         * attached, but also depends on DRP_STATE. Regardless
         * of next state, if nothing is attached, then always
         * request low power mode.
         */
        if (cc_is_open(cc1, cc2))
          pd.flags |= PD_FLAGS_LPM_REQUESTED;
#endif
        if (next_state == DRP_TC_DEFAULT)
        {
          if (PD_DEFAULT_STATE() == PD_STATE_SNK_DISCONNECTED)
            next_state = DRP_TC_UNATTACHED_SNK;
          else
            next_state = DRP_TC_UNATTACHED_SRC;
        }

        if (next_state == DRP_TC_UNATTACHED_SNK)
        {
          /*
           * The TCPCI comes out of auto toggle with
           * a prospective connection.  It is expecting
           * us to set the CC lines to what it is
           * thinking is best or it goes direct back to
           * unattached.  So get the SNK polarity to
           * be able to setup the CC lines to avoid this.
           */
          pd.polarity = get_snk_polarity(cc1, cc2);

          tcpm_set_cc(TYPEC_CC_RD);
          pd_set_power_role(PD_ROLE_SINK);
          timeout = 2 * MSEC_US;
          set_state(PD_STATE_SNK_DISCONNECTED);
        }
        else if (next_state == DRP_TC_UNATTACHED_SRC)
        {
          /*
           * The TCPCI comes out of auto toggle with
           * a prospective connection.  It is expecting
           * us to set the CC lines to what it is
           * thinking is best or it goes direct back to
           * unattached.  So get the SNK polarity to
           * be able to setup the CC lines to avoid this.
           */
          pd.polarity = get_src_polarity(cc1, cc2);

          tcpm_set_cc(TYPEC_CC_RP);
          pd_set_power_role(PD_ROLE_SOURCE);
          timeout = 2 * MSEC_US;
          set_state(PD_STATE_SRC_DISCONNECTED);
        }
        else
        {
          /*
           * We are staying in PD_STATE_DRP_AUTO_TOGGLE,
           * therefore enable auto-toggle.
           */
          tcpm_enable_drp_toggle();
          pd.flags |= PD_FLAGS_TCPC_DRP_TOGGLE;
          set_state(PD_STATE_DRP_AUTO_TOGGLE);
        }

        break;
      }
#endif
    case PD_STATE_ENTER_USB:
      if (pd.last_state != pd.task_state)
      {
        set_state_timeout(get_time().val + PD_T_SENDER_RESPONSE, READY_RETURN_STATE());
      }
      break;
    default:
      break;
  }

  pd.last_state = this_state;

  /*
   * Check for state timeout, and if not check if need to adjust
   * timeout value to wake up on the next state timeout.
   */
  now = get_time();
  if (pd.timeout)
  {
    if (now.val >= pd.timeout)
    {
      set_state(pd.timeout_state);
      /* On a state timeout, run next state soon */
      timeout = timeout < 10 * MSEC_US ? timeout : 10 * MSEC_US;
    }
    else if (pd.timeout - now.val < timeout)
    {
      timeout = pd.timeout - now.val;
    }
  }

#ifdef CONFIG_USB_PD_TCPC_LOW_POWER
  /* Determine if we need to put the TCPC in low power mode */
  if (pd.flags & PD_FLAGS_LPM_REQUESTED && !(pd.flags & PD_FLAGS_LPM_ENGAGED))
  {
    int64_t time_left;

    /* If any task prevents LPM, wait another debounce */
    if (pd.tasks_preventing_lpm)
    {
      pd.low_power_time = PD_LPM_DEBOUNCE_US + now.val;
    }

    time_left = pd.low_power_time - now.val;
    if (time_left <= 0)
    {
      pd.flags |= PD_FLAGS_LPM_ENGAGED;
      pd.flags |= PD_FLAGS_LPM_TRANSITION;
      tcpm_enter_low_power_mode();
      pd.flags &= ~PD_FLAGS_LPM_TRANSITION;
      timeout = -1;
    }
    else if (timeout < 0 || timeout > time_left)
    {
      timeout = time_left;
    }
  }
#endif

  /* Check for disconnection if we're connected */
  if (!pd_is_connected())
    return;
#ifdef CONFIG_USB_PD_DUAL_ROLE
  if (pd_is_power_swapping())
    return;
#endif
  if (pd.power_role == PD_ROLE_SOURCE)
  {
    /* Source: detect disconnect by monitoring CC */
    tcpm_get_cc(&cc1, &cc2);
    if (pd.polarity)
      cc1 = cc2;
    if (cc1 == TYPEC_CC_VOLT_OPEN)
    {
      set_state(PD_STATE_SRC_DISCONNECTED);
      /* Debouncing */
      timeout = 10 * MSEC_US;
#ifdef CONFIG_USB_PD_DUAL_ROLE
      /*
       * If Try.SRC is configured, then ATTACHED_SRC
       * needs to transition to TryWait.SNK. Change
       * power role to SNK and start state timer.
       */
      if (pd_try_src_enable)
      {
        /* Swap roles to sink */
        pd_set_power_role(PD_ROLE_SINK);
        tcpm_set_cc(TYPEC_CC_RD);
        /* Set timer for TryWait.SNK state */
        pd.try_src_marker = get_time().val + PD_T_DEBOUNCE;
        /* Advance to TryWait.SNK state */
        set_state(PD_STATE_SNK_DISCONNECTED);
        /* Mark state as TryWait.SNK */
        pd.flags |= PD_FLAGS_TRY_SRC;
      }
#endif
    }
  }
#ifdef CONFIG_USB_PD_DUAL_ROLE
  /*
   * Sink disconnect if VBUS is low and
   *  1) we are not waiting for VBUS to debounce after a power
   *     role swap.
   *  2) we are not recovering from a hard reset.
   */
  if (pd.power_role == PD_ROLE_SINK && pd.vbus_debounce_time < get_time().val && !pd_is_vbus_present() &&
      pd.task_state != PD_STATE_SNK_HARD_RESET_RECOVER && pd.task_state != PD_STATE_HARD_RESET_EXECUTE)
  {
    /* Sink: detect disconnect by monitoring VBUS */
    set_state(PD_STATE_SNK_DISCONNECTED);
    /* set timeout small to reconnect fast */
    timeout = 5 * MSEC_US;
  }
#endif /* CONFIG_USB_PD_DUAL_ROLE */
}

int pd_is_port_enabled()
{
  switch (pd.task_state)
  {
    case PD_STATE_DISABLED:
    case PD_STATE_SUSPENDED:
      return 0;
    default:
      return 1;
  }
}

/*
 * (enable=1) request pd_task transition to the suspended state.  hang
 * around for a while until we observe the state change.  this can
 * take a while (like 300ms) on startup when pd_task is sleeping in
 * tcpci_tcpm_init.
 *
 * (enable=0) force pd_task out of the suspended state and into the
 * port's default state.
 */

void pd_set_suspend(int enable)
{
  int tries = 300;

  if (enable)
  {
    pd.req_suspend_state = 1;
    do
    {
      task_wake();
      if (pd.task_state == PD_STATE_SUSPENDED)
        break;
      msleep(1);
    } while (--tries != 0);
    if (!tries)
      CPRINTS("TCPC set_suspend failed!");
  }
  else
  {
    if (pd.task_state != PD_STATE_SUSPENDED)
      CPRINTS("TCPC suspend disable request "
              "while not suspended!");
    set_state(PD_DEFAULT_STATE());

    task_wake();
  }
}

#if defined(CONFIG_CMD_PD) && defined(CONFIG_CMD_PD_FLASH)
static int hex8tou32(char* str, uint32_t* val)
{
  char* ptr = str;
  uint32_t tmp = 0;

  while (*ptr)
  {
    char c = *ptr++;
    if (c >= '0' && c <= '9')
      tmp = (tmp << 4) + (c - '0');
    else if (c >= 'A' && c <= 'F')
      tmp = (tmp << 4) + (c - 'A' + 10);
    else if (c >= 'a' && c <= 'f')
      tmp = (tmp << 4) + (c - 'a' + 10);
    else
      return EC_ERROR_INVAL;
  }
  if (ptr != str + 8)
    return EC_ERROR_INVAL;
  *val = tmp;
  return EC_SUCCESS;
}

static int remote_flashing(int argc, char** argv)
{
  int cnt, cmd;
  uint32_t data[VDO_MAX_SIZE - 1];
  char* e;
  static int flash_offset;

  if (argc < 4 || argc > (VDO_MAX_SIZE + 4 - 1))
    return EC_ERROR_PARAM_COUNT;

  port = strtoi(argv[1], &e, 10);
  if (*e || port >= 1)
    return EC_ERROR_PARAM2;

  cnt = 0;
  if (!strcasecmp(argv[3], "erase"))
  {
    cmd = VDO_CMD_FLASH_ERASE;
    flash_offset[port] = 0;
    ccprintf("ERASE ...");
  }
  else if (!strcasecmp(argv[3], "reboot"))
  {
    cmd = VDO_CMD_REBOOT;
    ccprintf("REBOOT ...");
  }
  else if (!strcasecmp(argv[3], "signature"))
  {
    cmd = VDO_CMD_ERASE_SIG;
    ccprintf("ERASE SIG ...");
  }
  else if (!strcasecmp(argv[3], "info"))
  {
    cmd = VDO_CMD_READ_INFO;
    ccprintf("INFO...");
  }
  else if (!strcasecmp(argv[3], "version"))
  {
    cmd = VDO_CMD_VERSION;
    ccprintf("VERSION...");
  }
  else
  {
    int i;
    argc -= 3;
    for (i = 0; i < argc; i++)
      if (hex8tou32(argv[i + 3], data + i))
        return EC_ERROR_INVAL;
    cmd = VDO_CMD_FLASH_WRITE;
    cnt = argc;
    ccprintf("WRITE %d @%04x ...", argc * 4, flash_offset[port]);
    flash_offset[port] += argc * 4;
  }

  pd_send_vdm(USB_VID_GOOGLE, cmd, data, cnt);

  /* Wait until VDM is done */
  while (pd.vdm_state > 0)
    task_wait_event(100 * MSEC_US);

  ccprintf("DONE %d\n", pd.vdm_state);
  return EC_SUCCESS;
}
#endif /* defined(CONFIG_CMD_PD) && defined(CONFIG_CMD_PD_FLASH) */

#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP)
void pd_send_hpd(enum hpd_event hpd)
{
  uint32_t data[1];
  int opos = pd_alt_mode(USB_SID_DISPLAYPORT);
  if (!opos)
    return;

  data[0] = VDO_DP_STATUS((hpd == hpd_irq), /* IRQ_HPD */
                          (hpd != hpd_low), /* HPD_HI|LOW */
                          0,                /* request exit DP */
                          0,                /* request exit USB */
                          0,                /* MF pref */
                          1,                /* enabled */
                          0,                /* power low */
                          0x2);
  pd_send_vdm(USB_SID_DISPLAYPORT, VDO_OPOS(opos) | CMD_ATTENTION, data, 1);
  /* Wait until VDM is done. */
  while (pd[0].vdm_state > 0)
    task_wait_event(USB_PD_RX_TMOUT_US * (PD_RETRY_COUNT + 1));
}
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE
void pd_request_source_voltage(int mv)
{
  pd_set_max_voltage(mv);

  if (pd.task_state == PD_STATE_SNK_READY || pd.task_state == PD_STATE_SNK_TRANSITION)
  {
    /* Set flag to send new power request in pd_task */
    pd.new_power_request = 1;
  }
  else
  {
    pd.power_role = PD_ROLE_SINK;
    tcpm_set_cc(TYPEC_CC_RD);
    set_state(PD_STATE_SNK_DISCONNECTED);
  }

  task_wake();
}

void pd_set_external_voltage_limit(int mv)
{
  pd_set_max_voltage(mv);

  if (pd.task_state == PD_STATE_SNK_READY || pd.task_state == PD_STATE_SNK_TRANSITION)
  {
    /* Set flag to send new power request in pd_task */
    pd.new_power_request = 1;

    task_wake();
  }
}

void pd_update_contract()
{
  if ((pd.task_state >= PD_STATE_SRC_NEGOCIATE) && (pd.task_state <= PD_STATE_SRC_GET_SINK_CAP))
  {
    pd.flags |= PD_FLAGS_UPDATE_SRC_CAPS;

    task_wake();
  }
}

#endif /* CONFIG_USB_PD_DUAL_ROLE */

#ifdef HAS_TASK_HOSTCMD

static int hc_pd_ports(struct host_cmd_handler_args* args)
{
  struct ec_response_usb_pd_ports* r = args->response;
  r->num_ports = 1;

  args->response_size = sizeof(*r);
  return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_PORTS, hc_pd_ports, EC_VER_MASK(0));

static const enum pd_dual_role_states dual_role_map[USB_PD_CTRL_ROLE_COUNT] = {
        [USB_PD_CTRL_ROLE_TOGGLE_ON] = PD_DRP_TOGGLE_ON,
        [USB_PD_CTRL_ROLE_TOGGLE_OFF] = PD_DRP_TOGGLE_OFF,
        [USB_PD_CTRL_ROLE_FORCE_SINK] = PD_DRP_FORCE_SINK,
        [USB_PD_CTRL_ROLE_FORCE_SOURCE] = PD_DRP_FORCE_SOURCE,
        [USB_PD_CTRL_ROLE_FREEZE] = PD_DRP_FREEZE,
};

#ifdef CONFIG_USBC_SS_MUX
static const enum typec_mux typec_mux_map[USB_PD_CTRL_MUX_COUNT] = {
        [USB_PD_CTRL_MUX_NONE] = TYPEC_MUX_NONE,
        [USB_PD_CTRL_MUX_USB] = TYPEC_MUX_USB,
        [USB_PD_CTRL_MUX_AUTO] = TYPEC_MUX_DP,
        [USB_PD_CTRL_MUX_DP] = TYPEC_MUX_DP,
        [USB_PD_CTRL_MUX_DOCK] = TYPEC_MUX_DOCK,
};
#endif

static enum ec_status hc_usb_pd_control(struct host_cmd_handler_args* args)
{
  const struct ec_params_usb_pd_control* p = args->params;
  struct ec_response_usb_pd_control_v2* r_v2 = args->response;
  struct ec_response_usb_pd_control_v1* r_v1 = args->response;
  struct ec_response_usb_pd_control* r = args->response;
  const char* task_state_name;
  mux_state_t mux_state;

  if (p->port >= board_get_usb_pd_port_count())
    return EC_RES_INVALID_PARAM;

  if (p->role >= USB_PD_CTRL_ROLE_COUNT || p->mux >= USB_PD_CTRL_MUX_COUNT)
    return EC_RES_INVALID_PARAM;

  if (p->role != USB_PD_CTRL_ROLE_NO_CHANGE)
  {
    if (IS_ENABLED(CONFIG_USB_PD_DUAL_ROLE))
      pd_set_dual_role(p->port, dual_role_map[p->role]);
    else
      return EC_RES_INVALID_PARAM;
  }

  if (IS_ENABLED(CONFIG_USBC_SS_MUX) && p->mux != USB_PD_CTRL_MUX_NO_CHANGE)
    usb_mux_set(p->port,
                typec_mux_map[p->mux],
                typec_mux_map[p->mux] == USB_PD_MUX_NONE ? USB_SWITCH_DISCONNECT : USB_SWITCH_CONNECT,
                polarity_rm_dts(pd_get_polarity(p->port)));

  if (p->swap == USB_PD_CTRL_SWAP_DATA)
  {
    pd_request_data_swap(p->port);
  }
  else if (IS_ENABLED(CONFIG_USB_PD_DUAL_ROLE))
  {
    if (p->swap == USB_PD_CTRL_SWAP_POWER)
      pd_request_power_swap(p->port);
    else if (IS_ENABLED(CONFIG_USBC_VCONN_SWAP) && p->swap == USB_PD_CTRL_SWAP_VCONN)
      pd_request_vconn_swap(p->port);
  }

  switch (args->version)
  {
    case 0:
      r->enabled = pd_comm_is_enabled(p->port);
      r->polarity = pd_get_polarity(p->port);
      r->role = pd_get_power_role(p->port);
      r->state = pd_get_task_state(p->port);
      args->response_size = sizeof(*r);
      break;
    case 1:
    case 2:
      r_v2->enabled = (pd_comm_is_enabled(p->port) ? PD_CTRL_RESP_ENABLED_COMMS : 0) |
                      (pd_is_connected(p->port) ? PD_CTRL_RESP_ENABLED_CONNECTED : 0) |
                      (pd_capable(p->port) ? PD_CTRL_RESP_ENABLED_PD_CAPABLE : 0);
      r_v2->role = pd_get_role_flags(p->port);
      r_v2->polarity = pd_get_polarity(p->port);

      r_v2->cc_state = pd_get_task_cc_state(p->port);
      task_state_name = pd_get_task_state_name(p->port);
      if (task_state_name)
        strzcpy(r_v2->state, task_state_name, sizeof(r_v2->state));
      else
        r_v2->state[0] = '\0';

      r_v2->control_flags = get_pd_control_flags(p->port);
      if (IS_ENABLED(CONFIG_USB_PD_ALT_MODE_DFP))
      {
        r_v2->dp_mode = get_dp_pin_mode(p->port);
        mux_state = usb_mux_get(p->port);
        if (mux_state & USB_PD_MUX_USB4_ENABLED)
        {
          r_v2->cable_speed = get_usb4_cable_speed(p->port);
        }
        if (mux_state & USB_PD_MUX_TBT_COMPAT_ENABLED || mux_state & USB_PD_MUX_USB4_ENABLED)
        {
          r_v2->cable_speed = get_tbt_cable_speed(p->port);
          r_v2->cable_gen = get_tbt_rounded_support(p->port);
        }
      }

      if (args->version == 1)
        args->response_size = sizeof(*r_v1);
      else
        args->response_size = sizeof(*r_v2);

      break;
    default:
      return EC_RES_INVALID_PARAM;
  }
  return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_CONTROL, hc_usb_pd_control, EC_VER_MASK(0) | EC_VER_MASK(1));

static int hc_remote_flash(struct host_cmd_handler_args* args)
{
  const struct ec_params_usb_pd_fw_update* p = args->params;
  = p->port;
  const uint32_t* data = &(p->size) + 1;
  int i, size, rv = EC_RES_SUCCESS;
  timestamp_t timeout;

  if (port >= 1)
    return EC_RES_INVALID_PARAM;

  if (p->size + sizeof(*p) > args->params_size)
    return EC_RES_INVALID_PARAM;

#if defined(CONFIG_BATTERY_PRESENT_CUSTOM) || defined(CONFIG_BATTERY_PRESENT_GPIO)
  /*
   * Do not allow PD firmware update if no battery and this port
   * is sinking power, because we will lose power.
   */
  if (battery_is_present() != BP_YES && charge_manager_get_active_charge_port() == port)
    return EC_RES_UNAVAILABLE;
#endif

  /*
   * Busy still with a VDM that host likely generated.  1 deep VDM queue
   * so just return for retry logic on host side to deal with.
   */
  if (pd.vdm_state > 0)
    return EC_RES_BUSY;

  switch (p->cmd)
  {
    case USB_PD_FW_REBOOT:
      pd_send_vdm(USB_VID_GOOGLE, VDO_CMD_REBOOT, NULL, 0);

      /*
       * Return immediately to free pending i2c bus.	Host needs to
       * manage this delay.
       */
      return EC_RES_SUCCESS;

    case USB_PD_FW_FLASH_ERASE:
      pd_send_vdm(USB_VID_GOOGLE, VDO_CMD_FLASH_ERASE, NULL, 0);

      /*
       * Return immediately.	Host needs to manage delays here which
       * can be as long as 1.2 seconds on 64KB RW flash.
       */
      return EC_RES_SUCCESS;

    case USB_PD_FW_ERASE_SIG:
      pd_send_vdm(USB_VID_GOOGLE, VDO_CMD_ERASE_SIG, NULL, 0);
      timeout.val = get_time().val + 500 * MSEC_US;
      break;

    case USB_PD_FW_FLASH_WRITE:
      /* Data size must be a multiple of 4 */
      if (!p->size || p->size % 4)
        return EC_RES_INVALID_PARAM;

      size = p->size / 4;
      for (i = 0; i < size; i += VDO_MAX_SIZE - 1)
      {
        pd_send_vdm(USB_VID_GOOGLE, VDO_CMD_FLASH_WRITE, data + i, MIN(size - i, VDO_MAX_SIZE - 1));
        timeout.val = get_time().val + 500 * MSEC_US;

        /* Wait until VDM is done */
        while ((pd.vdm_state > 0) && (get_time().val < timeout.val))
          task_wait_event(10 * MSEC_US);

        if (pd.vdm_state > 0)
          return EC_RES_TIMEOUT;
      }
      return EC_RES_SUCCESS;

    default:
      return EC_RES_INVALID_PARAM;
      break;
  }

  /* Wait until VDM is done or timeout */
  while ((pd.vdm_state > 0) && (get_time().val < timeout.val))
    task_wait_event(50 * MSEC_US);

  if ((pd.vdm_state > 0) || (pd.vdm_state == VDM_STATE_ERR_TMOUT))
    rv = EC_RES_TIMEOUT;
  else if (pd.vdm_state < 0)
    rv = EC_RES_ERROR;

  return rv;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_FW_UPDATE, hc_remote_flash, EC_VER_MASK(0));

static int hc_remote_rw_hash_entry(struct host_cmd_handler_args* args)
{
  int i, idx = 0, found = 0;
  const struct ec_params_usb_pd_rw_hash_entry* p = args->params;
  static int rw_hash_next_idx;

  if (!p->dev_id)
    return EC_RES_INVALID_PARAM;

  for (i = 0; i < RW_HASH_ENTRIES; i++)
  {
    if (p->dev_id == rw_hash_table[i].dev_id)
    {
      idx = i;
      found = 1;
      break;
    }
  }
  if (!found)
  {
    idx = rw_hash_next_idx;
    rw_hash_next_idx = rw_hash_next_idx + 1;
    if (rw_hash_next_idx == RW_HASH_ENTRIES)
      rw_hash_next_idx = 0;
  }
  memcpy(&rw_hash_table[idx], p, sizeof(*p));

  return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_RW_HASH_ENTRY, hc_remote_rw_hash_entry, EC_VER_MASK(0));

static int hc_remote_pd_dev_info(struct host_cmd_handler_args* args)
{
  const uint8_t* port = args->params;
  struct ec_params_usb_pd_rw_hash_entry* r = args->response;

  if (*port >= 1)
    return EC_RES_INVALID_PARAM;

  r->dev_id = pd[*port].dev_id;

  if (r->dev_id)
  {
    memcpy(r->dev_rw_hash, pd[*port].dev_rw_hash, PD_RW_HASH_SIZE);
  }

  r->current_image = pd[*port].current_image;

  args->response_size = sizeof(*r);
  return EC_RES_SUCCESS;
}

DECLARE_HOST_COMMAND(EC_CMD_USB_PD_DEV_INFO, hc_remote_pd_dev_info, EC_VER_MASK(0));

#ifndef CONFIG_USB_PD_TCPC
#ifdef CONFIG_EC_CMD_PD_CHIP_INFO
static int hc_remote_pd_chip_info(struct host_cmd_handler_args* args)
{
  const struct ec_params_pd_chip_info* p = args->params;
  struct ec_response_pd_chip_info *r = args->response, *info;

  if (p->port >= 1)
    return EC_RES_INVALID_PARAM;

  if (tcpm_get_chip_info(p->p->renew, &info))
    return EC_RES_ERROR;

  memcpy(r, info, sizeof(*r));
  args->response_size = sizeof(*r);

  return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_PD_CHIP_INFO, hc_remote_pd_chip_info, EC_VER_MASK(0));
#endif
#endif

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
static int hc_remote_pd_set_amode(struct host_cmd_handler_args* args)
{
  const struct ec_params_usb_pd_set_mode_request* p = args->params;

  if ((p->port >= 1) || (!p->svid) || (!p->opos))
    return EC_RES_INVALID_PARAM;

  switch (p->cmd)
  {
    case PD_EXIT_MODE:
      if (pd_dfp_exit_mode(p->p->svid, p->opos))
        pd_send_vdm(p->p->svid, CMD_EXIT_MODE | VDO_OPOS(p->opos), NULL, 0);
      else
      {
        CPRINTF("Failed exit mode\n");
        return EC_RES_ERROR;
      }
      break;
    case PD_ENTER_MODE:
      if (pd_dfp_enter_mode(p->p->svid, p->opos))
        pd_send_vdm(p->p->svid, CMD_ENTER_MODE | VDO_OPOS(p->opos), NULL, 0);
      break;
    default:
      return EC_RES_INVALID_PARAM;
  }
  return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_SET_AMODE, hc_remote_pd_set_amode, EC_VER_MASK(0));
#endif /* CONFIG_USB_PD_ALT_MODE_DFP */

#endif /* HAS_TASK_HOSTCMD */

#ifdef CONFIG_CMD_PD_CONTROL

static int pd_control(struct host_cmd_handler_args* args)
{
  static int pd_control_disabled;
  const struct ec_params_pd_control* cmd = args->params;
  int enable = 0;

  if (cmd->chip >= 1)
    return EC_RES_INVALID_PARAM;

  /* Always allow disable command */
  if (cmd->subcmd == PD_CONTROL_DISABLE)
  {
    pd_control_disabled = 1;
    return EC_RES_SUCCESS;
  }

  if (pd_control_disabled)
    return EC_RES_ACCESS_DENIED;

  if (cmd->subcmd == PD_SUSPEND)
  {
    enable = 0;
  }
  else if (cmd->subcmd == PD_RESUME)
  {
    enable = 1;
  }
  else if (cmd->subcmd == PD_RESET)
  {
#ifdef HAS_TASK_PDCMD
    board_reset_pd_mcu();
#else
    return EC_RES_INVALID_COMMAND;
#endif
  }
  else if (cmd->subcmd == PD_CHIP_ON && board_set_tcpc_power_mode)
  {
    board_set_tcpc_power_mode(cmd->chip, 1);
    return EC_RES_SUCCESS;
  }
  else
  {
    return EC_RES_INVALID_COMMAND;
  }

  pd_comm_enable(cmd->chip, enable);
  pd_set_suspend(cmd->chip, !enable);

  return EC_RES_SUCCESS;
}

DECLARE_HOST_COMMAND(EC_CMD_PD_CONTROL, pd_control, EC_VER_MASK(0));
#endif /* CONFIG_CMD_PD_CONTROL */

int is_sink_ready() { return pd.task_state == PD_STATE_SNK_READY; }

const char* get_state_cstr()
{
  switch (pd.task_state)
  {
    case PD_STATE_DISABLED:
      return "PD_STATE_DISABLED";
    case PD_STATE_SUSPENDED:
      return "PD_STATE_SUSPENDED";
#ifdef CONFIG_USB_PD_DUAL_ROLE
    case PD_STATE_SNK_DISCONNECTED:
      return "PD_STATE_SNK_DISCONNECTED";
    case PD_STATE_SNK_DISCONNECTED_DEBOUNCE:
      return "PD_STATE_SNK_DISCONNECTED_DEBOUNCE";
    case PD_STATE_SNK_HARD_RESET_RECOVER:
      return "PD_STATE_SNK_HARD_RESET_RECOVER";
    case PD_STATE_SNK_DISCOVERY:
      return "PD_STATE_SNK_DISCOVERY";
    case PD_STATE_SNK_REQUESTED:
      return "PD_STATE_SNK_REQUESTED";
    case PD_STATE_SNK_TRANSITION:
      return "PD_STATE_SNK_TRANSITION";
    case PD_STATE_SNK_READY:
      return "PD_STATE_SNK_READY";
    case PD_STATE_SNK_SWAP_INIT:
      return "PD_STATE_SNK_SWAP_INIT";
    case PD_STATE_SNK_SWAP_SNK_DISABLE:
      return "PD_STATE_SNK_SWAP_SNK_DISABLE";
    case PD_STATE_SNK_SWAP_SRC_DISABLE:
      return "PD_STATE_SNK_SWAP_SRC_DISABLE";
    case PD_STATE_SNK_SWAP_STANDBY:
      return "PD_STATE_SNK_SWAP_STANDBY";
    case PD_STATE_SNK_SWAP_COMPLETE:
      return "PD_STATE_SNK_SWAP_COMPLETE";
#endif /* CONFIG_USB_PD_DUAL_ROLE */

    case PD_STATE_SRC_DISCONNECTED:
      return "PD_STATE_SRC_DISCONNECTED";
    case PD_STATE_SRC_DISCONNECTED_DEBOUNCE:
      return "PD_STATE_SRC_DISCONNECTED_DEBOUNCE";
    case PD_STATE_SRC_HARD_RESET_RECOVER:
      return "PD_STATE_SRC_HARD_RESET_RECOVER";
    case PD_STATE_SRC_STARTUP:
      return "PD_STATE_SRC_STARTUP";
    case PD_STATE_SRC_DISCOVERY:
      return "PD_STATE_SRC_DISCOVERY";
    case PD_STATE_SRC_NEGOCIATE:
      return "PD_STATE_SRC_NEGOCIATE";
    case PD_STATE_SRC_ACCEPTED:
      return "PD_STATE_SRC_ACCEPTED";
    case PD_STATE_SRC_POWERED:
      return "PD_STATE_SRC_POWERED";
    case PD_STATE_SRC_TRANSITION:
      return "PD_STATE_SRC_TRANSITION";
    case PD_STATE_SRC_READY:
      return "PD_STATE_SRC_READY";
    case PD_STATE_SRC_GET_SINK_CAP:
      return "PD_STATE_SRC_GET_SINK_CAP";
    case PD_STATE_DR_SWAP:
      return "PD_STATE_DR_SWAP";

#ifdef CONFIG_USB_PD_DUAL_ROLE
    case PD_STATE_SRC_SWAP_INIT:
      return "PD_STATE_SRC_SWAP_INIT";
    case PD_STATE_SRC_SWAP_SNK_DISABLE:
      return "PD_STATE_SRC_SWAP_SNK_DISABLE";
    case PD_STATE_SRC_SWAP_SRC_DISABLE:
      return "PD_STATE_SRC_SWAP_SRC_DISABLE";
    case PD_STATE_SRC_SWAP_STANDBY:
      return "PD_STATE_SRC_SWAP_STANDBY";

#ifdef CONFIG_USBC_VCONN_SWAP
    case PD_STATE_VCONN_SWAP_SEND:
      return "PD_STATE_VCONN_SWAP_SEND";
    case PD_STATE_VCONN_SWAP_INIT:
      return "PD_STATE_VCONN_SWAP_INIT";
    case PD_STATE_VCONN_SWAP_READY:
      return "PD_STATE_VCONN_SWAP_READY";
#endif /* CONFIG_USBC_VCONN_SWAP */
#endif /* CONFIG_USB_PD_DUAL_ROLE */

    case PD_STATE_SOFT_RESET:
      return "PD_STATE_SOFT_RESET";
    case PD_STATE_HARD_RESET_SEND:
      return "PD_STATE_HARD_RESET_SEND";
    case PD_STATE_HARD_RESET_EXECUTE:
      return "PD_STATE_HARD_RESET_EXECUTE";
#ifdef CONFIG_COMMON_RUNTIME
    case PD_STATE_BIST_RX:
      return "PD_STATE_BIST_RX";
    case PD_STATE_BIST_TX:
      return "PD_STATE_BIST_TX";
#endif

#ifdef CONFIG_USB_PD_DUAL_ROLE_AUTO_TOGGLE
    case PD_STATE_DRP_AUTO_TOGGLE:
      return "PD_STATE_DRP_AUTO_TOGGLE";
#endif
    /* Number of states. Not an actual state. */
    case PD_STATE_COUNT:
      return "PD_STATE_COUNT";
    case PD_STATE_ENTER_USB:
      return "PD_STATE_ENTER_USB";
  }
  return "";
}
