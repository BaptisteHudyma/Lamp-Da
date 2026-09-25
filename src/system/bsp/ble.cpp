#include "src/system/bsp/ble.h"

#include <cstdint>
#include <cstring>

#include "src/system/hal/ble.h"
#include "src/system/hal/filesystem.h"
#include "src/system/hal/registers.h"

#include "src/system/bsp/ble_services/elk_service.h"

#include "src/system/bsp/text_out.h"

#include "src/system/component/battery.h"

#include "src/system/logic/alerts.h"

namespace lampda {
namespace bsp {
namespace ble {

namespace __private {

#define ADV_TIMEOUT 30 // seconds. Set this higher to automatically stop advertising after a time

static constexpr const char* const BLE_BOUND_PEER_FILE = "/.ble_bound.par";
static constexpr const char* const BLE_NAME_FILE = "/.ble_name.par";

/// Indicates if the last advertising cancel command was automatic or requested
bool advertisingStoppedByRequest = false;
/// Keep track of the use
bool _wasUsed = false;
/// Paired device address
inline static hal::ble::gap_addr_t identityAddr = {0};
/// If true, the identityAddr is set, refuse all other connections
static bool hasBondedPeer = false;
/// If true, the device is in pairing mode, and can accept all connections
static volatile bool pairingMode = false;

/// Keep track of the currently authorized handle.
static uint16_t authorizedConnHdl = HAL_BLE_INVALID_HANDLE;

/**
 * \brief Try to load the bounded device adress from memory
 */
bool try_load_bounded_pair(hal::ble::gap_addr_t& addr)
{
  // Try to load the bounded address file
  hal::filesystem::HAL_File boundFile;
  if (boundFile.open(BLE_BOUND_PEER_FILE, hal::filesystem::HAL_File::OpenType::READ) and boundFile.is_open() and
      boundFile.is_available() and boundFile.seek(0))
  {
    const bool isAdressOk = (boundFile.read(reinterpret_cast<uint8_t*>(&addr), sizeof(hal::ble::gap_addr_t)) ==
                             sizeof(hal::ble::gap_addr_t));
    boundFile.close();
    return isAdressOk;
  }
  boundFile.close();
  return false;
}

void stop_advertising()
{
  __private::pairingMode = false;
  logic::alerts::manager.clear(logic::alerts::Type::BLUETOOTH_ADVERT);

  if (not is_activated())
    return;

  hal::ble::hal_ble_stop_advertising();
}

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
bool addressMatches(const hal::ble::gap_addr_t& a, const hal::ble::gap_addr_t& b)
{
  return (memcmp(a.addr, b.addr, HAL_BLE_GAP_ADDR_LEN) == 0);
}

bool load_bluetooth_name(char* bleName)
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
      return true;

    bsp::lampda_print("Invalid stored BLE name (%s: %d), skipping to default", nameLenght, bleName);
  }

  // Default name !
  const uint32_t MAC_ADDRESS_0 = hal::registers::get_mac_adress();

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
  return true;
}

void try_save_bounded_peer(const hal::ble::gap_addr_t& addr)
{
  // Try to load the bounded address file
  hal::filesystem::HAL_File boundFile;

  // load content, without deletion
  if (boundFile.open(BLE_BOUND_PEER_FILE, hal::filesystem::HAL_File::OpenType::WRITE) and boundFile.is_open() and
      boundFile.seek(0))
  {
    boundFile.write(reinterpret_cast<const uint8_t*>(&addr), sizeof(hal::ble::gap_addr_t));
  }
  boundFile.close();
}

} // namespace __private

namespace callbacks {

static void on_connect(hal::ble::hal_ble_conn_handle_t conn_handle, bool connected, uint8_t reason)
{
  if (not is_activated())
    return;

  bsp::lampda_print("[BLE] Connected (handle=%d)", conn_handle);

  // skip the safety layer to reject devices fast !
  if (__private::hasBondedPeer and not __private::pairingMode)
  {
    if (not hal::ble::hal_ble_can_load_bound_key(conn_handle))
    {
      hal::ble::hal_ble_disconnect(conn_handle);

      bsp::lampda_print("[Connect] Fast disconnect unallowed user");
      return;
    }
  }

  // TODO
  //  conn->requestPHY(); // Request 2Mbps PHY
  //  conn->requestMtuExchange(247);

  // Immediatly request pairing, or any msg will be rejected !
  hal::ble::hal_ble_start_pairing(conn_handle);
}

static void on_pairing_done(hal::ble::hal_ble_conn_handle_t conn_handle, bool connected)
{
  if (not is_activated())
    return;

  if (connected)
  {
    bsp::lampda_print("[Sec]: Pairing failed - disconnecting");
    hal::ble::hal_ble_disconnect(conn_handle);
    return;
  }

  hal::ble::gap_addr_t resolvedAddr = hal::ble::hal_ble_get_adress(conn_handle);
  if (resolvedAddr.type == HAL_BLE_GAP_ADDR_TYPE_INVALID)
  {
    bsp::lampda_print("[Security] could not get connected device address, skip");
    return;
  }

  __private::identityAddr = resolvedAddr;
  __private::hasBondedPeer = true;
  __private::pairingMode = false;

  bsp::lampda_print("[Bond] Stored identity type=0x%02X %02X:%02X:%02X:%02X:%02X:%02X",
                    __private::identityAddr.type,
                    __private::identityAddr.addr[5],
                    __private::identityAddr.addr[4],
                    __private::identityAddr.addr[3],
                    __private::identityAddr.addr[2],
                    __private::identityAddr.addr[1],
                    __private::identityAddr.addr[0]);
}

static void on_advertising_stops()
{
  // clear the alert
  logic::alerts::manager.clear(logic::alerts::Type::BLUETOOTH_ADVERT);

  // skip if bluetooth is off
  if (not is_activated())
    return;

  // auto turned off, start again !
  if (not __private::advertisingStoppedByRequest)
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
  __private::advertisingStoppedByRequest = false;
}

static void on_disconnect(hal::ble::hal_ble_conn_handle_t conn_handle, bool connected, uint8_t reason)
{
  if (not is_activated() or not connected)
    return;

  // this connection is dead, disconnect it
  if (__private::authorizedConnHdl == conn_handle)
  {
    __private::authorizedConnHdl = HAL_BLE_INVALID_HANDLE;
  }

  // Dont stop advertising here, some BLE drivers can send one command by connections.
  // Fake call the advertising callback
  on_advertising_stops();

  bsp::lampda_print("Bluetooth disconnected (reason=0x%02X)", reason);
}

static void on_secured(hal::ble::hal_ble_conn_handle_t conn_handle)
{
  if (not is_activated())
    return;

  const hal::ble::gap_addr_t& resolvedAddr = hal::ble::hal_ble_get_adress(conn_handle);
  if (resolvedAddr.type == HAL_BLE_GAP_ADDR_TYPE_INVALID)
  {
    bsp::lampda_print("[Security] could not get connected device address, skip");
    return;
  }

  bsp::lampda_print("[Security] Secure connection activated=0x%02X %02X:%02X:%02X:%02X:%02X:%02X",
                    resolvedAddr.type,
                    resolvedAddr.addr[5],
                    resolvedAddr.addr[4],
                    resolvedAddr.addr[3],
                    resolvedAddr.addr[2],
                    resolvedAddr.addr[1],
                    resolvedAddr.addr[0]);

  // ── Refresh the stored address if it was an RPA at pairing time ───────
  // By now the SoftDevice has fully resolved the identity address.
  if (__private::hasBondedPeer and not __private::pairingMode)
  {
    // Check that address are matching
    if (!__private::addressMatches(resolvedAddr, __private::identityAddr))
    {
      bsp::lampda_print("[Security] Unknown device — disconnecting immediately");
      hal::ble::hal_ble_disconnect(conn_handle);
      return;
    }
  }
  else if (__private::pairingMode)
  {
    // Check that address are matching
    if (__private::hasBondedPeer and !__private::addressMatches(resolvedAddr, __private::identityAddr))
      bsp::lampda_print("[Security] First time pairing complete, peer authorized");
    else
      bsp::lampda_print("[Security] Recognized authorized peer, proceed");
  }

  // Only update if we now have a non-RPA identity address
  if (resolvedAddr.type != HAL_BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE)
  {
    __private::identityAddr = resolvedAddr;
    bsp::lampda_print("[Security] Updating the non-RPA identity");
  }
  // Save the handle: it's allowed to treat messages
  __private::authorizedConnHdl = conn_handle;

  // Send a battery level update
  const auto batteryLevel = component::battery::get_battery_minimum_cell_level();
  write_battery_level(static_cast<uint8_t>(batteryLevel / 100));

  // used !
  __private::_wasUsed = true;

  // Stop advertising manually
  on_advertising_stops();
}

} // namespace callbacks

void load_bound_file()
{
  // Try to load the bounded peer if it exist
  hal::ble::gap_addr_t boundAdress;
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

bool is_activated() { return hal::ble::hal_ble_is_initialized(); }

bool is_advertising() { return hal::ble::hal_ble_is_advertising(); }

bool is_open_to_all() { return is_activated() and __private::pairingMode; }

bool is_connected() { return is_activated() and hal::ble::hal_ble_get_connection_count() > 0; }

bool is_bounded() { return __private::hasBondedPeer; }

bool is_bounded(std::array<uint8_t, 8>& buffer)
{
  if (__private::hasBondedPeer)
  {
    buffer[0] = __private::identityAddr.type;
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

void disconnect() { hal::ble::hal_ble_disconnect(hal::ble::hal_ble_get_connection_handle()); }

bool is_connection_allowed(uint16_t connectionHandle)
{
  // refuse connections with the incorrect handle
  return is_activated() and connectionHandle != HAL_BLE_INVALID_HANDLE and
         connectionHandle == __private::authorizedConnHdl;
}

void clear_bounded_devices()
{
  hal::filesystem::delete_file(__private::BLE_BOUND_PEER_FILE);
  __private::identityAddr = {0};
  __private::hasBondedPeer = false;
  bsp::lampda_print("[Bond] Bond file cleared.");

  hal::ble::hal_ble_clear_bonds();
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

void start_advertising(bool allowUnknownConnections)
{
  // startup sequence can load a bound adress, so set the pairing mode after
  if (allowUnknownConnections)
  {
    // visual signal in case of a status update
    if (not __private::pairingMode)
    {
      // stop current advertizing
      callbacks::on_advertising_stops();
      logic::alerts::manager.raise(logic::alerts::Type::BLUETOOTH_ADVERT);
    }
    // force pairing mode, will accept any connection
    __private::pairingMode = true;

    __private::advertisingStoppedByRequest = false;

    hal::ble::hal_ble_adv_params_t advertiseParams = {.interval_min_ms = 20,
                                                      .interval_max_ms = 100,
                                                      .include_tx_power = true,
                                                      .timeout_s = ADV_TIMEOUT,
                                                      .restartOnDisconnect = true};
    hal::ble::hal_ble_start_advertising(&advertiseParams);
    return;
  }

  // no need to start again
  if (is_advertising())
    return;

  __private::advertisingStoppedByRequest = false;

  hal::ble::hal_ble_adv_params_t advertiseParams = {.interval_min_ms = 20,
                                                    .interval_max_ms = 100,
                                                    .include_tx_power = true,
                                                    .timeout_s = ADV_TIMEOUT,
                                                    .restartOnDisconnect = true};
  hal::ble::hal_ble_start_advertising(&advertiseParams);
}

void stop_bluetooth_advertising()
{
  if (not is_activated())
    return;

  __private::advertisingStoppedByRequest = true;
  __private::stop_advertising();
}

void write_battery_level(const uint8_t batteryLevel) { hal::ble::services::battery::write_battery_level(batteryLevel); }

void notify_battery_level(const uint8_t batteryLevel)
{
  hal::ble::services::battery::notify_battery_level(batteryLevel);
}

bool was_used() { return __private::_wasUsed; }

void init()
{
  if (is_activated())
    return;

  hal::ble::hal_ble_event_callbacks_t callbacks = {
          .on_connect = callbacks::on_connect,
          .on_disconnect = callbacks::on_disconnect,
          .on_advertising_stops = callbacks::on_advertising_stops,
  };

  char bleName[MaxBleNameLenght];
  __private::load_bluetooth_name(bleName);

  // Init BLE stack
  hal::ble::hal_ble_init(bleName, &callbacks);

  // Security:
  hal::ble::hal_ble_sec_config_t security = {
          .on_pairing_done = callbacks::on_pairing_done,
          .on_secured = callbacks::on_secured,
          .mitm_required = true,
  };
  hal::ble::hal_ble_set_security_config(&security);

  // define ELK services and characs

  static const char firmwareRevision[] = {
          EXPECTED_FIRMWARE_VERSION_MAJOR + '0', '.', EXPECTED_FIRMWARE_VERSION_MINOR + '0', 0};
  static const char hardwareRevision[] = {HARDWARE_VERSION_MAJOR + '0', '.', HARDWARE_VERSION_MINOR + '0', 0};
  static const char softwareRevision[] = {USER_SOFTWARE_VERSION_MAJOR + '0', '.', USER_SOFTWARE_VERSION_MINOR + '0', 0};

#ifdef LMBD_LAMP_TYPE__SIMPLE
  const char* deviceType = "LAMPDA-SIMPLE";
#elif LMBD_LAMP_TYPE__CCT
  const char* deviceType = "LAMPDA-CCT";
#elif LMBD_LAMP_TYPE__INDEXABLE
  const char* deviceType = "LAMPDA-RGB";
#endif

  hal::ble::services::system_infos::Infos infos = {
          .model = deviceType,
          .firmwareRev = firmwareRevision,
          .hardwareRev = hardwareRevision,
          .softwareRev = softwareRevision,
          .manufacturer = "Lambda le fou",
  };

  hal::ble::services::system_infos::init(infos, true);
  hal::ble::services::battery::init(static_cast<uint8_t>(component::battery::get_battery_minimum_cell_level() / 100),
                                    false);
  hal::ble::services::uart::init(true);
  bsp::ble::services::init_elk_service();
}

void shutdown()
{
  if (__private::hasBondedPeer)
    __private::try_save_bounded_peer(__private::identityAddr);

  hal::ble::hal_ble_deinit();
}

namespace serial {

bool is_activated()
{
  const bool isUartActivated = is_connected() and hal::ble::services::uart::is_notified_enabled();
  if (not isUartActivated)
    return false;

  // prevent stray enqueud messages
  if (not is_connection_allowed(__private::authorizedConnHdl))
  {
    hal::ble::services::uart::flush();
    return false;
  }
  return true;
}

bool is_available()
{
  return hal::ble::services::uart::is_available() and is_connection_allowed(__private::authorizedConnHdl);
}

char read() { return hal::ble::services::uart::read(); }

size_t write(const char* const buffer, size_t bufferSize)
{
  return hal::ble::services::uart::write(buffer, bufferSize);
}

uint16_t mtu_size() { return hal::ble::hal_ble_get_mtu(__private::authorizedConnHdl); }

} // namespace serial

} // namespace ble
} // namespace bsp
} // namespace lampda
