#ifndef CHARGER_H
#define CHARGER_H

namespace charger {

bool check_vendor_device_values();

bool is_usb_powered();
bool is_powered_on();

bool enable_charge();

}  // namespace charger

#endif  // CHARGER_H