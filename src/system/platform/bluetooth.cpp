#include "bluetooth.h"

#include <bluefruit.h>
#include <cstdint>

#include "src/system/logic/alerts.h"
#include "src/system/utils/constants.h"

#include "src/system/physical/battery.h"

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

/// indicates if the connection is secured and we can trust the commands
bool isConnectionSecured = false;

/// Indicates if the last advertising cancel command was automatic or requested
bool advertisingStoppedByRequest = false;

// System Info Service
BLEDis bleSystemInfo;
// System battery service
BLEBas bleBatteryService;

/// led controler service
::lampda::bluetooth::BLEElkService bleElkService;

static bool isInitialized = false;

/// Store the known paired device
static ble_gap_addr_t identityAddr[1];
/// Keep track of if we have a bounded peer
static bool hasBoundedPeer = false;

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

void connect_callback(uint16_t connHdl)
{
  BLEConnection* conn = Bluefruit.Connection(connHdl);
  conn->requestPHY();            // request 2M PHY if available
  conn->requestMtuExchange(247); // request larger MTU
  conn->requestPairing();        // if not a paired device, will require a pairing event

  const auto batteryLevel = physical::battery::get_battery_minimum_cell_level();
  write_battery_level(static_cast<uint8_t>(batteryLevel / 100));
  platform::lampda_print("Bluetooth connected");

  if (__private::hasBoundedPeer)
  {
    ble_gap_addr_t peerAddr = conn->getPeerAddr();

    bool isSame = true;
    for (size_t i = 0; i < BLE_GAP_ADDR_LEN; i++)
    {
      if (peerAddr.addr[i] != __private::identityAddr[0].addr[i])
      {
        isSame = false;
        break;
      }
    }
    if (not isSame)
    {
      // refuse
      platform::lampda_print("[BLE] refused: not the same address: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X",
                             peerAddr.addr[5],
                             peerAddr.addr[4],
                             peerAddr.addr[3],
                             peerAddr.addr[2],
                             peerAddr.addr[1],
                             peerAddr.addr[0]);
      Bluefruit.disconnect(connHdl);
    }
  }
}

void pair_completed_callback(uint16_t connHdl, uint8_t authStatus)
{
  if (authStatus == 0)
  {
    // Resolve the peer IDENTITY address
    BLEConnection* conn = Bluefruit.Connection(connHdl);
    ble_gap_addr_t peerAddr = conn->getPeerAddr();

    identityAddr[0] = peerAddr;
    hasBoundedPeer = true;
    // TODO: save_identity(identityAddr);
    platform::lampda_print("BLE pairing succeeded: new pair associated with 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X",
                           __private::identityAddr[0].addr[5],
                           __private::identityAddr[0].addr[4],
                           __private::identityAddr[0].addr[3],
                           __private::identityAddr[0].addr[2],
                           __private::identityAddr[0].addr[1],
                           __private::identityAddr[0].addr[0]);
  }
  else
  {
    platform::lampda_print("BLE pairing failed : disconnecting");
  }
  // disconnect in the end because the applications cannot detect us if we are connected
  Bluefruit.disconnect(connHdl);
}

void connection_secured_callback(uint16_t connHdl) { isConnectionSecured = true; }

void disconnect_callback(uint16_t conn_hdl, uint8_t reason)
{
  isConnectionSecured = false;
  // Dont stop advertising here, some BLE drivers can send one command by connections.
  // Instead, restart the advertising
  platform::lampda_print("Bluetooth disconnected");
  start_advertising(not hasBoundedPeer);
}

void adv_stop_callback(void)
{
  // auto turned off, start again !
  if (not advertisingStoppedByRequest)
  {
    start_advertising(not hasBoundedPeer);
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

  isConnectionSecured = false;

  // TODO: load_identity(identityAddr); if it exist

  // pairs devices
  static constexpr uint8_t peripheralCount = 1;
  static constexpr uint8_t centralCount = 0;
  Bluefruit.begin(peripheralCount, centralCount);
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values

  Bluefruit.Security.setIOCaps(false, false, false);
  Bluefruit.Security.setMITM(false); // ManInTheMiddle protection
  Bluefruit.Security.setPairCompleteCallback(pair_completed_callback);
  Bluefruit.Security.setSecuredCallback(connection_secured_callback);

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

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  // Configure and start the BLE Uart service
  platform::lampda_print("Blutooth started under the name:%s", ble_name);

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  isInitialized = true;
}

} // namespace __private

/*
 *
 *
 */

bool is_activated() { return __private::isInitialized; }

bool is_advertising() { return Bluefruit.Advertising.isRunning(); }

bool is_connected() { return Bluefruit.connected() != 0; }

bool is_secured() { return is_connected() && __private::isConnectionSecured; }

// void display_infos() { Bluefruit.printInfo(); }

void start_advertising(const bool isOpenToAll)
{
  if (!__private::isInitialized)
  {
    // call once when the program starts
    __private::startup_sequence();
  }

  __private::stop_advertising();

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Advertise services that we want to advertise only
  Bluefruit.Advertising.addService(__private::bleSystemInfo);
  // Bluefruit.Advertising.addService(bleBatteryService);
  Bluefruit.Advertising.addService(__private::bleElkService);

  Bluefruit.Advertising.setStopCallback(__private::adv_stop_callback);
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.setInterval(32, 244);             // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(ADV_TIMEOUT_FAST); // advertisement timeout

  if (not isOpenToAll)
  {
    if (__private::hasBoundedPeer)
    {
      // set whitelist
      const ble_gap_addr_t* temp = __private::identityAddr;
      uint32_t err = sd_ble_gap_whitelist_set(&temp, 1);
      if (err == NRF_SUCCESS)
      {
        Bluefruit.Advertising.setFilter(BLE_GAP_ADV_FP_ANY);
        // force the use of the whitelist
        // TODO: for some reason this makes the bluetooth advertisement to fail
        // Bluefruit.Advertising.setFilter(BLE_GAP_ADV_FP_FILTER_CONNREQ);
        platform::lampda_print(
                "[BLE] start paired bluetooth advertisement with 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X",
                __private::identityAddr[0].addr[5],
                __private::identityAddr[0].addr[4],
                __private::identityAddr[0].addr[3],
                __private::identityAddr[0].addr[2],
                __private::identityAddr[0].addr[1],
                __private::identityAddr[0].addr[0]);
      }
      else
      {
        platform::lampda_print("[BLE] failed to add paired bluetooth device to whitelist");
        return;
      }
    }
    else
    {
      platform::lampda_print("[BLE] Cannot start paired bluetooth advertisement without a pair");
      return;
    }
  }
  else
  {
    // anyone can connect
    platform::lampda_print("[BLE] start public bluetooth advertisement");
    Bluefruit.Advertising.setFilter(BLE_GAP_ADV_FP_ANY);
  }

  // no need to start again
  __private::advertisingStoppedByRequest = false;

  const bool hasStarted = Bluefruit.Advertising.start(0); // Stop advertising entirely after ADV_TIMEOUT seconds
  if (not hasStarted)
  {
    platform::lampda_print("[BLE] Could not start advertisement");
  }

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

} // namespace bluetooth
} // namespace platform
} // namespace lampda
