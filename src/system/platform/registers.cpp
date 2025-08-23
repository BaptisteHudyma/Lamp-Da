#ifndef PLATFORM_REGISTER_CPP
#define PLATFORM_REGISTER_CPP

#include "registers.h"
#include "time.h"

// constants
#include "src/system/utils/time_utils.h"
#include "src/system/utils/constants.h"

// registers
#include <Arduino.h>

// watchdog
#include <hal/nrf_wdt.h>

// convert register id to watchdog register id
nrf_wdt_rr_register_t registerId_to_register(const uint8_t registerId)
{
  switch (registerId)
  {
    case 0:
      return NRF_WDT_RR0;
    case 1:
      return NRF_WDT_RR1;
    case 2:
      return NRF_WDT_RR2;
    case 3:
      return NRF_WDT_RR3;
    case 4:
      return NRF_WDT_RR4;
    case 5:
      return NRF_WDT_RR5;
    case 6:
      return NRF_WDT_RR6;
    case 7:
      return NRF_WDT_RR7;
    default:
      return NRF_WDT_RR0;
  }
}

void setup_watchdog(const uint32_t timeoutDelaySecond)
{
  // Configure watchdog
  nrf_wdt_behaviour_set(NRF_WDT, NRF_WDT_BEHAVIOUR_PAUSE_SLEEP_HALT);
  nrf_wdt_reload_value_set(NRF_WDT, timeoutDelaySecond * 32768 + 1); // set timeout

  // enable registers
  nrf_wdt_reload_request_enable(NRF_WDT, registerId_to_register(USER_WATCHDOG_ID));
  nrf_wdt_reload_request_enable(NRF_WDT, registerId_to_register(POWER_WATCHDOG_ID));

  nrf_wdt_task_trigger(NRF_WDT, NRF_WDT_TASK_START); // Start WDT
}

void kick_watchdog(const uint8_t registerId)
{
  // update watchdog (prevent crash)
  nrf_wdt_reload_request_set(NRF_WDT, registerId_to_register(registerId));
}

void setup_adc(const uint8_t resolution)
{
  analogReference(AR_INTERNAL_3_0); // 3v reference
  analogReadResolution(resolution);
}

uint8_t get_wire_interface_count() { return WIRE_INTERFACES_COUNT; }

void enter_serial_dfu() { enterUf2Dfu(); }

bool is_voltage_detected_on_vbus() { return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0x00; }

// POWER_RESETREAS_RESETPIN_Msk: reset from pin-reset detected
// POWER_RESETREAS_DOG_Msk: reset from watchdog
// POWER_RESETREAS_SREQ_Msk: reset via soft reset
// POWER_RESETREAS_LOCKUP_Msk: reset from cpu lockup
// POWER_RESETREAS_OFF_Msk: wake up from pin interrupt
// POWER_RESETREAS_LPCOMP_Msk: wake up from analogic pin detect (LPCOMP)
// POWER_RESETREAS_DIF_Msk: wake up from debug interface
// POWER_RESETREAS_NFC_Msk: wake from NFC field detection
// POWER_RESETREAS_VBUS_Msk: wake from vbus high signal

bool is_started_from_reset() { return (readResetReason() & POWER_RESETREAS_RESETPIN_Msk) != 0x00; }

bool is_started_from_watchdog() { return (readResetReason() & POWER_RESETREAS_DOG_Msk) != 0x00; }

bool is_started_from_interrupt() { return (readResetReason() & POWER_RESETREAS_OFF_Msk) != 0x00; }

float read_CPU_temperature_degreesC()
{
  static float procTemp = 0.0;
  // avoid spam of the register command
  EVERY_N_MILLIS(500) { procTemp = readCPUTemperature(); }
  return procTemp;
}

void go_to_sleep(const int wakeUpPin) { systemOff(wakeUpPin, 0); }

#endif