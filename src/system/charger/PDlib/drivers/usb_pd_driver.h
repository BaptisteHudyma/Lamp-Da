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
#include <stdint.h>

// #define CONFIG_BBRAM
#define CONFIG_CHARGE_MANAGER
// #define CONFIG_USBC_BACKWARDS_COMPATIBLE_DFP
// #define CONFIG_USBC_VCONN_SWAP
// #define CONFIG_USB_PD_ALT_MODE
// #define CONFIG_USB_PD_CHROMEOS
#define CONFIG_USB_PD_DUAL_ROLE
  // #define CONFIG_USB_PD_GIVE_BACK
  // #define CONFIG_USB_PD_SIMPLE_DFP
  // #define CONFIG_USB_PD_TCPM_TCPCI

#define CONFIG_USB_PD_VBUS_DETECT_TCPC

/* Default pull-up value on the USB-C ports when they are used as source. */
#define CONFIG_USB_PD_PULLUP TYPEC_RP_USB

// remove a warning
#ifdef PD_ROLE_DEFAULT
#undef PD_ROLE_DEFAULT
#endif

/* Override PD_ROLE_DEFAULT in usb_pd.h */
#define PD_ROLE_DEFAULT(port) (PD_ROLE_SINK)

/* Don't automatically change roles */
#undef CONFIG_USB_PD_INITIAL_DRP_STATE
#define CONFIG_USB_PD_INITIAL_DRP_STATE PD_DRP_FREEZE

/* board specific type-C power constants */
/*
 * delay to turn on the power supply max is ~16ms.
 * delay to turn off the power supply max is about ~180ms.
 */
#define PD_POWER_SUPPLY_TURN_ON_DELAY  10000 /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY 20000 /* us */

/* Define typical operating power and max power */
// Most Arduinos seem to be ok with 12 V, but have problems starting around 15 V.
#define PD_OPERATING_POWER_MW (2250ull)
#define PD_MAX_CURRENT_MA     (5000ull)
#define PD_MAX_VOLTAGE_MV     (20000ull)
#define PD_MAX_POWER_MW       (PD_MAX_VOLTAGE_MV * PD_MAX_CURRENT_MA / 1000)

#define PDO_FIXED_FLAGS (PDO_FIXED_DUAL_ROLE | PDO_FIXED_DATA_SWAP | PDO_FIXED_COMM_CAP)

  extern void delay_us(uint64_t us);
  extern void delay_ms(uint64_t ms);

#define usleep(us) (delay_us(us))
#define msleep(ms) (delay_ms(ms))

  typedef union
  {
    uint64_t val;
    struct
    {
      uint32_t lo;
      uint32_t hi;
    } le /* little endian words */;
  } timestamp_t;

  uint32_t pd_task_set_event(uint32_t event, int wait_for_reply);
  void pd_power_supply_reset(int port);

  extern uint8_t get_pd_source_cnt();
  extern uint32_t get_available_pd_current_mA();
  extern uint32_t get_available_pd_voltage_mV();

  // reset the cached values, call on power supply unplugged
  extern void reset_cache();

  uint32_t get_next_pdo_amps();
  uint32_t get_next_pdo_voltage();

  // Get the current timestamp from the system timer.
  timestamp_t get_time(void);

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