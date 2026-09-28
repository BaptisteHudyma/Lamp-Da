#include "src/system/hal/ble.h"

#include <Arduino.h>
#include <bluefruit.h>
#include <cstring>

namespace lampda {
namespace hal {
namespace ble {

static struct
{
  bool initialized;
  const hal_ble_event_callbacks_t* callbacks;

  /* GATT database */
  struct gatt_type_t
  {
    BLECharacteristic* characteristic;
    hal_ble_read_callback_t read_cb;
    hal_ble_write_callback_t write_cb;
  };
  std::array<gatt_type_t, HAL_BLE_MAX_CHARACTERISTICS> gatt_map;
  uint8_t gatt_count;

  /* Connection state */
  struct connection_t
  {
    hal_ble_conn_handle_t handle;
    bool connected;
    uint16_t mtu;
  };
  std::array<connection_t, BLE_MAX_PERIPHERAL_CONNECTIONS> connections;

  /* Security */
  const hal_ble_sec_config_t* sec_config;
} hal_ble_state = {.initialized = false, .callbacks = nullptr, .gatt_count = 0, .sec_config = nullptr};

/* ========== Forward Declarations ========== */

static void adafruit_on_connect_callback(uint16_t conn_handle);
static void adafruit_on_disconnect_callback(uint16_t conn_handle, uint8_t reason);
static void adafruit_characteristic_write_callback(uint16_t conn_handle,
                                                   BLECharacteristic* chr,
                                                   uint8_t* data,
                                                   uint16_t len);
static void adafruit_on_pair_complete_callback(uint16_t conn_handle, uint8_t authStatus);
static void adafruit_on_secured_connection_callback(uint16_t conn_handle);
static void adafruit_on_advertising_stops();

/// hash a char pointer to a handle value
uint16_t get_characteristic_handle(BLECharacteristic* ble_chr) { return ((uintptr_t)ble_chr & 0xFFFF); }

/* ========== Initialization & Lifecycle ========== */

int32_t hal_ble_init(const char* device_name, const hal_ble_event_callbacks_t* callbacks)
{
  if (hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_ALREADY_INIT;
  }

  if (!device_name || !callbacks)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  /* Store callbacks */
  hal_ble_state.callbacks = callbacks;

  /* Initialize Bluefruit stack */
  if (!Bluefruit.begin())
  {
    return HAL_BLE_ERROR_GENERIC;
  }
  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values

  /* Set device name */
  Bluefruit.setName(device_name);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Register connection callbacks */
  Bluefruit.Periph.setConnectCallback(adafruit_on_connect_callback);
  Bluefruit.Periph.setDisconnectCallback(adafruit_on_disconnect_callback);

  Bluefruit.Advertising.setStopCallback(adafruit_on_advertising_stops);

  hal_ble_state.initialized = true;
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_deinit(void)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Stop advertising */
  Bluefruit.Advertising.stop();

  /* Disconnect all connections */
  if (Bluefruit.connected() != 0)
    Bluefruit.disconnect(hal_ble_get_connection_handle());

  /* Deinitialize Bluefruit */
  /// TODO: Bluefruit.end();

  /* Clear state */
  hal_ble_state.initialized = false;
  hal_ble_state.callbacks = nullptr;
  hal_ble_state.gatt_count = 0;

  for (auto& conn: hal_ble_state.connections)
  {
    conn.handle = 0;
    conn.connected = false;
    conn.mtu = 0;
  }

  return HAL_BLE_SUCCESS;
}

bool hal_ble_is_initialized(void) { return hal_ble_state.initialized; }

/* ========== GATT Database Setup ========== */

int32_t hal_ble_add_service(hal_ble_service_t* service)
{
  if (!service)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  if (hal_ble_state.gatt_count >= HAL_BLE_MAX_CHARACTERISTICS)
  {
    return HAL_BLE_ERROR_NO_MEMORY;
  }

  /* Create BLE service based on UUID type */
  static size_t servicesCnt = 0;
  static BLEService s_services[HAL_BLE_MAX_SERVICES];

  BLEService* ble_service = &s_services[servicesCnt];

  if (service->uuid.type == hal_ble_uuid_type_t::TYPE_16BIT)
  {
    ble_service->setUuid(service->uuid.value.uuid16);
  }
  else if (service->uuid.type == hal_ble_uuid_type_t::TYPE_128BIT)
  {
    ble_service->setUuid(service->uuid.value.uuid128);
  }
  else
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!ble_service->begin())
  {
    return HAL_BLE_ERROR_GENERIC;
  }

  servicesCnt++;
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_add_characteristic(hal_ble_characteristic_t* characteristic)
{
  if (!characteristic)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  if (hal_ble_state.gatt_count == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM; /* No service added yet */
  }

  if (hal_ble_state.gatt_count >= HAL_BLE_MAX_CHARACTERISTICS)
  {
    return HAL_BLE_ERROR_NO_MEMORY;
  }

  /* Convert HAL properties to Bluefruit properties */
  uint8_t props = 0;
  if (static_cast<uint8_t>(characteristic->properties) & static_cast<uint8_t>(hal_ble_gatt_prop_t::READ))
  {
    props |= CHR_PROPS_READ;
  }
  if (static_cast<uint8_t>(characteristic->properties) & static_cast<uint8_t>(hal_ble_gatt_prop_t::WRITE))
  {
    props |= CHR_PROPS_WRITE;
  }
  if (static_cast<uint8_t>(characteristic->properties) & static_cast<uint8_t>(hal_ble_gatt_prop_t::WRITE_NO_RSP))
  {
    props |= CHR_PROPS_WRITE_WO_RESP;
  }
  if (static_cast<uint8_t>(characteristic->properties) & static_cast<uint8_t>(hal_ble_gatt_prop_t::NOTIFY))
  {
    props |= CHR_PROPS_NOTIFY;
  }
  if (static_cast<uint8_t>(characteristic->properties) & static_cast<uint8_t>(hal_ble_gatt_prop_t::INDICATE))
  {
    props |= CHR_PROPS_INDICATE;
  }

  /* Convert HAL permissions to Bluefruit permissions */
  SecureMode_t readPermission = SECMODE_NO_ACCESS;
  if (static_cast<uint8_t>(characteristic->rPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::READ))
  {
    readPermission = SECMODE_OPEN;
  }
  if (static_cast<uint8_t>(characteristic->rPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::READ_ENCRYPTED))
  {
    readPermission = SECMODE_NO_ACCESS;
  }
  if (static_cast<uint8_t>(characteristic->rPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::WRITE))
  {
    readPermission = SECMODE_OPEN;
  }
  if (static_cast<uint8_t>(characteristic->rPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::WRITE_ENCRYPTED))
  {
    readPermission = SECMODE_NO_ACCESS;
  }
  SecureMode_t writePermission = SECMODE_NO_ACCESS;
  if (static_cast<uint8_t>(characteristic->wPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::READ))
  {
    writePermission = SECMODE_OPEN;
  }
  if (static_cast<uint8_t>(characteristic->wPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::READ_ENCRYPTED))
  {
    writePermission = SECMODE_NO_ACCESS;
  }
  if (static_cast<uint8_t>(characteristic->wPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::WRITE))
  {
    writePermission = SECMODE_OPEN;
  }
  if (static_cast<uint8_t>(characteristic->wPermissions) & static_cast<uint8_t>(hal_ble_gatt_perm_t::WRITE_ENCRYPTED))
  {
    writePermission = SECMODE_NO_ACCESS;
  }

  /* Create BLE characteristic */
  static BLECharacteristic s_chars[HAL_BLE_MAX_CHARACTERISTICS];
  BLECharacteristic* ble_chr = &s_chars[hal_ble_state.gatt_count];

  ble_chr->setProperties(props);
  ble_chr->setPermission(readPermission, writePermission);
  ble_chr->setMaxLen(characteristic->max_length);

  if (characteristic->uuid.type == hal_ble_uuid_type_t::TYPE_16BIT)
  {
    ble_chr->setUuid(characteristic->uuid.value.uuid16);
  }
  else if (characteristic->uuid.type == hal_ble_uuid_type_t::TYPE_128BIT)
  {
    ble_chr->setUuid(characteristic->uuid.value.uuid128);
  }
  else
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  /* Set initial value if provided */
  if (characteristic->initial_value && characteristic->initial_length > 0)
  {
    ble_chr->write(characteristic->initial_value, characteristic->initial_length);
  }

  /* Register write callback if provided */
  if (characteristic->write_cb)
  {
    ble_chr->setWriteCallback(adafruit_characteristic_write_callback);
    hal_ble_state.gatt_map[hal_ble_state.gatt_count].write_cb = characteristic->write_cb;
  }

  /* Store read callback if provided */
  if (characteristic->read_cb)
  {
    hal_ble_state.gatt_map[hal_ble_state.gatt_count].read_cb = characteristic->read_cb;
  }

  /* Add to last used service */
  if (ble_chr->begin() != ERROR_NONE)
  {
    return HAL_BLE_ERROR_GENERIC;
  }

  /* Store characteristic handle and mapping */
  characteristic->handle = get_characteristic_handle(ble_chr);
  hal_ble_state.gatt_map[hal_ble_state.gatt_count].characteristic = ble_chr;
  hal_ble_state.gatt_count++;
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_set_characteristic_value(hal_ble_char_handle_t char_handle, const uint8_t* data, uint16_t length)
{
  if (!data || length == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Find characteristic by handle */
  for (uint8_t i = 0; i < hal_ble_state.gatt_count; i++)
  {
    if (get_characteristic_handle(hal_ble_state.gatt_map[i].characteristic) == char_handle)
    {
      BLECharacteristic* chr = hal_ble_state.gatt_map[i].characteristic;
      chr->write(data, length);
      return HAL_BLE_SUCCESS;
    }
  }

  return HAL_BLE_ERROR_INVALID_PARAM;
}

int32_t hal_ble_get_characteristic_value(hal_ble_char_handle_t char_handle, uint8_t* data, uint16_t max_length)
{
  if (!data || max_length == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Find characteristic by handle */
  for (uint8_t i = 0; i < hal_ble_state.gatt_count; i++)
  {
    if (get_characteristic_handle(hal_ble_state.gatt_map[i].characteristic) == char_handle)
    {
      BLECharacteristic* chr = hal_ble_state.gatt_map[i].characteristic;
      uint16_t len = chr->read(data, max_length);
      return (int32_t)len;
    }
  }

  return HAL_BLE_ERROR_INVALID_PARAM;
}

/* ========== Notifications & Indications ========== */

int32_t hal_ble_notify(hal_ble_char_handle_t char_handle, const uint8_t* data, uint16_t length)
{
  if (!data || length == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Find characteristic by handle */
  for (uint8_t i = 0; i < hal_ble_state.gatt_count; i++)
  {
    if (get_characteristic_handle(hal_ble_state.gatt_map[i].characteristic) == char_handle)
    {
      BLECharacteristic* chr = hal_ble_state.gatt_map[i].characteristic;

      /* Bluefruit notify sends to all connected centrals */
      chr->notify(data, length);
      return HAL_BLE_SUCCESS;
    }
  }

  return HAL_BLE_ERROR_INVALID_PARAM;
}

int32_t hal_ble_indicate(hal_ble_conn_handle_t conn_handle,
                         hal_ble_char_handle_t char_handle,
                         const uint8_t* data,
                         uint16_t length)
{
  if (!data || length == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Find characteristic by handle */
  for (uint8_t i = 0; i < hal_ble_state.gatt_count; i++)
  {
    if (get_characteristic_handle(hal_ble_state.gatt_map[i].characteristic) == char_handle)
    {
      BLECharacteristic* chr = hal_ble_state.gatt_map[i].characteristic;

      /* Bluefruit indicate sends to specific connection */
      chr->indicate(data, length);
      return HAL_BLE_SUCCESS;
    }
  }

  return HAL_BLE_ERROR_INVALID_PARAM;
}

/* ========== Advertising Control ========== */

int32_t hal_ble_start_advertising(const hal_ble_adv_params_t* adv_params)
{
  if (!adv_params)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  if (hal_ble_is_advertising())
  {
    return HAL_BLE_SUCCESS; /* Already advertising */
  }

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  if (adv_params->include_tx_power)
    Bluefruit.Advertising.addTxPower();

  /* Set advertising interval (Bluefruit expects interval in units of 0.625ms) */
  uint16_t interval_ms = adv_params->interval_min_ms;
  uint16_t interval_units = (interval_ms * 8) / 5; /* Convert ms to 0.625ms units */

  Bluefruit.Advertising.setInterval(interval_units, interval_units);
  Bluefruit.Advertising.restartOnDisconnect(adv_params->restartOnDisconnect);

  /* Start advertising */
  if (!Bluefruit.Advertising.start(adv_params->timeout_s))
  {
    return HAL_BLE_ERROR_GENERIC;
  }

  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_stop_advertising(void)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }
  if (!hal_ble_is_advertising())
  {
    return HAL_BLE_SUCCESS; /* Already advertising */
  }

  if (!Bluefruit.Advertising.stop())
    return HAL_BLE_ERROR_GENERIC;

  return HAL_BLE_SUCCESS;
}

bool hal_ble_is_advertising(void)
{
  if (!hal_ble_state.initialized)
  {
    return false;
  }

  return Bluefruit.Advertising.isRunning();
}

/* ========== Connection Management ========== */

int32_t hal_ble_disconnect(hal_ble_conn_handle_t conn_handle)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  if (conn_handle == HAL_BLE_INVALID_HANDLE)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  /* Bluefruit: disconnect specific connection */
  Bluefruit.disconnect(conn_handle);

  return HAL_BLE_SUCCESS;
}

hal_ble_conn_handle_t hal_ble_get_connection_handle() { return Bluefruit.connHandle(); }

int32_t hal_ble_get_mtu(hal_ble_conn_handle_t conn_handle)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  if (conn_handle == HAL_BLE_INVALID_HANDLE)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  /* Find connection and return MTU */
  for (uint8_t i = 0; i < BLE_MAX_PERIPHERAL_CONNECTIONS; i++)
  {
    if (hal_ble_state.connections[i].handle == conn_handle && hal_ble_state.connections[i].connected)
    {
      // This is not the "true" current MTU, but the max supported by the BLE layer
      return (int32_t)Bluefruit.getMaxMtu(BLE_GAP_ROLE_PERIPH);
    }
  }

  return HAL_BLE_ERROR_INVALID_PARAM;
}

int32_t hal_ble_request_mtu_exchange(hal_ble_conn_handle_t conn_handle, uint16_t desired_mtu)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  if (conn_handle == HAL_BLE_INVALID_HANDLE)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (desired_mtu < HAL_BLE_MIN_MTU || desired_mtu > HAL_BLE_MAX_MTU)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  // request MTU exchange
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  if (conn == nullptr)
  {
    return HAL_BLE_ERROR_GENERIC;
  }

  if (!conn->requestMtuExchange(desired_mtu))
    return HAL_BLE_ERROR_GENERIC;

  // update stored MTU
  /// TODO: this can fail if the host refuses
  for (uint8_t i = 0; i < BLE_MAX_PERIPHERAL_CONNECTIONS; i++)
  {
    if (hal_ble_state.connections[i].handle == conn_handle)
    {
      hal_ble_state.connections[i].mtu = desired_mtu;
      return HAL_BLE_SUCCESS;
    }
  }
  return HAL_BLE_ERROR_GENERIC;
}

/* ========== Security & Pairing ========== */

int32_t hal_ble_set_security_config(const hal_ble_sec_config_t* sec_config)
{
  if (!sec_config)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Store security config */
  hal_ble_state.sec_config = sec_config;

  Bluefruit.Security.setPairCompleteCallback(adafruit_on_pair_complete_callback);
  Bluefruit.Security.setSecuredCallback(adafruit_on_secured_connection_callback);
  Bluefruit.Security.setMITM(sec_config->mitm_required); // Man In The Middle protection

  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_start_pairing(hal_ble_conn_handle_t conn_handle)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  if (conn_handle == HAL_BLE_INVALID_HANDLE)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  /* Bluefruit: initiate pairing */
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  if (conn == nullptr)
  {
    return HAL_BLE_ERROR_GENERIC;
  }
  conn->requestPairing();

  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_clear_bonds(void)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  // Internal adafruit clear
  bond_clear_all();

  return HAL_BLE_SUCCESS;
}

/* ========== Device Information ========== */

int32_t hal_ble_set_device_name(const char* name)
{
  const auto strLenght = strlen(name);
  if (!name || strLenght == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (strLenght > HAL_BLE_MAX_DEVICE_NAME_LEN)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  Bluefruit.setName(name);
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_get_device_name(char* name, uint16_t max_length)
{
  if (!name || max_length == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  char dev_name[64];
  uint16_t len = Bluefruit.getName(dev_name, 32);
  if (len > max_length)
  {
    len = max_length;
  }

  strncpy(name, dev_name, len);
  name[len] = '\0';

  return (int32_t)len;
}

int32_t hal_ble_set_tx_power(int8_t tx_power_dbm)
{
  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Bluefruit: set TX power */
  Bluefruit.setTxPower(tx_power_dbm);

  return HAL_BLE_SUCCESS;
}

int8_t hal_ble_get_tx_power(void)
{
  if (!hal_ble_state.initialized)
  {
    return 0;
  }

  /* Bluefruit: get TX power (may not be available; return 0 as default) */
  return 0; /* Placeholder: Bluefruit doesn't expose getter directly */
}

/* ========== Debugging & Status ========== */

uint8_t hal_ble_get_connection_count(void)
{
  if (!hal_ble_state.initialized)
  {
    return 0;
  }

  return Bluefruit.connected();
}

bool hal_ble_is_connected(hal_ble_conn_handle_t conn_handle)
{
  if (!hal_ble_state.initialized)
  {
    return false;
  }

  return Bluefruit.Periph.connected(conn_handle);
}

int8_t hal_ble_get_rssi(hal_ble_conn_handle_t conn_handle)
{
  if (!hal_ble_state.initialized)
  {
    return 0;
  }

  if (conn_handle == HAL_BLE_INVALID_HANDLE)
  {
    return 0;
  }

  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  if (conn == nullptr)
  {
    return 0;
  }

  /* Bluefruit: get RSSI */
  return conn->getRssi();
}

gap_addr_t hal_ble_get_address(hal_ble_conn_handle_t conn_handle)
{
  gap_addr_t addr = {0};
  addr.type = HAL_BLE_GAP_ADDR_TYPE_INVALID;
  if (!hal_ble_state.initialized)
  {
    return addr;
  }

  if (conn_handle == HAL_BLE_INVALID_HANDLE)
  {
    return addr;
  }

  if (!Bluefruit.Periph.connected(conn_handle))
  {
    return addr;
  }

  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  if (conn == nullptr)
  {
    return addr;
  }

  const auto& resolvedAddr = conn->getPeerAddr();
  addr.type = resolvedAddr.addr_type;
  for (uint8_t i = 0; i < BLE_GAP_ADDR_LEN; i++)
    addr.addr[i] = resolvedAddr.addr[i];
  return addr;
}

bool hal_ble_can_load_bound_key(hal_ble_conn_handle_t conn_handle)
{
  if (!hal_ble_state.initialized)
  {
    return false;
  }

  if (conn_handle == HAL_BLE_INVALID_HANDLE)
  {
    return false;
  }

  if (!Bluefruit.Periph.connected(conn_handle))
  {
    return false;
  }

  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  if (conn == nullptr)
  {
    return false;
  }

  bond_keys_t ltkey;
  return conn->loadBondKey(&ltkey);
}

/* ========== Platform Capabilities ========== */

bool hal_ble_has_feature(hal_ble_feature_t feature)
{
  switch (feature)
  {
    case hal_ble_feature_t::HAL_BLE_FEATURE_BONDING:
      return true;
    case hal_ble_feature_t::HAL_BLE_FEATURE_LE_SECURE_CONNECTIONS:
      return true; /* NRF52840 supports LE Secure Connections */
    case hal_ble_feature_t::HAL_BLE_FEATURE_DLE:
      return true; /* Data Length Extension available */
    case hal_ble_feature_t::HAL_BLE_FEATURE_MULTI_ROLE:
      return true; /* NRF52840 supports simultaneous roles */
    default:
      return false;
  }
}

/* ========== Internal Callbacks ========== */

static void adafruit_on_connect_callback(uint16_t conn_handle)
{
  if (!hal_ble_state.callbacks || !hal_ble_state.callbacks->on_connect)
  {
    return;
  }

  // Store connection handle
  for (auto& connection: hal_ble_state.connections)
  {
    if (!connection.connected)
    {
      connection.handle = conn_handle;
      connection.connected = true;
      connection.mtu = HAL_BLE_MIN_MTU;
      break;
    }
  }

  hal_ble_state.callbacks->on_connect(conn_handle);
}

static void adafruit_on_disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  if (!hal_ble_state.callbacks || !hal_ble_state.callbacks->on_disconnect)
  {
    return;
  }

  /* Remove connection handle */
  for (auto& connection: hal_ble_state.connections)
  {
    if (connection.handle == conn_handle)
    {
      connection.connected = false;
      break;
    }
  }

  hal_ble_state.callbacks->on_disconnect(conn_handle, reason);
}

static void adafruit_characteristic_write_callback(uint16_t conn_handle,
                                                   BLECharacteristic* chr,
                                                   uint8_t* data,
                                                   uint16_t len)
{
  if (!chr || !data)
  {
    return;
  }

  /* Find characteristic in mapping and call user callback */
  for (uint8_t i = 0; i < hal_ble_state.gatt_count; i++)
  {
    if (hal_ble_state.gatt_map[i].characteristic == chr && hal_ble_state.gatt_map[i].write_cb)
    {
      hal_ble_state.gatt_map[i].write_cb(conn_handle, data, len);
      break;
    }
  }
}

static void adafruit_on_pair_complete_callback(uint16_t conn_handle, uint8_t authStatus)
{
  if (!hal_ble_state.sec_config || !hal_ble_state.sec_config->on_pairing_done)
  {
    return;
  }
  hal_ble_state.sec_config->on_pairing_done(conn_handle, authStatus != 0);
}

static void adafruit_on_secured_connection_callback(uint16_t conn_handle)
{
  if (!hal_ble_state.sec_config || !hal_ble_state.sec_config->on_secured)
  {
    return;
  }
  hal_ble_state.sec_config->on_secured(conn_handle);
}

static void adafruit_on_advertising_stops()
{
  if (!hal_ble_state.callbacks || !hal_ble_state.callbacks->on_advertising_stops)
  {
    return;
  }
  hal_ble_state.callbacks->on_advertising_stops();
}

namespace services {

namespace battery {

/// System battery service
BLEBas bleBatteryService;

void init(const uint8_t batteryLevel, const bool addToAdvertised)
{
  bleBatteryService.begin();
  write_battery_level(batteryLevel);

  if (addToAdvertised)
    Bluefruit.Advertising.addService(bleBatteryService);
}

void write_battery_level(const uint8_t batteryLevel) { bleBatteryService.write(batteryLevel); }

void notify_battery_level(const uint8_t batteryLevel) { bleBatteryService.notify(batteryLevel); }

} // namespace battery

namespace uart {

/// UART over BLE
BLEUart bleuart;

void init(const bool addToAdvertised)
{
  bleuart.begin();

  if (addToAdvertised)
    Bluefruit.Advertising.addService(bleuart);
}

bool is_notified_enabled() { return bleuart.notifyEnabled(); }

bool is_available() { return bleuart.available(); }

char read() { return (char)bleuart.read(); }

void flush() { bleuart.flush(); }

size_t write(const char* const buffer, size_t bufferSize) { return bleuart.write(buffer, bufferSize); }

} // namespace uart

namespace system_infos {

BLEDis bleSystemInfo;

void init(const Infos& sysInfos, const bool addToAdvertised)
{
  bleSystemInfo.setModel(sysInfos.model);
  bleSystemInfo.setFirmwareRev(sysInfos.firmwareRev);
  bleSystemInfo.setHardwareRev(sysInfos.hardwareRev);
  bleSystemInfo.setSoftwareRev(sysInfos.softwareRev);
  bleSystemInfo.setManufacturer(sysInfos.manufacturer);

  bleSystemInfo.begin();

  if (addToAdvertised)
    Bluefruit.Advertising.addService(bleSystemInfo);
}

} // namespace system_infos

} // namespace services

} // namespace ble
} // namespace hal
} // namespace lampda
