#ifndef CHARGER_H
#define CHARGER_H

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

}  // namespace charger

#endif  // CHARGER_H