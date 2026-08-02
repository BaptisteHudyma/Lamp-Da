/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "usb_pd.h"
#include <string.h>

#ifdef CONFIG_COMMON_RUNTIME
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ##args)
#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ##args)
#else
#define CPRINTS(format, args...)
#define CPRINTF(format, args...)
#endif

static int rw_flash_changed = 1;

int pd_check_requested_voltage(uint32_t rdo)
{
  int max_ma = rdo & 0x3FF;
  int op_ma = (rdo >> 10) & 0x3FF;
  int idx = RDO_POS(rdo);
  uint32_t pdo;
  uint32_t pdo_ma;
#if defined(CONFIG_USB_PD_DYNAMIC_SRC_CAP) || defined(CONFIG_USB_PD_MAX_SINGLE_SOURCE_CURRENT)
  const uint32_t* src_pdo;
  const int pdo_cnt = charge_manager_get_source_pdo(&src_pdo);
#else
  const uint32_t* src_pdo = pd_src_pdo;
  const int pdo_cnt = pd_src_pdo_cnt;
#endif

  /* Board specific check for this request */
  if (pd_board_check_request(rdo, pdo_cnt))
    return EC_ERROR_INVAL;

  /* check current ... */
  pdo = src_pdo[idx - 1];
  pdo_ma = (pdo & 0x3ff);
  if (op_ma > pdo_ma)
    return EC_ERROR_INVAL; /* too much op current */
  if (max_ma > pdo_ma && !(rdo & RDO_CAP_MISMATCH))
    return EC_ERROR_INVAL; /* too much max current */

  CPRINTF("Requested %d V %d mA (for %d/%d mA)\n",
          ((pdo >> 10) & 0x3ff) * 50,
          (pdo & 0x3ff) * 10,
          op_ma * 10,
          max_ma * 10);

  /* Accept the requested voltage */
  return EC_SUCCESS;
}

int pd_board_check_request(uint32_t rdo, int pdo_cnt)
{
  int idx = RDO_POS(rdo);

  /* Check for invalid index */
  return (!idx || idx > pdo_cnt) ? EC_ERROR_INVAL : EC_SUCCESS;
}

#ifdef CONFIG_USB_PD_DUAL_ROLE
/* Last received source cap */
static uint32_t pd_src_caps[PDO_MAX_OBJECTS];
static uint8_t pd_src_cap_cnt;

/* Cap on the max voltage requested as a sink (in millivolts) */
static unsigned max_request_mv = PD_MAX_VOLTAGE_MV; /* no cap */

int pd_find_pdo_index(int max_mv, uint32_t* selected_pdo)
{
  int i, uw, mv, ma;
  int ret = 0;
  int __attribute__((unused)) cur_mv = 0;
  int cur_uw = 0;
  int prefer_cur;
  const uint32_t* src_caps = pd_src_caps;

  /* max voltage is always limited by this boards max request */
  max_mv = MIN(max_mv, PD_MAX_VOLTAGE_MV);

  /* Get max power that is under our max voltage input */
  for (i = 0; i < pd_src_cap_cnt; i++)
  {
    /* its an unsupported Augmented PDO (PD3.0) */
    if ((src_caps[i] & PDO_TYPE_MASK) == PDO_TYPE_AUGMENTED)
      continue;

    mv = ((src_caps[i] >> 10) & 0x3FF) * 50;
    /* Skip invalid voltage */
    if (!mv)
      continue;
    /* Skip any voltage not supported by this board */
    if (!pd_is_valid_input_voltage(mv))
      continue;

    if ((src_caps[i] & PDO_TYPE_MASK) == PDO_TYPE_BATTERY)
    {
      uw = 250000 * (src_caps[i] & 0x3FF);
    }
    else
    {
      ma = (src_caps[i] & 0x3FF) * 10;
      ma = MIN(ma, PD_MAX_CURRENT_MA);
      uw = ma * mv;
    }

    if (mv > max_mv)
      continue;
    uw = MIN(uw, PD_MAX_POWER_MW * 1000);
    prefer_cur = 0;

    /* Apply special rules in case of 'tie' */
#ifdef PD_PREFER_LOW_VOLTAGE
    if (uw == cur_uw && mv < cur_mv)
      prefer_cur = 1;
#elif defined(PD_PREFER_HIGH_VOLTAGE)
    if (uw == cur_uw && mv > cur_mv)
      prefer_cur = 1;
#endif
    /* Prefer higher power, except for tiebreaker */
    if (uw > cur_uw || prefer_cur)
    {
      ret = i;
      cur_uw = uw;
      cur_mv = mv;
    }
  }

  if (selected_pdo)
    *selected_pdo = src_caps[ret];

  return ret;
}

void pd_extract_pdo_power(uint32_t pdo, uint32_t* ma, uint32_t* mv)
{
  int max_ma, uw;

  *mv = ((pdo >> 10) & 0x3FF) * 50;

  if (*mv == 0)
  {
    CPRINTF("ERR:PDO mv=0\n");
    *ma = 0;
    return;
  }

  if ((pdo & PDO_TYPE_MASK) == PDO_TYPE_BATTERY)
  {
    uw = 250000 * (pdo & 0x3FF);
    max_ma = 1000 * MIN(1000 * uw, PD_MAX_POWER_MW) / *mv;
  }
  else
  {
    max_ma = 10 * (pdo & 0x3FF);
    max_ma = MIN(max_ma, PD_MAX_POWER_MW * 1000 / *mv);
  }

  *ma = MIN(max_ma, PD_MAX_CURRENT_MA);
}

int pd_build_request(uint32_t* rdo, uint32_t* ma, uint32_t* mv, enum pd_request_type req_type)
{
  uint32_t pdo;
  int pdo_index, flags = 0;
  int uw;
  int max_or_min_ma;
  int max_or_min_mw;

  if (req_type == PD_REQUEST_VSAFE5V)
  {
    /* src cap 0 should be vSafe5V */
    pdo_index = 0;
    pdo = pd_src_caps[0];
  }
  else
  {
    /* find pdo index for max voltage we can request */
    pdo_index = pd_find_pdo_index(max_request_mv, &pdo);
  }

  pd_extract_pdo_power(pdo, ma, mv);
  uw = *ma * *mv;
  /* Mismatch bit set if less power offered than the operating power */
  if (uw < (1000 * PD_OPERATING_POWER_MW))
    flags |= RDO_CAP_MISMATCH;

#ifdef CONFIG_USB_PD_GIVE_BACK
  /* Tell source we are give back capable. */
  flags |= RDO_GIVE_BACK;

  /*
   * BATTERY PDO: Inform the source that the sink will reduce
   * power to this minimum level on receipt of a GotoMin Request.
   */
  max_or_min_mw = PD_MIN_POWER_MW;

  /*
   * FIXED or VARIABLE PDO: Inform the source that the sink will reduce
   * current to this minimum level on receipt of a GotoMin Request.
   */
  max_or_min_ma = PD_MIN_CURRENT_MA;
#else
  /*
   * Can't give back, so set maximum current and power to operating
   * level.
   */
  max_or_min_ma = *ma;
  max_or_min_mw = uw / 1000;
#endif

  if ((pdo & PDO_TYPE_MASK) == PDO_TYPE_BATTERY)
  {
    int mw = uw / 1000;
    *rdo = RDO_BATT(pdo_index + 1, mw, max_or_min_mw, flags);
  }
  else
  {
    *rdo = RDO_FIXED(pdo_index + 1, *ma, max_or_min_ma, flags);
  }
  return EC_SUCCESS;
}

void pd_process_source_cap(int cnt, uint32_t* src_caps)
{
#ifdef CONFIG_CHARGE_MANAGER
  uint32_t ma, mv, pdo;
#endif
  int i;

  pd_src_cap_cnt = cnt;
  for (i = 0; i < cnt; i++)
    pd_src_caps[i] = *src_caps++;

#ifdef CONFIG_CHARGE_MANAGER
  /* Get max power info that we could request */
  pd_find_pdo_index(PD_MAX_VOLTAGE_MV, &pdo);
  pd_extract_pdo_power(pdo, &ma, &mv);

  /* Set max. limit, but apply 500mA ceiling */
  // charge_manager_set_ceil( CEIL_REQUESTOR_PD, PD_MIN_MA);
  pd_set_input_current_limit(ma, mv);
#endif
}

struct cached_tcpm_message
{
  uint32_t header;
  uint32_t payload[7];
};

/* Cache depth needs to be power of 2 */
/* TODO: Keep track of the high water mark */
#define CACHE_DEPTH      (1 << 3)
#define CACHE_DEPTH_MASK (CACHE_DEPTH - 1)

struct queue
{
  /*
   * Head points to the index of the first empty slot to put a new RX
   * message. Must be masked before used in lookup.
   */
  uint32_t head;
  /*
   * Tail points to the index of the first message for the PD task to
   * consume. Must be masked before used in lookup.
   */
  uint32_t tail;
  struct cached_tcpm_message buffer[CACHE_DEPTH];
};
static struct queue cached_messages;

/* Note this method can be called from an interrupt context. */
int tcpm_enqueue_message()
{
  int rv;
  struct queue* const q = &cached_messages;
  struct cached_tcpm_message* const head = &q->buffer[q->head & CACHE_DEPTH_MASK];

  if (q->head - q->tail == CACHE_DEPTH)
  {
    CPRINTS("C%d RX EC Buffer full!", port);
    return EC_ERROR_OVERFLOW;
  }

  /* Blank any old message, just in case. */
  memset(head, 0, sizeof(*head));
  /* Call the raw driver without caching */
  rv = tcpc_config.drv->get_message(head->payload, &head->header);
  if (rv)
  {
    CPRINTS("Could not retrieve RX message (%d)", rv);
    return rv;
  }

  /* Increment atomically to ensure get_message_raw happens-before */
  // TODO: atomic_add(&q->head, 1);
  q->head += 1;

  /* Wake PD task up so it can process incoming RX messages */
  task_set_event(TASK_EVENT_WAKE);

  return EC_SUCCESS;
}

int tcpm_has_pending_message()
{
  const struct queue* const q = &cached_messages;

  return q->head != q->tail;
}

int tcpm_dequeue_message(uint32_t* const payload, int* const header)
{
  struct queue* const q = &cached_messages;
  struct cached_tcpm_message* const tail = &q->buffer[q->tail & CACHE_DEPTH_MASK];

  if (!tcpm_has_pending_message())
  {
    CPRINTS("No message in RX buffer!");
    return EC_ERROR_BUSY;
  }

  /* Copy cache data in to parameters */
  *header = tail->header;
  memcpy(payload, tail->payload, sizeof(tail->payload));

  /* Increment atomically to ensure memcpy happens-before */
  // TODO atomic_add(&q->tail, 1);
  q->tail += 1;
  return EC_SUCCESS;
}

void tcpm_clear_pending_messages()
{
  struct queue* const q = &cached_messages;

  q->tail = q->head;
}

/* Cable structure for storing cable attributes */
struct pd_cable
{
  /* Note: the following fields are used by TCPMv1 */
  /* Last received SOP' message id counter*/
  uint8_t last_sop_p_msg_id;
  /* Last received SOP'' message id counter*/
  uint8_t last_sop_p_p_msg_id;
  /* Cable flags. See CABLE_FLAGS_* */
  uint8_t flags;
  /* For storing Discover mode response from device */
  // union tbt_mode_resp_device dev_mode_resp;
  /* For storing Discover mode response from cable */
  // union tbt_mode_resp_cable cable_mode_resp;

  /* Cable revision */
  // enum pd_rev_type rev;
};

static struct pd_cable cable;

int consume_sop_prime_repeat_msg(uint8_t msg_id)
{
  if (cable.last_sop_p_msg_id != msg_id)
  {
    cable.last_sop_p_msg_id = msg_id;
    return 0;
  }
  CPRINTF("SOP Prime repeat msg_id %d\n", msg_id);
  return 1;
}

int consume_sop_prime_prime_repeat_msg(uint8_t msg_id)
{
  if (cable.last_sop_p_p_msg_id != msg_id)
  {
    cable.last_sop_p_p_msg_id = msg_id;
    return 0;
  }
  CPRINTF("SOP Prime Prime repeat msg_id %d\n", msg_id);
  return 1;
}

void reset_pd_cable()
{
  memset(&cable, 0, sizeof(cable));
  cable.last_sop_p_msg_id = INVALID_MSG_ID_COUNTER;
  cable.last_sop_p_p_msg_id = INVALID_MSG_ID_COUNTER;
}

#pragma weak pd_process_source_cap_callback
void pd_process_source_cap_callback(int cnt, uint32_t* src_caps) {}

void pd_set_max_voltage(unsigned mv) { max_request_mv = mv; }

unsigned pd_get_max_voltage(void) { return max_request_mv; }

#endif /* CONFIG_USB_PD_DUAL_ROLE */

int pd_svdm(int cnt, uint32_t* payload, uint32_t** rpayload) { return 0; }

#ifndef CONFIG_USB_PD_CUSTOM_VDM
int pd_vdm(int cnt, uint32_t* payload, uint32_t** rpayload) { return 0; }
#endif /* !CONFIG_USB_PD_CUSTOM_VDM */

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
static int hc_remote_pd_discovery(struct host_cmd_handler_args* args)
{
  const uint8_t* port = args->params;
  struct ec_params_usb_pd_discovery_entry* r = args->response;

  if (*port >= 1)
    return EC_RES_INVALID_PARAM;

  r->vid = pd_get_identity_vid(*port);
  r->ptype = PD_IDH_PTYPE(pe[*port].identity[0]);
  /* pid only included if vid is assigned */
  if (r->vid)
    r->pid = PD_PRODUCT_PID(pe[*port].identity[2]);

  args->response_size = sizeof(*r);
  return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_DISCOVERY, hc_remote_pd_discovery, EC_VER_MASK(0));

static int hc_remote_pd_get_amode(struct host_cmd_handler_args* args)
{
  struct svdm_amode_data* modep;
  const struct ec_params_usb_pd_get_mode_request* p = args->params;
  struct ec_params_usb_pd_get_mode_response* r = args->response;

  if (p->port >= 1)
    return EC_RES_INVALID_PARAM;

  /* no more to send */
  if (p->svid_idx >= pe[p->port].svid_cnt)
  {
    r->svid = 0;
    args->response_size = sizeof(r->svid);
    return EC_RES_SUCCESS;
  }

  r->svid = pe[p->port].svids[p->svid_idx].svid;
  r->opos = 0;
  memcpy(r->vdo, pe[p->port].svids[p->svid_idx].mode_vdo, 24);
  modep = get_modep(p->r->svid);

  if (modep)
    r->opos = pd_alt_mode(p->r->svid);

  args->response_size = sizeof(*r);
  return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_GET_AMODE, hc_remote_pd_get_amode, EC_VER_MASK(0));

#endif

#define FW_RW_END (CONFIG_EC_WRITABLE_STORAGE_OFF + CONFIG_RW_STORAGE_OFF + CONFIG_RW_SIZE)
