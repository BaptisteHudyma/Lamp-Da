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
void disable_charge();

// return the current charge status, or status of the last charge action
String charge_status();

// return the read value of vBus voltage (milliVolts)
uint16_t getVbusVoltage_mV();

}  // namespace charger

#endif  // CHARGER_H