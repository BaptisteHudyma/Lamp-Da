#ifndef CHARGER_H
#define CHARGER_H

namespace charger {

void setup();

bool check_vendor_device_values();

bool is_usb_powered();
bool is_powered_on();

bool enable_charge();

// write a sery of commands to the charger to disable the charging process
void disable_charge();

void set_system_voltage(const float voltage);

}  // namespace charger

#endif  // CHARGER_H