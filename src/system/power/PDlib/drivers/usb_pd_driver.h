/*
 * usb_pd_driver.h
 *
 * Created: 11/11/2017 23:55:25
 *  Author: jason
 */

#ifndef USB_PD_DRIVER_H_
#define USB_PD_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../usb_pd.h"
#include "../config.h"
#include <stdint.h>

#define PD_POWER_SUPPLY_TURN_ON_DELAY  250 * MSEC_US /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY 100 * MSEC_US /* us */

/* Define typical operating power and max power */
#define PD_OPERATING_POWER_MW (2250ull)

#define PD_MIN_CURRENT_MA (100ull)
#define PD_MIN_VOLTAGE_MV (5000ull)
#define PD_MIN_POWER_MW   (PD_MIN_VOLTAGE_MV * PD_MIN_CURRENT_MA / 1000)

#define PD_MAX_CURRENT_MA (5000ull)
#define PD_MAX_VOLTAGE_MV (20000ull)
#define PD_MAX_POWER_MW   (PD_MAX_VOLTAGE_MV * PD_MAX_CURRENT_MA / 1000)

#define PDO_FIXED_FLAGS (PDO_FIXED_DUAL_ROLE | PDO_FIXED_DATA_SWAP | PDO_FIXED_COMM_CAP)

#define usleep(us) (delay_us(us))
#define msleep(ms) (delay_ms(ms))

  struct SourcePowerParameters
  {
    uint16_t requestedVoltage_mV;
    uint16_t requestedCurrent_mA;
  };

  struct SinkUsableParameters
  {
    uint32_t current_mA;
    uint32_t voltage_mV;
    uint32_t timestamp;
  };

  /**
   * \brief return the requested OnTheGo parameters, negociated with a power sink
   */
  struct SourcePowerParameters get_OTG_requested_parameters();
  struct SinkUsableParameters get_allowed_consuption();

  void pd_startup();
  void pd_turn_off();

  void pd_loop();

  /// force the power to source mode if force != 0
  void force_set_to_source(int force);

  /**
   * \brief allow or forbid power sourcing from this device
   * \param[in] allowPowerSourcing If 0, this device will only charge. Else, if negociated via usb, this device will
   * charge others.
   */
  void set_allow_power_sourcing(const int allowPowerSourcing);

  /**
   * Regularly update thez battery level
   */
  void set_battery_level(const uint8_t battLevelPercent);

  /**
   * \brief set usb pd to suspend mode
   */
  void supsend_usb_pd(int shouldSuspend);

  /**
   * \brief return 1 if the system is preparing a swap
   */
  int is_activating_otg();

  // should stop all vbus current loading
  int should_stop_vbus_charge();

  void pd_power_supply_reset();

  extern uint8_t get_pd_source_cnt();
  extern uint32_t get_pd_source(const uint8_t index);

  extern uint32_t get_available_pd_current_mA();
  extern uint32_t get_available_pd_voltage_mV();

  // reset the cached values, call on power supply unplugged
  extern void reset_cache();

  // The associated cable is pd capable
  int is_pd_conector();

/* Standard macros / definitions */
#ifndef MAX
#define MAX(a, b)                      \
  ({                                   \
    __typeof__(a) temp_a = (a);        \
    __typeof__(b) temp_b = (b);        \
                                       \
    temp_a > temp_b ? temp_a : temp_b; \
  })
#endif
#ifndef MIN
#define MIN(a, b)                      \
  ({                                   \
    __typeof__(a) temp_a = (a);        \
    __typeof__(b) temp_b = (b);        \
                                       \
    temp_a < temp_b ? temp_a : temp_b; \
  })
#endif

#ifdef __cplusplus
}
#endif

#endif /* USB_PD_DRIVER_H_ */
