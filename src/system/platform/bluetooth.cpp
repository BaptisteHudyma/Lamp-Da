#include "bluetooth.h"

#include <bluefruit.h>
#include <cstdint>

#include "src/system/logic/alerts.h"
#include "src/system/utils/constants.h"

#include "src/system/platform/print.h"
#include "src/system/platform/time.h"

#include "src/system/platform/bluetooth/elk_service.h"

namespace lampda {
namespace platform {
namespace bluetooth {

namespace __private {

#define ADV_TIMEOUT_FAST 30 // seconds. Set this higher to automatically stop advertising after a time
#define ADV_TIMEOUT      30 // seconds. Set this higher to automatically stop advertising after a time

#define BLE_APPEARANCE_LIGHT_SOURCE_GENERIC          0x07C0 /**< Light fixture BLE appearance flag (official flags) */
#define BLE_APPEARANCE_LIGHT_SOURCE_MULTICOLOR_ARRAY 0x07C6 /**< Light fixture BLE appearance flag (official flags) */

/// Indicates if the last advertising cancel command was automatic or requested
bool advertisingStoppedByRequest = false;

// System Info Service
BLEDis bleSystemInfo;
// System battery service
BLEBas bleBatteryService;

/// led controler service
::lampda::bluetooth::BLEElkService bleElkService;

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

void stop_advertising()
{
  if (!isInitialized)
    return;

  logic::alerts::manager.clear(logic::alerts::Type::BLUETOOTH_ADVERT);
  Bluefruit.Advertising.stop();
}

void connect_callback(uint16_t conn_hdl) { platform::lampda_print("Bluetooth connected"); }

void disconnect_callback(uint16_t conn_hdl, uint8_t reason)
{
  // Dont stop advertising here, some BLE drivers can send one command by connections.
  // Instead, restart the advertising
  start_advertising();
  platform::lampda_print("Bluetooth disconnected");
}

void adv_stop_callback(void)
{
  // auto turned off, start again !
  if (not advertisingStoppedByRequest)
  {
    start_advertising();
    platform::lampda_print("BLE Advertising timeout, advertising restarted.");
  }
  else
  {
    __private::stop_advertising();
    platform::lampda_print("BLE Advertising stop requested.");
  }
  advertisingStoppedByRequest = false;
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

  // setup service
  bleSystemInfo.begin();
}

void startup_sequence()
{
  if (isInitialized)
    return;

  // pairs devices
  static constexpr uint8_t peripheralCount = 1;
  static constexpr uint8_t centralCount = 0;
  Bluefruit.begin(peripheralCount, centralCount);
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values

  // add services
  set_device_informations();
  bleBatteryService.begin();
  bleElkService.begin();

  const uint32_t MAC_ADDRESS_0 = NRF_FICR->DEVICEADDR[0];
  const uint32_t MAC_ADDRESS_1 = NRF_FICR->DEVICEADDR[1];

  /// ELK-BLE is necessary to be recognized as a led drivable bluetooth object
  char ble_name[25] =
          "ELK-BLE-Lampda-XXXX-XXXX"; // Null-terminated string must be 1 longer than you set it, for the null
  // Fill in the XXXX in ble_name
  byte_to_str(&ble_name[15], (MAC_ADDRESS_0 >> 24) & 0xFF);
  byte_to_str(&ble_name[17], (MAC_ADDRESS_0 >> 16) & 0xFF);
  byte_to_str(&ble_name[20], (MAC_ADDRESS_0 >> 8) & 0xFF);
  byte_to_str(&ble_name[22], (MAC_ADDRESS_0 >> 0) & 0xFF);

  //  Set the name we just made, and appearance
  Bluefruit.setName(ble_name);
  Bluefruit.setAppearance(BLE_APPEARANCE_LIGHT_SOURCE_MULTICOLOR_ARRAY);

  // Configure and start the BLE Uart service
  platform::lampda_print("Blutooth started under the name:%s", ble_name);

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Advertise services that we want to advertise only
  Bluefruit.Advertising.addService(bleSystemInfo);
  // Bluefruit.Advertising.addService(bleBatteryService);
  Bluefruit.Advertising.addService(bleElkService);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.setStopCallback(adv_stop_callback);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);             // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(ADV_TIMEOUT_FAST); // advertisement timeout

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  isInitialized = true;
}

} // namespace __private

/*
 *
 *
 */

#ifdef USE_BLUETOOTH

bool is_activated() { return __private::isInitialized; }

bool is_advertising() { return Bluefruit.Advertising.isRunning(); }

bool is_connected() { return Bluefruit.connected() != 0; }

// void display_infos() { Bluefruit.printInfo(); }

void start_advertising()
{
  if (!__private::isInitialized)
  {
    // call once when the program starts
    __private::startup_sequence();
  }
  // no need to start again
  if (is_advertising())
    return;

  __private::advertisingStoppedByRequest = false;

  Bluefruit.Advertising.start(ADV_TIMEOUT); // Stop advertising entirely after ADV_TIMEOUT seconds

  // reraise the alert every minutes
  logic::alerts::manager.raise(logic::alerts::Type::BLUETOOTH_ADVERT);
}

void stop_bluetooth_advertising()
{
  if (!__private::isInitialized)
    return;

  __private::advertisingStoppedByRequest = true;
  __private::stop_advertising();
}

void write_battery_level(const uint8_t batteryLevel)
{
  if (!__private::isInitialized)
    return;
  __private::bleBatteryService.write(batteryLevel);
}

void notify_battery_level(const uint8_t batteryLevel)
{
  if (!__private::isInitialized)
    return;
  __private::bleBatteryService.notify(batteryLevel);
}

// Bluetooth can also by disabled at the system level
#else

bool is_activated() { return false; }

bool is_advertising() { return false; }

bool is_connected() { return false; }

void start_advertising() {}

void stop_bluetooth_advertising() {}

void write_battery_level(const uint8_t batteryLevel) {}

void notify_battery_level(const uint8_t batteryLevel) {}

#endif

} // namespace bluetooth
} // namespace platform
} // namespace lampda
