#include "src/system/hal/ble.h"

#include <cstring>

namespace lampda {
namespace hal {
namespace ble {

int32_t hal_ble_init(const char* device_name, const hal_ble_event_callbacks_t* callbacks) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_deinit(void) { return HAL_BLE_SUCCESS; }

bool hal_ble_is_initialized(void) { return true; }

int32_t hal_ble_add_service(hal_ble_service_t* service) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_add_characteristic(hal_ble_characteristic_t* characteristic) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_set_characteristic_value(hal_ble_char_handle_t char_handle, const uint8_t* data, size_t length)
{
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_get_characteristic_value(hal_ble_char_handle_t char_handle, uint8_t* data, size_t max_length)
{
  return HAL_BLE_ERROR_GENERIC;
}

int32_t hal_ble_notify(hal_ble_char_handle_t char_handle, const uint8_t* data, size_t length)
{
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_indicate(hal_ble_conn_handle_t conn_handle,
                         hal_ble_char_handle_t char_handle,
                         const uint8_t* data,
                         size_t length)
{
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_start_advertising(const hal_ble_adv_params_t* adv_params) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_stop_advertising(void) { return HAL_BLE_SUCCESS; }

bool hal_ble_is_advertising(void) { return false; }

int32_t hal_ble_set_conn_params(const hal_ble_conn_params_t* params) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_disconnect(hal_ble_conn_handle_t conn_handle) { return HAL_BLE_SUCCESS; }

hal_ble_conn_handle_t hal_ble_get_connection_handle() { return 0; }

int32_t hal_ble_get_mtu(hal_ble_conn_handle_t conn_handle) { return 25; }

int32_t hal_ble_request_mtu_exchange(hal_ble_conn_handle_t conn_handle, uint16_t desired_mtu)
{
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_set_security_config(const hal_ble_sec_config_t* sec_config) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_start_pairing(hal_ble_conn_handle_t conn_handle) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_confirm_pairing(hal_ble_conn_handle_t conn_handle, bool accept) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_clear_bonds(void) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_set_device_name(const char* name) { return HAL_BLE_SUCCESS; }

int32_t hal_ble_get_device_name(char* name, size_t max_length)
{
  /// TODO
  char devName[] = "lampda_ble";
  strncpy(name, devName, 11);
  name[11] = '\0';
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_get_address(uint8_t addr[6])
{
  addr[0] = 0;
  addr[1] = 0;
  addr[2] = 0;
  addr[3] = 0;
  addr[4] = 0;
  addr[5] = 0;
  return HAL_BLE_SUCCESS;
}

int32_t hal_ble_set_tx_power(int8_t tx_power_dbm) { return HAL_BLE_SUCCESS; }

int8_t hal_ble_get_tx_power(void) { return 4; }

uint8_t hal_ble_get_connection_count(void) { return 0; }

bool hal_ble_is_connected(hal_ble_conn_handle_t conn_handle) { return false; }

int8_t hal_ble_get_rssi(hal_ble_conn_handle_t conn_handle) { return 1; }

gap_addr_t hal_ble_get_adress(hal_ble_conn_handle_t conn_handle)
{
  gap_addr_t addr = {0};
  addr.type = HAL_BLE_GAP_ADDR_TYPE_INVALID;
  return addr;
}

bool hal_ble_can_load_bound_key(hal_ble_conn_handle_t conn_handle) { return false; }

bool hal_ble_has_feature(hal_ble_feature_t feature) { return false; }

namespace services {

/// OTA battery level service
namespace battery {

/// Init the service, with an initial value (0-100)
void init(const uint8_t batteryLevel, const bool addToAdvertised) {}

/// update battery level (0-100)
void write_battery_level(const uint8_t batteryLevel) {}

/// notify the connected device of a battery level (0-100)
void notify_battery_level(const uint8_t batteryLevel) {}

} // namespace battery

/// OTA UART connection
namespace uart {

/// Start the uart service
void init(const bool addToAdvertised) {}

/// Return true uart notify is activated
bool is_notified_enabled() { return false; }

/// Return true if a char is available to read
bool is_available() { return false; }

/// Read a character (blocking)
char read() { return ' '; }

/// Flush the rx queue
void flush() {}

/// Write a buffer to connected device
size_t write(const char* const buffer, size_t bufferSize) { return 0; }

} // namespace uart

namespace system_infos {

void init(const Infos& sysInfos, const bool addToAdvertised) {}

} // namespace system_infos

} // namespace services

} // namespace ble
} // namespace hal
} // namespace lampda
