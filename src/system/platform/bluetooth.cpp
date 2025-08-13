#include "bluetooth.h"

#include <bluefruit.h>

#include "src/system/alerts.h"
#include "src/system/utils/constants.h"
#include "src/system/utils/print.h"

#include "src/system/platform/time.h"

namespace bluetooth {

#define ADV_TIMEOUT 30 // seconds. Set this higher to automatically stop advertising after a time

// System Info Service
BLEDis bleSystemInfo;
// System battery service
BLEBas bleBatteryService;

static bool isInitialized = false;

// convert a 4-bit nibble to a hexadecimal character
char nibble_to_hex(uint8_t nibble)
{
  nibble &= 0xF;
  return nibble > 9 ? nibble - 10 + 'A' : nibble + '0';
}
// convert an 8-bit byte to a string of 2 hexadecimal characters
void byte_to_str(char* buff, uint8_t val)
{
  buff[0] = nibble_to_hex(val >> 4);
  buff[1] = nibble_to_hex(val);
}

void connect_callback(uint16_t conn_hdl)
{
  alerts::manager.clear(alerts::Type::BLUETOOTH_ADVERT);
  lampda_print("Bluetooth connected");
}

void disconnect_callback(uint16_t conn_hdl, uint8_t reason)
{
  alerts::manager.clear(alerts::Type::BLUETOOTH_ADVERT);
  lampda_print("Bluetooth disconnected");
}

void adv_stop_callback(void)
{
  alerts::manager.clear(alerts::Type::BLUETOOTH_ADVERT);
  lampda_print("Advertising time passed, advertising will now stop.");
}

void set_device_informations()
{
  static const char firmwareRevision[] = {
          EXPECTED_FIRMWARE_VERSION_MAJOR + '0', '.', EXPECTED_FIRMWARE_VERSION_MINOR + '0', 0};
  static const char hardwareRevision[] = {HARDWARE_VERSION_MAJOR + '0', '.', HARDWARE_VERSION_MINOR + '0', 0};
  static const char softwareRevision[] = {USER_SOFTWARE_VERSION_MAJOR + '0', '.', USER_SOFTWARE_VERSION_MINOR + '0', 0};

#ifdef LMBD_LAMP_TYPE__SIMPLE
  bleSystemInfo.setModel("LAMPDA-SIMPLE");
#elif LMBD_LAMP_TYPE__CCT
  bleSystemInfo.setModel("LAMPDA-CCT");
#elif LMBD_LAMP_TYPE__INDEXABLE
  bleSystemInfo.setModel("LAMPDA-RGB");
#endif

  bleSystemInfo.setFirmwareRev(firmwareRevision);
  bleSystemInfo.setHardwareRev(hardwareRevision);
  bleSystemInfo.setSoftwareRev(softwareRevision);
  bleSystemInfo.setManufacturer("Lambda le fou");
  // bleSystemInfo.setRegCertList();
  // bleSystemInfo.setPNPID();
}

void startup_sequence()
{
  if (isInitialized)
    return;

  Bluefruit.begin(1, 1);
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values

  set_device_informations();

  // add services
  bleSystemInfo.begin();
  bleBatteryService.begin();

  const uint32_t MAC_ADDRESS_0 = NRF_FICR->DEVICEADDR[0];
  const uint32_t MAC_ADDRESS_1 = NRF_FICR->DEVICEADDR[1];

  char ble_name[17] = "Lampda-XXXX-XXXX"; // Null-terminated string must be 1 longer than you set it, for the null
  // Fill in the XXXX in ble_name
  byte_to_str(&ble_name[7], (MAC_ADDRESS_0 >> 24) & 0xFF);
  byte_to_str(&ble_name[9], (MAC_ADDRESS_0 >> 16) & 0xFF);
  byte_to_str(&ble_name[12], (MAC_ADDRESS_0 >> 8) & 0xFF);
  byte_to_str(&ble_name[14], (MAC_ADDRESS_0 >> 0) & 0xFF);
  // Set the name we just made
  Bluefruit.setName(ble_name);

  // Configure and start the BLE Uart service
  lampda_print("Blutooth started under the name:%s", ble_name);

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Advertise services that we want to advertise only
  Bluefruit.Advertising.addService(bleSystemInfo);
  // Bluefruit.Advertising.addService(bleBatteryService);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.setStopCallback(adv_stop_callback);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244); // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);   // number of seconds in fast mode

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  isInitialized = true;
}

void start_advertising()
{
  if (!isInitialized)
  {
    // call once when the program starts
    startup_sequence();
  }

  Bluefruit.printInfo();
  Bluefruit.Advertising.start(ADV_TIMEOUT); // Stop advertising entirely after ADV_TIMEOUT seconds

  alerts::manager.raise(alerts::Type::BLUETOOTH_ADVERT);
}

void disable_bluetooth()
{
  if (!isInitialized)
    return;

  alerts::manager.clear(alerts::Type::BLUETOOTH_ADVERT);

  Bluefruit.Advertising.stop();
}

void write_battery_level(const uint8_t batteryLevel)
{
  if (!isInitialized)
    return;
  bleBatteryService.write(batteryLevel);
}

void notify_battery_level(const uint8_t batteryLevel)
{
  if (!isInitialized)
    return;
  bleBatteryService.notify(batteryLevel);
}

} // namespace bluetooth
