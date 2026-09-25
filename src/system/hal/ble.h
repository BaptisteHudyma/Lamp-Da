/**
 * @file ble.h
 * @brief Hardware Abstraction Layer for Bluetooth Low Energy (BLE) Peripheral Role
 *
 * This header defines a platform-agnostic BLE Peripheral interface compatible with:
 *  - Adafruit NRF52840 (Arduino SDK)
 *  - Zephyr RTOS (NRF52840)
 *  - Linux (simulation/testing via BlueZ)
 *
 * Peripheral Role: Device advertises and accepts connections from Central devices.
 *
 * Terminology:
 *  - BLE: Bluetooth Low Energy (wireless protocol)
 *  - GATT: Generic Attribute Profile (service/characteristic structure)
 *  - GAP: Generic Access Profile (advertising, connection parameters)
 *  - UUID: Universally Unique Identifier (128-bit or 16-bit service/characteristic ID)
 *  - MTU: Maximum Transmission Unit (max data per BLE packet)
 */

#ifndef HAL_BLE_H
#define HAL_BLE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

namespace lampda {
namespace hal {
/// Handle the platform specific bluetooth operations
namespace ble {

//
#define BLE_MAX_PERIPHERAL_CONNECTIONS 1

/* ========== Error Codes ========== */
#define HAL_BLE_SUCCESS             (0)
#define HAL_BLE_ERROR_GENERIC       (-1)
#define HAL_BLE_ERROR_INVALID_PARAM (-2)
#define HAL_BLE_ERROR_NOT_SUPPORTED (-3)
#define HAL_BLE_ERROR_NO_MEMORY     (-4)
#define HAL_BLE_ERROR_BUSY          (-5)
#define HAL_BLE_ERROR_TIMEOUT       (-6)
#define HAL_BLE_ERROR_ALREADY_INIT  (-7)
#define HAL_BLE_ERROR_NOT_INIT      (-8)

/** @brief BLE address length. */
#define HAL_BLE_GAP_ADDR_LEN (6)

/**@defgroup HAL_BLE_GAP_ADDR_TYPES GAP Address types
 * @{ */
#define HAL_BLE_GAP_ADDR_TYPE_INVALID                       0x0F /**< Public (identity) address.*/
#define HAL_BLE_GAP_ADDR_TYPE_PUBLIC                        0x00 /**< Public (identity) address.*/
#define HAL_BLE_GAP_ADDR_TYPE_RANDOM_STATIC                 0x01 /**< Random static (identity) address. */
#define HAL_BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE     0x02 /**< Random private resolvable address. */
#define HAL_BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE 0x03 /**< Random private non-resolvable address. */
#define HAL_BLE_GAP_ADDR_TYPE_ANONYMOUS                      \
  0x7F /**< An advertiser may advertise without its address. \
        This type of advertising is called anonymous. */
/**@} */

typedef struct
{
  uint8_t type;                       ///< address type in HAL_BLE_GAP_ADDR_TYPE_*
  uint8_t addr[HAL_BLE_GAP_ADDR_LEN]; ///< 48-bit address, LSB format
} gap_addr_t;

/* ========== UUID Handling ========== */

/**
 * @brief UUID (Universally Unique Identifier) type
 *
 * Supports both:
 *  - 16-bit UUIDs (BLE standard shortcuts, e.g., 0x180A for Device Information)
 *  - 128-bit UUIDs (vendor-specific custom services)
 */
typedef enum
{
  HAL_BLE_UUID_TYPE_16BIT, /**< Standard BLE 16-bit UUID */
  HAL_BLE_UUID_TYPE_128BIT /**< Vendor-specific 128-bit UUID */
} hal_ble_uuid_type_t;

/**
 * @brief UUID structure (supports both 16-bit and 128-bit)
 */
typedef struct
{
  hal_ble_uuid_type_t type;
  union
  {
    uint16_t uuid16;     /**< 16-bit UUID (little-endian) */
    uint8_t uuid128[16]; /**< 128-bit UUID (little-endian, LSB first) */
  } value;
} hal_ble_uuid_t;

/* ========== GATT (Generic Attribute Profile) ========== */

/**
 * @brief GATT (Generic Attribute Profile) Attribute Permissions
 *
 * Controls who can read/write characteristic values.
 */
typedef enum
{
  HAL_BLE_GATT_PERM_READ = 0x01,            /**< Client can read */
  HAL_BLE_GATT_PERM_READ_ENCRYPTED = 0x02,  /**< Read only over encrypted link */
  HAL_BLE_GATT_PERM_WRITE = 0x04,           /**< Client can write */
  HAL_BLE_GATT_PERM_WRITE_ENCRYPTED = 0x08, /**< Write only over encrypted link */
} hal_ble_gatt_perm_t;

/**
 * @brief GATT Characteristic Properties
 *
 * Defines how clients interact with a characteristic.
 */
typedef enum
{
  HAL_BLE_GATT_PROP_READ = 0x01,         /**< Client can read value */
  HAL_BLE_GATT_PROP_WRITE_NO_RSP = 0x02, /**< Write without response */
  HAL_BLE_GATT_PROP_WRITE = 0x04,        /**< Write with response (acknowledgment) */
  HAL_BLE_GATT_PROP_NOTIFY = 0x08,       /**< Server sends notifications (no ack) */
  HAL_BLE_GATT_PROP_INDICATE = 0x10,     /**< Server sends indications (requires ack) */
  HAL_BLE_GATT_PROP_BROADCAST = 0x20,    /**< Can be broadcast (advertising data) */
} hal_ble_gatt_prop_t;

/**
 * @brief Characteristic Read/Write Callback Context
 *
 * Opaque handle to connection context for callbacks.
 */
typedef uint16_t hal_ble_conn_handle_t;

/**
 * @brief Callback fired when Central writes to a characteristic
 *
 * @param[in] conn_handle Connection handle (identifies which Central)
 * @param[in] char_handle Characteristic handle (which characteristic was written)
 * @param[in] data Pointer to received data
 * @param[in] length Number of bytes received
 * @param[in] offset Offset into characteristic value (for long writes)
 */
typedef void (*hal_ble_write_callback_t)(
        hal_ble_conn_handle_t conn_handle, uint16_t char_handle, const uint8_t* data, size_t length, uint16_t offset);

/**
 * @brief Callback fired when Central reads a characteristic
 *
 * Allows dynamic value generation at read time.
 *
 * @param[in] conn_handle Connection handle
 * @param[in] char_handle Characteristic being read
 * @param[out] data Buffer to fill with characteristic value
 * @param[in] max_length Maximum bytes to write
 * @return Number of bytes written to @p data, or negative error code
 */
typedef int32_t (*hal_ble_read_callback_t)(hal_ble_conn_handle_t conn_handle,
                                           uint16_t char_handle,
                                           uint8_t* data,
                                           size_t max_length);

/**
 * @brief Characteristic Descriptor Handle Storage
 *
 * Internal (opaque) for platform use.
 */
typedef uint16_t hal_ble_char_handle_t;

/**
 * @brief GATT Characteristic Definition
 *
 * Describes a single BLE characteristic and its behavior.
 */
typedef struct
{
  hal_ble_char_handle_t handle;      /**< Output: Set by hal_ble_add_characteristic() */
  hal_ble_uuid_t uuid;               /**< Characteristic UUID */
  hal_ble_gatt_prop_t properties;    /**< Read/Write/Notify/Indicate flags */
  hal_ble_gatt_perm_t permissions;   /**< Access control (encrypted, etc.) */
  size_t max_length;                 /**< Max value size in bytes */
  uint8_t* initial_value;            /**< Optional initial value (NULL = empty) */
  size_t initial_length;             /**< Length of initial value */
  hal_ble_read_callback_t read_cb;   /**< Optional callback on read */
  hal_ble_write_callback_t write_cb; /**< Optional callback on write */
} hal_ble_characteristic_t;

/**
 * @brief GATT Service Definition
 *
 * Groups related characteristics under a single UUID.
 */
typedef struct
{
  hal_ble_uuid_t uuid;     /**< Service UUID (e.g., 0x180A for Device Info) */
  uint16_t service_handle; /**< Output: Set by hal_ble_add_service() */
} hal_ble_service_t;

/* ========== Security & Pairing ========== */

/**
 * @brief Pairing Completed Callback
 *
 * Fired when pairing finishes (success or failure).
 *
 * @param[in] conn_handle Connection that paired
 * @param[in] success true if pairing succeeded, false if rejected/failed
 */
typedef void (*hal_ble_pairing_complete_cb_t)(hal_ble_conn_handle_t conn_handle, bool success);

/**
 * @brief Connection secured Callback
 *
 * Fired when a secure connection is established.
 *
 * @param[in] conn_handle Connection that paired
 */
typedef void (*hal_ble_secure_cb_t)(hal_ble_conn_handle_t conn_handle);

/**
 * @brief Security Configuration
 *
 * Aggregates security settings for the peripheral.
 */
typedef struct
{
  hal_ble_pairing_complete_cb_t on_pairing_done; /**< Called when pairing finishes */
  hal_ble_secure_cb_t on_secured;                /**< Called when connection is secured */
  bool mitm_required;                            /**< Require MITM (Man-In-The-Middle) protection */
} hal_ble_sec_config_t;

/**
 * @brief Advertising stopped callback
 *
 * Fired when a the advertising stops for any reason.
 *
 * @param[in] conn_handle Connection that paired
 */
typedef void (*hal_ble_advertising_stops_cb_t)();

/* ========== GAP (Generic Access Profile) - Advertising ========== */

/**
 * @brief Advertising Parameters
 *
 * Controls how often and how long the device advertises.
 */
typedef struct
{
  uint16_t interval_min_ms; /**< Min advertising interval (milliseconds, typical 20-10240) */
  uint16_t interval_max_ms; /**< Max advertising interval (milliseconds) */
  bool include_tx_power;    /**< Include TX power in advertising packet */
  uint32_t timeout_s;       /**< Advertizing timeout */
  bool restartOnDisconnect; /**< Restart Advertising on disconnection */
} hal_ble_adv_params_t;

/**
 * @brief Connection Parameters
 *
 * Defines link layer behavior (latency, supervision timeout, etc.).
 */
typedef struct
{
  uint16_t min_conn_interval_ms;   /**< Minimum connection interval (7.5 to 4000 ms) */
  uint16_t max_conn_interval_ms;   /**< Maximum connection interval */
  uint16_t conn_latency;           /**< Slave latency (skip N connection events, 0-499) */
  uint16_t supervision_timeout_ms; /**< Timeout before link is dropped (100-32000 ms) */
} hal_ble_conn_params_t;

/**
 * @brief Connection State Change Callback
 *
 * Fired when Central connects or disconnects.
 *
 * @param[in] conn_handle Connection handle (invalid after disconnect)
 * @param[in] connected true on connect, false on disconnect
 * @param[in] reason Disconnect reason code (0 = normal, varies by platform)
 */
typedef void (*hal_ble_conn_state_cb_t)(hal_ble_conn_handle_t conn_handle, bool connected, uint8_t reason);

/**
 * @brief BLE Event Callback Collection
 *
 * Register all event callbacks in one structure.
 */
typedef struct
{
  hal_ble_conn_state_cb_t on_connect;                  /**< Called on connection established */
  hal_ble_conn_state_cb_t on_disconnect;               /**< Called on connection lost */
  hal_ble_advertising_stops_cb_t on_advertising_stops; /**< Called on advertising stopped */
} hal_ble_event_callbacks_t;

/* ========== Initialization & Lifecycle ========== */

/**
 * @brief Initialize BLE Stack
 *
 * Must be called before any other hal_ble_* function.
 *
 * @param[in] device_name Peripheral device name (max 31 characters for advertising)
 * @param[in] callbacks Event handler callbacks
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_init(const char* device_name, const hal_ble_event_callbacks_t* callbacks);

/**
 * @brief Deinitialize BLE Stack
 *
 * Stops advertising, closes connections, releases resources.
 *
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_deinit(void);

/**
 * @brief Check if BLE Stack is Initialized
 *
 * @return true if initialized, false otherwise
 */
bool hal_ble_is_initialized(void);

/* ========== GATT Database Setup ========== */

/**
 * @brief Add a Service to GATT Database
 *
 * Services group related characteristics. Typically called before adding characteristics.
 *
 * @param[in,out] service Service definition (handle will be filled)
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note Must call before hal_ble_start_advertising()
 * @note Typical service UUIDs:
 *  - 0x180A = Device Information Service
 *  - 0x180F = Battery Service
 *  - 0x181A = Environmental Sensing
 */
int32_t hal_ble_add_service(hal_ble_service_t* service);

/**
 * @brief Add a Characteristic to the Current Service
 *
 * Characteristics hold data values and define client interaction methods.
 *
 * @param[in,out] characteristic Characteristic definition (handle will be filled)
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note Must call between hal_ble_add_service() and hal_ble_start_advertising()
 * @note Read/Write callbacks are optional; if NULL, static value is used
 */
int32_t hal_ble_add_characteristic(hal_ble_characteristic_t* characteristic);

/**
 * @brief Update Characteristic Value
 *
 * Changes the current value stored in GATT database.
 * Use for non-callback characteristics or when value changes internally.
 *
 * @param[in] char_handle Characteristic handle (from hal_ble_add_characteristic)
 * @param[in] data New value
 * @param[in] length Number of bytes
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note Does not trigger notifications automatically; call hal_ble_notify() instead
 */
int32_t hal_ble_set_characteristic_value(hal_ble_char_handle_t char_handle, const uint8_t* data, size_t length);

/**
 * @brief Read Current Characteristic Value
 *
 * Retrieves value from GATT database (useful for non-callback characteristics).
 *
 * @param[in] char_handle Characteristic handle
 * @param[out] data Buffer to receive value
 * @param[in] max_length Buffer size
 * @return Number of bytes read, or negative error code
 */
int32_t hal_ble_get_characteristic_value(hal_ble_char_handle_t char_handle, uint8_t* data, size_t max_length);

/* ========== Notifications & Indications ========== */

/**
 * @brief Send Notification to All Connected Centrals
 *
 * Notification: Server sends value change without waiting for acknowledgment.
 * Faster but unreliable (client may miss it).
 *
 * @param[in] char_handle Characteristic handle
 * @param[in] data Updated value
 * @param[in] length Number of bytes
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note Only works for characteristics with HAL_BLE_GATT_PROP_NOTIFY property
 * @note Client must have enabled notifications via CCCD (Client Characteristic Config Descriptor)
 */
int32_t hal_ble_notify(hal_ble_char_handle_t char_handle, const uint8_t* data, size_t length);

/**
 * @brief Send Indication to a Specific Connected Central
 *
 * Indication: Server sends value; Central must send acknowledgment.
 * Slower but reliable (guaranteed delivery).
 *
 * @param[in] conn_handle Connection handle (target Central)
 * @param[in] char_handle Characteristic handle
 * @param[in] data Updated value
 * @param[in] length Number of bytes
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note Only works for characteristics with HAL_BLE_GATT_PROP_INDICATE property
 * @note Client must have enabled indications via CCCD
 */
int32_t hal_ble_indicate(hal_ble_conn_handle_t conn_handle,
                         hal_ble_char_handle_t char_handle,
                         const uint8_t* data,
                         size_t length);

/* ========== Advertising Control ========== */

/**
 * @brief Start Advertising (Peripheral Becomes Discoverable)
 *
 * Begins broadcasting BLE advertisements periodically.
 * Centrals can discover and connect to this device.
 *
 * @param[in] adv_params Advertising interval and mode
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note Will continue until hal_ble_stop_advertising() is called
 * @note Device name and GATT database must be set before calling
 */
int32_t hal_ble_start_advertising(const hal_ble_adv_params_t* adv_params);

/**
 * @brief Stop Advertising
 *
 * Stops broadcasting; existing connections remain active.
 *
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_stop_advertising(void);

/**
 * @brief Check if Currently Advertising
 *
 * @return true if advertising, false otherwise
 */
bool hal_ble_is_advertising(void);

/* ========== Connection Management ========== */

/**
 * @brief Disconnect a Connected Central
 *
 * Forcefully closes the link to a specific Central.
 *
 * @param[in] conn_handle Connection handle
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_disconnect(hal_ble_conn_handle_t conn_handle);

/**
 * \brief Return the current connection handle
 */
hal_ble_conn_handle_t hal_ble_get_connection_handle();

/**
 * @brief Get Current MTU (Maximum Transmission Unit) Size
 *
 * Default is 23 bytes; may increase after negotiation with Central.
 *
 * @param[in] conn_handle Connection handle
 * @return MTU size in bytes, or negative error code
 */
int32_t hal_ble_get_mtu(hal_ble_conn_handle_t conn_handle);

/**
 * @brief Request MTU Exchange with Central
 *
 * Initiates negotiation for larger packets (faster transfers).
 *
 * @param[in] conn_handle Connection handle
 * @param[in] desired_mtu Requested MTU (typically 200+)
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_request_mtu_exchange(hal_ble_conn_handle_t conn_handle, uint16_t desired_mtu);

/* ========== Security & Pairing ========== */

/**
 * @brief Configure Security Parameters
 *
 * Set encryption level, bonding mode, MITM protection, etc.
 * Must be called before starting advertising.
 *
 * @param[in] sec_config Security configuration
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_set_security_config(const hal_ble_sec_config_t* sec_config);

/**
 * @brief Initiate Pairing from Peripheral Side
 *
 * Requests the connected Central to start pairing (security exchange).
 *
 * @param[in] conn_handle Connection handle
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note More common for Central to initiate; this is rarely used
 * @note Result via on_pairing_request callback
 */
int32_t hal_ble_start_pairing(hal_ble_conn_handle_t conn_handle);

/**
 * @brief Clear All Stored Bonds (Forget Paired Centrals)
 *
 * Erases bonding information; next connection requires re-pairing.
 * Useful for factory reset scenarios.
 *
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_clear_bonds(void);

/* ========== Device Information ========== */

/**
 * @brief Set Device Name
 *
 * Public name shown in advertisements and GATT Device Information Service.
 *
 * @param[in] name Device name (max 31 characters recommended)
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_set_device_name(const char* name);

/**
 * @brief Get Device Name
 *
 * @param[out] name Buffer to receive device name
 * @param[in] max_length Buffer size
 * @return Number of bytes written, or negative error code
 */
int32_t hal_ble_get_device_name(char* name, size_t max_length);

/**
 * @brief Get BLE MAC Address (Bluetooth Device Address)
 *
 * @param[out] addr 6-byte buffer for Bluetooth address (big-endian)
 * @return HAL_BLE_SUCCESS or error code
 */
int32_t hal_ble_get_address(uint8_t addr[6]);

/**
 * @brief Set TX Power (Transmit Power)
 *
 * Controls radio transmission strength (affects range and power consumption).
 *
 * @param[in] tx_power_dbm Transmit power in dBm (typical: -40 to +4 dBm)
 * @return HAL_BLE_SUCCESS or error code
 *
 * @note Higher power = greater range but higher battery drain
 */
int32_t hal_ble_set_tx_power(int8_t tx_power_dbm);

/**
 * @brief Get TX Power
 *
 * @return Current TX power in dBm
 */
int8_t hal_ble_get_tx_power(void);

/* ========== Debugging & Status ========== */

/**
 * @brief Get Number of Active Connections
 *
 * @return Number of currently connected Centrals (typically 0-8)
 */
uint8_t hal_ble_get_connection_count(void);

/**
 * @brief Get Connection State
 *
 * @param[in] conn_handle Connection handle
 * @return true if connected, false otherwise
 */
bool hal_ble_is_connected(hal_ble_conn_handle_t conn_handle);

/**
 * @brief Get Connection Signal Strength (RSSI)
 *
 * RSSI (Received Signal Strength Indicator) indicates link quality.
 *
 * @param[in] conn_handle Connection handle
 * @return RSSI in dBm (typical: -100 to -20 dBm), or error code
 */
int8_t hal_ble_get_rssi(hal_ble_conn_handle_t conn_handle);

/**
 * @brief Return the adress of the connected device
 */
gap_addr_t hal_ble_get_adress(hal_ble_conn_handle_t conn_handle);

/**
 * Fill a bound key if it exists
 */
bool hal_ble_can_load_bound_key(hal_ble_conn_handle_t conn_handle);

/* ========== Platform Capabilities ========== */

/**
 * @brief Query BLE Feature Support
 *
 * Some platforms may not support all features.
 */
typedef enum
{
  HAL_BLE_FEATURE_BONDING,
  HAL_BLE_FEATURE_LE_SECURE_CONNECTIONS,
  HAL_BLE_FEATURE_DLE,        /**< Data Length Extension (larger packets) */
  HAL_BLE_FEATURE_MULTI_ROLE, /**< Simultaneous Peripheral + Central */
} hal_ble_feature_t;

/**
 * @brief Check if Feature is Supported
 *
 * @param[in] feature Feature to query
 * @return true if supported, false otherwise
 */
bool hal_ble_has_feature(hal_ble_feature_t feature);

/* ========== Constants ========== */

#define HAL_BLE_INVALID_HANDLE      (0xFFFF)
#define HAL_BLE_MAX_DEVICE_NAME_LEN (31)
#define HAL_BLE_MAX_ADV_DATA_LEN    (31)
#define HAL_BLE_MAX_CHARACTERISTICS (32) /**< Per service */
#define HAL_BLE_MAX_SERVICES        (16)
#define HAL_BLE_MIN_MTU             (23)  /**< BLE minimum MTU */
#define HAL_BLE_MAX_MTU             (247) /**< Extended MTU (DLE) */

namespace services {

/// OTA battery level service
namespace battery {

/// Init the service, with an initial value (0-100)
void init(const uint8_t batteryLevel, const bool addToAdvertised);

/// update battery level (0-100)
void write_battery_level(const uint8_t batteryLevel);

/// notify the connected device of a battery level (0-100)
void notify_battery_level(const uint8_t batteryLevel);

} // namespace battery

/// OTA UART connection
namespace uart {

/// Start the uart service
void init(const bool addToAdvertised);

/// Return true uart notify is activated
bool is_notified_enabled();

/// Return true if a char is available to read
bool is_available();

/// Read a character (blocking)
char read();

/// Flush the rx queue
void flush();

/// Write a buffer to connected device
size_t write(const char* const buffer, size_t bufferSize);

} // namespace uart

namespace system_infos {

struct Infos
{
  const char* model;
  const char* firmwareRev;
  const char* hardwareRev;
  const char* softwareRev;
  const char* manufacturer;
};

void init(const Infos& sysInfos, const bool addToAdvertised);

} // namespace system_infos

} // namespace services

} // namespace ble
} // namespace hal
} // namespace lampda

#endif // HAL_BLE_H
