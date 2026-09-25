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
  struct
  {
    BLEService* service;
    BLECharacteristic* characteristic;
    hal_ble_read_callback_t read_cb;
    hal_ble_write_callback_t write_cb;
  } gatt_map[HAL_BLE_MAX_CHARACTERISTICS];
  uint8_t gatt_count;

  /* Connection state */
  struct
  {
    hal_ble_conn_handle_t handle;
    bool connected;
    uint16_t mtu;
  } connections[BLE_MAX_PERIPHERAL_CONNECTIONS];

  /* Security */
  const hal_ble_sec_config_t* sec_config;
} hal_ble_state = {.initialized = false, .callbacks = NULL, .gatt_count = 0, .sec_config = NULL};

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
  hal_ble_state.callbacks = NULL;
  hal_ble_state.gatt_count = 0;
  memset(hal_ble_state.connections, 0, sizeof(hal_ble_state.connections));

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
  BLEService* ble_service = NULL;

  if (service->uuid.type == HAL_BLE_UUID_TYPE_16BIT)
  {
    ble_service = new BLEService(service->uuid.value.uuid16);
  }
  else if (service->uuid.type == HAL_BLE_UUID_TYPE_128BIT)
  {
    ble_service = new BLEService(service->uuid.value.uuid128);
  }
  else
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!ble_service->begin())
  {
    delete ble_service;
    return HAL_BLE_ERROR_GENERIC;
  }

  /* Store service handle (opaque to application) */
  service->service_handle = (uint16_t)((uintptr_t)ble_service & 0xFFFF);

  /* Store for later characteristic addition */
  hal_ble_state.gatt_map[hal_ble_state.gatt_count].service = ble_service;

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
  if (characteristic->properties & HAL_BLE_GATT_PROP_READ)
  {
    props |= CHR_PROPS_READ;
  }
  if (characteristic->properties & HAL_BLE_GATT_PROP_WRITE)
  {
    props |= CHR_PROPS_WRITE;
  }
  if (characteristic->properties & HAL_BLE_GATT_PROP_WRITE_NO_RSP)
  {
    props |= CHR_PROPS_WRITE_WO_RESP;
  }
  if (characteristic->properties & HAL_BLE_GATT_PROP_NOTIFY)
  {
    props |= CHR_PROPS_NOTIFY;
  }
  if (characteristic->properties & HAL_BLE_GATT_PROP_INDICATE)
  {
    props |= CHR_PROPS_INDICATE;
  }

  /* Convert HAL permissions to Bluefruit permissions */
  uint8_t perms = 0;
  if (characteristic->permissions & HAL_BLE_GATT_PERM_READ)
  {
    perms |= SECMODE_OPEN;
  }
  if (characteristic->permissions & HAL_BLE_GATT_PERM_READ_ENCRYPTED)
  {
    perms |= SECMODE_NO_ACCESS;
  }
  if (characteristic->permissions & HAL_BLE_GATT_PERM_WRITE)
  {
    perms |= SECMODE_OPEN;
  }
  if (characteristic->permissions & HAL_BLE_GATT_PERM_WRITE_ENCRYPTED)
  {
    perms |= SECMODE_NO_ACCESS;
  }

  /* Create BLE characteristic */
  BLECharacteristic* ble_chr = NULL;

  if (characteristic->uuid.type == HAL_BLE_UUID_TYPE_16BIT)
  {
    ble_chr = new BLECharacteristic(characteristic->uuid.value.uuid16, props, perms, characteristic->max_length);
  }
  else if (characteristic->uuid.type == HAL_BLE_UUID_TYPE_128BIT)
  {
    ble_chr = new BLECharacteristic(characteristic->uuid.value.uuid128, props, perms, characteristic->max_length);
  }
  else
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!ble_chr)
  {
    return HAL_BLE_ERROR_NO_MEMORY;
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
  characteristic->handle = (uint16_t)((uintptr_t)ble_chr & 0xFFFF);
  hal_ble_state.gatt_map[hal_ble_state.gatt_count].characteristic = ble_chr;
  hal_ble_state.gatt_count++;

  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_set_characteristic_value(hal_ble_char_handle_t char_handle, const uint8_t* data, size_t length)
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
    if ((uint16_t)((uintptr_t)hal_ble_state.gatt_map[i].characteristic & 0xFFFF) == char_handle)
    {
      BLECharacteristic* chr = hal_ble_state.gatt_map[i].characteristic;
      chr->write(data, length);
      return HAL_BLE_SUCCESS;
    }
  }

  return HAL_BLE_ERROR_INVALID_PARAM;
}

int32_t hal_ble_get_characteristic_value(hal_ble_char_handle_t char_handle, uint8_t* data, size_t max_length)
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
    if ((uint16_t)((uintptr_t)hal_ble_state.gatt_map[i].characteristic & 0xFFFF) == char_handle)
    {
      BLECharacteristic* chr = hal_ble_state.gatt_map[i].characteristic;
      uint16_t len = chr->read(data, max_length);
      return (int32_t)len;
    }
  }

  return HAL_BLE_ERROR_INVALID_PARAM;
}

/* ========== Notifications & Indications ========== */

int32_t hal_ble_notify(hal_ble_char_handle_t char_handle, const uint8_t* data, size_t length)
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
    if ((uint16_t)((uintptr_t)hal_ble_state.gatt_map[i].characteristic & 0xFFFF) == char_handle)
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
                         size_t length)
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
    if ((uint16_t)((uintptr_t)hal_ble_state.gatt_map[i].characteristic & 0xFFFF) == char_handle)
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
  uint32_t interval_ms = adv_params->interval_min_ms;
  uint32_t interval_units = (interval_ms * 8) / 5; /* Convert ms to 0.625ms units */

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
      return (int32_t)hal_ble_state.connections[i].mtu;
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
  if (conn == NULL)
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
  if (conn == NULL)
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
  if (!name || strlen(name) == 0)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (strlen(name) > HAL_BLE_MAX_DEVICE_NAME_LEN)
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

int32_t hal_ble_get_device_name(char* name, size_t max_length)
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
  size_t len = Bluefruit.getName(dev_name, 32);
  if (len > max_length)
  {
    len = max_length;
  }

  strncpy(name, dev_name, len);
  name[len] = '\0';

  return (int32_t)len;
}

int32_t hal_ble_get_address(uint8_t addr[6])
{
  if (!addr)
  {
    return HAL_BLE_ERROR_INVALID_PARAM;
  }

  if (!hal_ble_state.initialized)
  {
    return HAL_BLE_ERROR_NOT_INIT;
  }

  /* Bluefruit: get MAC address */
  uint8_t mac[6];
  Bluefruit.getAddr(mac);
  memcpy(addr, mac, 6);

  return HAL_BLE_SUCCESS;
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

  /* Bluefruit: get RSSI */
  return Bluefruit.Connection(conn_handle)->getRssi();
}

gap_addr_t hal_ble_get_adress(hal_ble_conn_handle_t conn_handle)
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
  if (conn == NULL)
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
  if (conn == NULL)
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
    case HAL_BLE_FEATURE_BONDING:
      return true;
    case HAL_BLE_FEATURE_LE_SECURE_CONNECTIONS:
      return true; /* NRF52840 supports LE Secure Connections */
    case HAL_BLE_FEATURE_DLE:
      return true; /* Data Length Extension available */
    case HAL_BLE_FEATURE_MULTI_ROLE:
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

  /* Store connection handle */
  for (uint8_t i = 0; i < BLE_MAX_PERIPHERAL_CONNECTIONS; i++)
  {
    if (!hal_ble_state.connections[i].connected)
    {
      hal_ble_state.connections[i].handle = conn_handle;
      hal_ble_state.connections[i].connected = true;
      hal_ble_state.connections[i].mtu = HAL_BLE_MIN_MTU;
      break;
    }
  }

  hal_ble_state.callbacks->on_connect(conn_handle, true, 0);
}

static void adafruit_on_disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  if (!hal_ble_state.callbacks || !hal_ble_state.callbacks->on_disconnect)
  {
    return;
  }

  /* Remove connection handle */
  for (uint8_t i = 0; i < BLE_MAX_PERIPHERAL_CONNECTIONS; i++)
  {
    if (hal_ble_state.connections[i].handle == conn_handle)
    {
      hal_ble_state.connections[i].connected = false;
      break;
    }
  }

  hal_ble_state.callbacks->on_disconnect(conn_handle, false, reason);
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
      hal_ble_char_handle_t char_handle = (uint16_t)((uintptr_t)chr & 0xFFFF);
      hal_ble_state.gatt_map[i].write_cb(conn_handle, char_handle, data, len, 0);
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
