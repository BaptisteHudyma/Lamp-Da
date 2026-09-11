#include "src/system/hal/bluetooth.h"

#include <bluefruit.h>
#include <InternalFileSystem.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

// Relative Path Here !!
#include "bluetooth/elk_service.h"
//

#include <cstdint>

#include "src/system/utils/constants.h"

#include "src/system/hal/time.h"
#include "src/system/hal/queues.h"
#include "src/system/hal/filesystem.h"

#include "src/system/bsp/text_out.h"
#include "src/system/bsp/threads.h"

#include "src/system/component/battery.h"

#include "src/system/logic/alerts.h"

namespace lampda {
namespace hal {
namespace bluetooth {

namespace __private {

#define ADV_TIMEOUT_FAST 30 // seconds. Set this higher to automatically stop advertising after a time
#define ADV_TIMEOUT      30 // seconds. Set this higher to automatically stop advertising after a time

#define BLE_APPEARANCE_LIGHT_SOURCE_GENERIC          0x07C0 /**< Light fixture BLE appearance flag (official flags) */
#define BLE_APPEARANCE_LIGHT_SOURCE_MULTICOLOR_ARRAY 0x07C6 /**< Light fixture BLE appearance flag (official flags) */

static constexpr const char* const BLE_BOUND_PEER_FILE = "/.ble_bound.par";
static constexpr const char* const BLE_NAME_FILE = "/.ble_name.par";

/// Indicates if the last advertising cancel command was automatic or requested
bool advertisingStoppedByRequest = false;
/// Keep track of the use
bool _wasUsed = false;

/// System Info Service
BLEDis bleSystemInfo;
/// System battery service
BLEBas bleBatteryService;
/// uart over ble
BLEUart bleuart;

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

// Compare two BLE addresses.
// NOTE: addr_type is intentionally ignored here because after bonding and
// RPA resolution, the SoftDevice provides the identity address regardless of
// which address type the phone used during the connection attempt.
bool addressMatches(const ble_gap_addr_t& a, const ble_gap_addr_t& b)
{
  return (memcmp(a.addr, b.addr, BLE_GAP_ADDR_LEN) == 0);
}

inline static char bleName[MaxBleNameLenght] = "";

/// Paired device address
inline static ble_gap_addr_t identityAddr = {0};
/// If true, the identityAddr is set, refuse all other connections
static bool hasBondedPeer = false;
/// If true, the device is in pairing mode, and can accept all connections
static volatile bool pairingMode = false;

/// Keep track of the currently authorized handle.
static uint16_t authorizedConnHdl = BLE_CONN_HANDLE_INVALID;

bool try_load_bounded_pair(ble_gap_addr_t& addr)
{
  // Try to load the bounded address file
  hal::filesystem::HAL_File boundFile;
  if (boundFile.open(BLE_BOUND_PEER_FILE, hal::filesystem::HAL_File::OpenType::READ) and boundFile.is_open() and
      boundFile.is_available() and boundFile.seek(0))
  {
    const bool isAdressOk =
            (boundFile.read(reinterpret_cast<uint8_t*>(&addr), sizeof(ble_gap_addr_t)) == sizeof(ble_gap_addr_t));
    boundFile.close();
    return isAdressOk;
  }
  boundFile.close();
  return false;
}

void load_bluetooth_name()
{
  auto charValidator = [](const char c) {
    // Allow numbers
    if (c >= '0' && c <= '9')
      return true;
    // Allow lower case letters
    if (c >= 'a' && c <= 'z')
      return true;
    // Allow upper case letters
    if (c >= 'A' && c <= 'Z')
      return true;
    // special alowed chars
    if (c == '-')
      return true;

    // invalid char
    return false;
  };

  // Try to load the bounded address file
  hal::filesystem::HAL_File nameFile;
  if (nameFile.open(BLE_NAME_FILE, hal::filesystem::HAL_File::OpenType::READ) and nameFile.is_open() and
      nameFile.is_available() and nameFile.seek(0))
  {
    const auto nameLenght = nameFile.read((uint8_t*)bleName, nameFile.size());

    bool isValid = true;
    size_t i = 0;
    for (; i <= nameLenght - 1; i++)
    {
      if (bleName[i] == '\0')
        break;
      if (not charValidator(bleName[i]))
      {
        isValid = false;
        break;
      }
    }
    for (; i < MaxBleNameLenght; i++)
    {
      bleName[i] = '\0';
    }

    nameFile.close();

    // valid name, quit
    if (isValid)
      return;

    bsp::lampda_print("Invalid stored BLE name (%s: %d), skipping to default", nameLenght, bleName);
  }

  // Default name !
  const uint32_t MAC_ADDRESS_0 = NRF_FICR->DEVICEADDR[0];
  const uint32_t MAC_ADDRESS_1 = NRF_FICR->DEVICEADDR[1];

  /// ELK-BLE is necessary to be recognized as a led drivable bluetooth object
  const char defaultBleName[25] =
          "ELK-BLE-Lampda-XXXX-XXXX"; // Null-terminated string must be 1 longer than you set it, for the null
  for (int i = 0; i < 25; i++)
    bleName[i] = defaultBleName[i];

  // Fill in the XXXX in ble_name
  byte_to_str(&bleName[15], (MAC_ADDRESS_0 >> 24) & 0xFF);
  byte_to_str(&bleName[17], (MAC_ADDRESS_0 >> 16) & 0xFF);
  byte_to_str(&bleName[20], (MAC_ADDRESS_0 >> 8) & 0xFF);
  byte_to_str(&bleName[22], (MAC_ADDRESS_0 >> 0) & 0xFF);
}

void try_save_bounded_peer(const ble_gap_addr_t& addr)
{
  // Try to load the bounded address file
  hal::filesystem::HAL_File boundFile;

  // load content, without deletion
  if (boundFile.open(BLE_BOUND_PEER_FILE, hal::filesystem::HAL_File::OpenType::WRITE) and boundFile.is_open() and
      boundFile.seek(0))
  {
    boundFile.write(reinterpret_cast<const uint8_t*>(&addr), sizeof(ble_gap_addr_t));
  }
  boundFile.close();
}

void stop_advertising()
{
  __private::pairingMode = false;
  logic::alerts::manager.clear(logic::alerts::Type::BLUETOOTH_ADVERT);

  if (not is_activated())
    return;

  Bluefruit.Advertising.stop();
}

void adv_stop_callback(void)
{
  // clear the alert
  logic::alerts::manager.clear(logic::alerts::Type::BLUETOOTH_ADVERT);

  // skip if bluetooth is off
  if (not is_activated())
    return;

  // auto turned off, start again !
  if (not advertisingStoppedByRequest)
  {
    // Dont restart if this was the pairing mode: it timedout and need restarting
    if (not __private::pairingMode and __private::hasBondedPeer)
      start_advertising(false);
    else
    {
      __private::stop_advertising();
      bsp::lampda_print("BLE Advertising stopped");
    }
  }
  else
  {
    __private::stop_advertising();
    bsp::lampda_print("BLE Advertising stop requested.");
  }
  advertisingStoppedByRequest = false;
}

void pair_complete_callback(uint16_t conn_hdl, uint8_t authStatus)
{
  if (not is_activated())
    return;

  if (authStatus != 0)
  {
    bsp::lampda_print("[Sec]: Pairing failed (0x%02X) - disconnecting", authStatus);
    Bluefruit.disconnect(conn_hdl);
    return;
  }

  if (not Bluefruit.connected(conn_hdl))
  {
    bsp::lampda_print("[Sec] connection handle is already disconnected");
    return;
  }

  bsp::lampda_print("[Sec]: Pairing & bounding success");

  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  if (conn == NULL)
  {
    bsp::lampda_print("[Pair] connection handle invalid");
    return;
  }

  identityAddr = conn->getPeerAddr();
  hasBondedPeer = true;
  pairingMode = false;

  bsp::lampda_print("[Bond] Stored identity type=0x%02X %02X:%02X:%02X:%02X:%02X:%02X",
                    identityAddr.addr_type,
                    identityAddr.addr[5],
                    identityAddr.addr[4],
                    identityAddr.addr[3],
                    identityAddr.addr[2],
                    identityAddr.addr[1],
                    identityAddr.addr[0]);
}

void secured_connection_callback(uint16_t conn_hdl)
{
  if (not is_activated())
    return;

  if (not Bluefruit.connected(conn_hdl))
  {
    bsp::lampda_print("[Security] connection handle is already disconnected");
    return;
  }

  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  if (conn == NULL)
  {
    bsp::lampda_print("[Security] connection handle invalid");
    return;
  }
  const ble_gap_addr_t& resolvedAddr = conn->getPeerAddr();

  bsp::lampda_print("[Security] Secure connection activated=0x%02X %02X:%02X:%02X:%02X:%02X:%02X",
                    resolvedAddr.addr_type,
                    resolvedAddr.addr[5],
                    resolvedAddr.addr[4],
                    resolvedAddr.addr[3],
                    resolvedAddr.addr[2],
                    resolvedAddr.addr[1],
                    resolvedAddr.addr[0]);

  // ── Refresh the stored address if it was an RPA at pairing time ───────
  // By now the SoftDevice has fully resolved the identity address.
  if (hasBondedPeer and not pairingMode)
  {
    // Check that address are matching
    if (!addressMatches(resolvedAddr, identityAddr))
    {
      bsp::lampda_print("[Security] Unknown device — disconnecting immediately");
      Bluefruit.disconnect(conn_hdl);
      return;
    }
  }
  else if (pairingMode)
  {
    // Check that address are matching
    if (hasBondedPeer and !addressMatches(resolvedAddr, identityAddr))
      bsp::lampda_print("[Security] First time pairing complete, peer authorized");
    else
      bsp::lampda_print("[Security] Recognized authorized peer, proceed");
  }

  // Only update if we now have a non-RPA identity address
  if (resolvedAddr.addr_type != BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE)
  {
    identityAddr = resolvedAddr;
    bsp::lampda_print("[Security] Updating the non-RPA identity");
  }
  // Save the handle: it's allowed to treat messages
  authorizedConnHdl = conn_hdl;

  // Send a battery level update
  const auto batteryLevel = component::battery::get_battery_minimum_cell_level();
  write_battery_level(static_cast<uint8_t>(batteryLevel / 100));

  // used !
  _wasUsed = true;

  // Stop advertising manually
  adv_stop_callback();
}

void connect_callback(uint16_t conn_hdl)
{
  if (not is_activated())
    return;

  bsp::lampda_print("[BLE] Connected (handle=%d)", conn_hdl);

  BLEConnection* conn = Bluefruit.Connection(conn_hdl);
  if (conn == NULL)
  {
    bsp::lampda_print("[Connect] connection handle invalid");
    return;
  }

  // skip the safety layer to reject devices fast !
  if (hasBondedPeer and not pairingMode)
  {
    bond_keys_t ltkey;
    if (not conn->loadBondKey(&ltkey))
    {
      Bluefruit.disconnect(conn_hdl);
      bsp::lampda_print("[Connect] Fast disconnect unallowed user");
      return;
    }
  }

  conn->requestPHY(); // Request 2Mbps PHY
  conn->requestMtuExchange(247);

  // Immediatly request pairing, or any msg will be rejected !
  conn->requestPairing();
}

void disconnect_callback(uint16_t conn_hdl, uint8_t reason)
{
  if (not is_activated())
    return;

  // this connection is dead, disconnect it
  if (authorizedConnHdl == conn_hdl)
  {
    authorizedConnHdl = BLE_CONN_HANDLE_INVALID;
  }

  // Dont stop advertising here, some BLE drivers can send one command by connections.
  // Fake call the advertising callback
  adv_stop_callback();

  bsp::lampda_print("Bluetooth disconnected (reason=0x%02X)", reason);
}

void set_device_informations()
{
  if (not is_activated())
    return;

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
  if (is_activated())
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
  bleuart.begin();
  bleElkService.begin();

  __private::load_bluetooth_name();

  //  Set the name we just made, and appearance
  Bluefruit.setName(bleName);
  Bluefruit.setAppearance(BLE_APPEARANCE_LIGHT_SOURCE_MULTICOLOR_ARRAY);

  // Configure and start the BLE Uart service
  bsp::lampda_print("Blutooth started under the name:%s", bleName);

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Advertise services that we want to advertise only
  Bluefruit.Advertising.addService(bleSystemInfo);
  // Bluefruit.Advertising.addService(bleBatteryService);
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.Advertising.addService(bleElkService);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.setStopCallback(adv_stop_callback);
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);             // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(ADV_TIMEOUT_FAST); // advertisement timeout

  Bluefruit.Security.setPairCompleteCallback(pair_complete_callback);
  Bluefruit.Security.setSecuredCallback(secured_connection_callback);
  Bluefruit.Security.setMITM(true); // Man In The Middle protection

  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // If not done yet
  load_bound_file();

  isInitialized = true;
}

} // namespace __private

/*
 *
 *
 */

bool is_activated() { return __private::isInitialized; }

bool is_advertising() { return is_activated() and Bluefruit.Advertising.isRunning(); }

bool is_open_to_all() { return is_activated() and __private::pairingMode; }

bool is_connected() { return is_activated() and Bluefruit.connected() != 0; }

bool is_bounded() { return __private::hasBondedPeer; }

bool is_bounded(std::array<uint8_t, 8>& buffer)
{
  if (__private::hasBondedPeer)
  {
    buffer[0] = __private::identityAddr.addr_type;
    buffer[1] = __private::identityAddr.addr[5];
    buffer[2] = __private::identityAddr.addr[4];
    buffer[3] = __private::identityAddr.addr[3];
    buffer[4] = __private::identityAddr.addr[2];
    buffer[5] = __private::identityAddr.addr[1];
    buffer[6] = __private::identityAddr.addr[0];
    return true;
  }
  return false;
}

void disconnect()
{
  if (is_connected())
    Bluefruit.disconnect(Bluefruit.connHandle());
}

bool is_connection_allowed(uint16_t connectionHandle)
{
  // refuse connections with the incorrect handle
  return is_activated() and connectionHandle != BLE_CONN_HANDLE_INVALID and
         connectionHandle == __private::authorizedConnHdl;
}

void clear_bounded_devices()
{
  // In some case, the file system may need to be restarted
  InternalFS.begin();

  InternalFS.remove(__private::BLE_BOUND_PEER_FILE);
  __private::identityAddr = {0};
  __private::hasBondedPeer = false;
  bsp::lampda_print("[Bond] Bond file cleared.");

  // Internal adafruit clear
  bond_clear_all();
}

// void display_infos() { Bluefruit.printInfo(); }

void load_bound_file()
{
  // Try to load the bounded peer if it exist
  ble_gap_addr_t boundAdress;
  if (__private::try_load_bounded_pair(boundAdress))
  {
    bsp::lampda_print("[Bond] Loaded address: %02X:%02X:%02X:%02X:%02X:%02X",
                      boundAdress.addr[5],
                      boundAdress.addr[4],
                      boundAdress.addr[3],
                      boundAdress.addr[2],
                      boundAdress.addr[1],
                      boundAdress.addr[0]);

    __private::identityAddr = boundAdress;
    __private::hasBondedPeer = true;
  }
  else
  {
    __private::identityAddr = {0};
    __private::hasBondedPeer = false;
  }
}

void init()
{
  // call once when the program starts
  __private::startup_sequence();
}

void start_advertising(bool allowUnknownConnections)
{
  init();

  // startup sequence can load a bound adress, so set the pairing mode after
  if (allowUnknownConnections)
  {
    // visual signal in case of a status update
    if (not __private::pairingMode)
    {
      // stop current advertizing
      __private::adv_stop_callback();
      logic::alerts::manager.raise(logic::alerts::Type::BLUETOOTH_ADVERT);
    }
    // force pairing mode, will accept any connection
    __private::pairingMode = true;

    __private::advertisingStoppedByRequest = false;
    Bluefruit.Advertising.start(ADV_TIMEOUT); // Stop advertising entirely after ADV_TIMEOUT seconds
    return;
  }

  // no need to start again
  if (is_advertising())
    return;

  __private::advertisingStoppedByRequest = false;

  Bluefruit.Advertising.start(ADV_TIMEOUT); // Stop advertising entirely after ADV_TIMEOUT seconds
}

void stop_bluetooth_advertising()
{
  if (not is_activated())
    return;

  __private::advertisingStoppedByRequest = true;
  __private::stop_advertising();
}

void write_battery_level(const uint8_t batteryLevel)
{
  if (not is_activated())
    return;
  __private::bleBatteryService.write(batteryLevel);
}

void notify_battery_level(const uint8_t batteryLevel)
{
  if (not is_activated())
    return;
  __private::bleBatteryService.notify(batteryLevel);
}

bool was_used() { return __private::_wasUsed; }

void shutdown()
{
  if (__private::hasBondedPeer)
    __private::try_save_bounded_peer(__private::identityAddr);

  __private::isInitialized = false;
  // NRF_RADIO->POWER = 0;
}

bool set_bluetooth_name(const std::array<char, MaxBleNameLenght>& name)
{
  // Try to load the bounded address file
  hal::filesystem::HAL_File nameFile;

  // Replace content
  if (nameFile.open(__private::BLE_NAME_FILE, hal::filesystem::HAL_File::OpenType::WRITE) and nameFile.is_open() and
      nameFile.seek(0))
  {
    nameFile.write((uint8_t*)name.data(), name.size());
    nameFile.close();
    return true;
  }

  nameFile.close();
  return false;
}

namespace serial {
bool is_activated()
{
  const bool isUartActivated =
          hal::bluetooth::is_activated() and Bluefruit.connected() and __private::bleuart.notifyEnabled();
  if (not isUartActivated)
    return false;

  // prevent stray enqueud messages
  if (not is_connection_allowed(__private::authorizedConnHdl))
  {
    __private::bleuart.flush();
    return false;
  }
  return true;
}

bool is_available() { return __private::bleuart.available() and is_connection_allowed(__private::authorizedConnHdl); }

char read() { return (char)__private::bleuart.read(); }

size_t write(const char* const buffer, size_t bufferSize) { return __private::bleuart.write(buffer, bufferSize); }

uint16_t mtu_size() { return Bluefruit.getMaxMtu(BLE_GAP_ROLE_PERIPH); }

} // namespace serial

} // namespace bluetooth
} // namespace hal
} // namespace lampda
