#ifndef CHARGER_H
#define CHARGER_H

#include <Arduino.h>

#include <cstdint>

namespace charger {

void setup();
void shutdown();

bool check_vendor_device_values();

bool is_usb_powered();
bool is_powered_on();

// is charger active
bool is_charging();

// main charge processus, call at every cycles
bool charge_processus();

// write a sery of commands to the charger to disable the charging process
void disable_charge(const bool force = false);

// return the current charge status, or status of the last charge action
String charge_status();

// return the read value of vBus voltage (milliVolts)
uint16_t get_vbus_voltage_mV();

// return the charging current currently applied to the battery
uint16_t get_charge_current_mA();

// return true if the battery is charging very slowly
bool is_slow_charging();

}  // namespace charger

#endif  // CHARGER_H