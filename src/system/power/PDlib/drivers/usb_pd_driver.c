/*
 * usb_pd_driver.c
 *
 * Created: 11/11/2017 23:55:12
 *  Author: jason
 */

#include "usb_pd_driver.h"
#include "../usb_pd.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(t) (sizeof(t) / sizeof(t[0]))
#endif

extern struct tc_module tc_instance;
extern uint32_t g_us_timestamp_upper_32bit;

struct SourcePowerParameters otgParameters;
struct SourcePowerParameters get_OTG_requested_parameters() { return otgParameters; }

int canBecomePowerSource = 1; // enabled by default
void set_allow_power_sourcing(const int allowPowerSourcing) { canBecomePowerSource = allowPowerSourcing != 0; }

int _isActivatingOTG = 0;
int is_activating_otg() { return _isActivatingOTG; }

const uint32_t pd_src_pdo[] = {
        // TODO: set the PDO with the battery pack watt
        PDO_FIXED(5000, 1500, PDO_FIXED_FLAGS),
        // PDO_FIXED(9000, 3000, PDO_FIXED_FLAGS),
        // PDO_FIXED(15000, 3000, PDO_FIXED_FLAGS),
        // TODO: debug PPS
        // PDO_VAR(4750, 20000, 3000)
};
const int pd_src_pdo_cnt = ARRAY_SIZE(pd_src_pdo);

const uint32_t pd_snk_pdo[] = {
        // Support all SPR
        PDO_FIXED(5000, 3000, PDO_FIXED_FLAGS),
        PDO_FIXED(9000, 3000, PDO_FIXED_FLAGS),
        PDO_FIXED(15000, 3000, PDO_FIXED_FLAGS),
        PDO_FIXED(20000, 3000, PDO_FIXED_FLAGS),
        // PPS
        // PDO_VAR(4750u, 20000u, 3000u)
};
const int pd_snk_pdo_cnt = ARRAY_SIZE(pd_snk_pdo);

int pdSources = 0;
uint32_t* srcCapsSaved = NULL;

void pd_startup()
{
  // if enabled, will send updates when source
  pd_ping_enable(0);
}

void pd_turn_off()
{
  // stop negociation for sleep mode
  pd_request_source_voltage(5000);
}

// Called by the pd algo when the source capabilities are received
void pd_process_source_cap_callback(int cnt, uint32_t* src_caps)
{
  pdSources = cnt;
  srcCapsSaved = src_caps;
}

uint8_t get_pd_source_cnt() { return pdSources; }
uint32_t get_pd_source(const uint8_t index)
{
  if (index > pdSources)
    return 0;
  return srcCapsSaved[index];
}

uint32_t availableCurrent = 0;
uint32_t availableVoltage = 0;
void pd_set_input_current_limit(uint32_t max_ma, uint32_t supply_voltage)
{
  availableCurrent = max_ma;
  availableVoltage = supply_voltage;
}

uint32_t get_available_pd_current_mA() { return availableCurrent; }

uint32_t get_available_pd_voltage_mV() { return availableVoltage; }

void reset_cache()
{
  pdSources = 0;
  srcCapsSaved = NULL;

  availableCurrent = 0;
  availableVoltage = 0;

  _isActivatingOTG = 0;

  otgParameters.requestedVoltage_mV = 0;
  otgParameters.requestedCurrent_mA = 0;
}

void pd_loop()
{
  static int reset = 0;
  pd_run_state_machine(reset);
  reset = 0;
}

// valid voltages from 0 to 20V
int pd_is_valid_input_voltage(int mv) { return mv > 0 && mv <= 20000; }

int is_pd_conector() { return srcCapsSaved != NULL; }

timestamp_t get_time(void)
{
  timestamp_t t;
  t.val = time_ms() * 1000;
  t.val += time_us() % 1000;
  return t;
}

// close source voltage, discharge vbus
void pd_power_supply_reset()
{
  // reset OTG params
  otgParameters.requestedVoltage_mV = 5000;
  otgParameters.requestedCurrent_mA = 0;
  _isActivatingOTG = 0;
  return;
}

int pd_custom_vdm(int cnt, uint32_t* payload, uint32_t** rpayload)
{
#if 0
	int cmd = PD_VDO_CMD(payload[0]);
	uint16_t dev_id = 0;
	int is_rw, is_latest;

	/* make sure we have some payload */
	if (cnt == 0)
		return 0;

	switch (cmd) {
	case VDO_CMD_VERSION:
		/* guarantee last byte of payload is null character */
		*(payload + cnt - 1) = 0;
		//CPRINTF("version: %s\n", (char *)(payload+1));
		break;
	case VDO_CMD_READ_INFO:
	case VDO_CMD_SEND_INFO:
		/* copy hash */
		if (cnt == 7) {
			dev_id = VDO_INFO_HW_DEV_ID(payload[6]);
			is_rw = VDO_INFO_IS_RW(payload[6]);

			is_latest = pd_dev_store_rw_hash(
							 dev_id,
							 payload + 1,
							 is_rw ?
							 SYSTEM_IMAGE_RW :
							 SYSTEM_IMAGE_RO);

			/*
			 * Send update host event unless our RW hash is
			 * already known to be the latest update RW.
			 */
			if (!is_rw || !is_latest)
				pd_send_host_event(PD_EVENT_UPDATE_DEVICE);

			//CPRINTF("DevId:%d.%d SW:%d RW:%d\n",
			//	HW_DEV_ID_MAJ(dev_id),
			//	HW_DEV_ID_MIN(dev_id),
			//	VDO_INFO_SW_DBG_VER(payload[6]),
			//	is_rw);
		} else if (cnt == 6) {
			/* really old devices don't have last byte */
			pd_dev_store_rw_hash( dev_id, payload + 1,
					     SYSTEM_IMAGE_UNKNOWN);
		}
		break;
	case VDO_CMD_CURRENT:
		CPRINTF("Current: %dmA\n", payload[1]);
		break;
	case VDO_CMD_FLIP:
		/* TODO: usb_mux_flip(); */
		break;
	}
#endif // if 0

  return 0;
}

void pd_execute_data_swap(int data_role) { /* Do nothing */ }

int pd_check_data_swap(int data_role)
{
  /* Allow data swap if we are a UFP, otherwise don't allow. */
  return (data_role == PD_ROLE_UFP) ? 1 : 0;
}

int pd_is_power_swap_succesful()
{
  // accept power swap
  return canBecomePowerSource != 0;
}

// Check that the board health is good
int pd_board_checks(void) { return EC_SUCCESS; }

// enable the source supply
int pd_set_power_supply_ready()
{
  _isActivatingOTG = 1;
  // enable 5V OTG (cold start)
  otgParameters.requestedVoltage_mV = 5000;
  otgParameters.requestedCurrent_mA = 500;

  return EC_SUCCESS; /* we are ready */
}

void pd_transition_voltage(int idx)
{
  // augment OTG voltage/current to requested profile
  uint32_t ma, mv;
  pd_extract_pdo_power(pd_src_pdo[idx], &ma, &mv);

  otgParameters.requestedVoltage_mV = mv;
  otgParameters.requestedCurrent_mA = ma;
}

void pd_check_dr_role(int dr_role, int flags)
{
  /*
      // If UFP, try to switch to DFP
      if ((flags & PD_FLAGS_PARTNER_DR_DATA) && dr_role == PD_ROLE_UFP)
      pd_request_data_swap();
  */
}

void pd_check_pr_role(int pr_role, int flags)
{
  // for some reason, this function can be called in disconnected state
  if (!pd_is_connected())
    return;

  /*
   * If partner is dual-role power and dualrole toggling is on, consider
   * if a power swap is necessary.
   */
  if ((flags & PD_FLAGS_PARTNER_DR_POWER) && pd_get_dual_role() == PD_DRP_TOGGLE_OFF) // PD_DRP_TOGGLE_ON)
  {
    /*
     * If we are a sink and partner is not externally powered, then swap to become a source.
     * If we are source and partner is externally powered, swap to become a sink.
     */
    int partner_extpower = flags & PD_FLAGS_PARTNER_UNCONSTR;

    if ((!partner_extpower && pr_role == PD_ROLE_SINK) || (partner_extpower && pr_role == PD_ROLE_SOURCE))
      pd_request_power_swap();
  }
}
