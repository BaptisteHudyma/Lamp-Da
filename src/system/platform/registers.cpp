#ifndef PLATFORM_REGISTER_CPP
#define PLATFORM_REGISTER_CPP

#include "registers.h"

#include <Arduino.h>

void setup_watchdog(const uint32_t timeoutDelaySecond)
{
  // Configure WDT
  NRF_WDT->CONFIG = 0x01;                        // Configure WDT to run when CPU is asleep
  NRF_WDT->CRV = timeoutDelaySecond * 32768 + 1; // set timeout
  NRF_WDT->RREN = 0x01;                          // Enable the RR[0] reload register
  NRF_WDT->TASKS_START = 1;                      // Start WDT
}
void kick_watchdog()
{
  // update watchdog (prevent crash)
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void setup_adc(uint8_t resolution)
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

void start_thread(taskfunc_t taskFunction) { Scheduler.startLoop(taskFunction); }

void yield_this_thread() { yield(); }

float read_CPU_temperature_degreesC() { return readCPUTemperature(); }

void go_to_sleep(int wakeUpPin) { systemOff(wakeUpPin, 0); }

#endif