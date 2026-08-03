#include "bluetooth.h"

#include <bluefruit.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <cstdint>

#include "src/system/logic/alerts.h"
#include "src/system/utils/constants.h"

#include "src/system/component/battery.h"

#include "src/system/hal/print.h"
#include "src/system/hal/time.h"

#include "src/system/hal/bluetooth/elk_service.h"

namespace lampda {
namespace hal {
namespace bluetooth {

namespace __private {

#define ADV_TIMEOUT_FAST 30 // seconds. Set this higher to automatically stop advertising after a time
#define ADV_TIMEOUT      30 // seconds. Set this higher to automatically stop advertising after a time

#define BLE_APPEARANCE_LIGHT_SOURCE_GENERIC          0x07C0 /**< Light fixture BLE appearance flag (official flags) */
#define BLE_APPEARANCE_LIGHT_SOURCE_MULTICOLOR_ARRAY 0x07C6 /**< Light fixture BLE appearance flag (official flags) */

/// Indicates if the last advertising cancel command was automatic or requested
bool advertisingStoppedByRequest = false;

/// System Info Service
BLEDis bleSystemInfo;
/// System battery service
BLEBas bleBatteryService;
/// uart over ble
BLEUart bleuart;

/// led controler service
::lampda::bluetooth::BLEElkService bleElkService;

// UART TX Task structures
static constexpr size_t UART_TX_BUFFER_SIZE = 512;
static constexpr size_t UART_TX_QUEUE_SIZE = 8;
struct UartSendRequest
{
  char data[UART_TX_BUFFER_SIZE];
  size_t len;
};

static QueueHandle_t uart_send_queue = nullptr;
static TaskHandle_t uart_tx_task_handle = nullptr;

static void uart_tx_task(void* pvParameters)
{
  UartSendRequest req;
  while (true)
  {
    if (xQueueReceive(uart_send_queue, &req, portMAX_DELAY) == pdTRUE)
    {
      if (req.len == 0 || !Bluefruit.connected() || !bleuart.notifyEnabled())
        continue;

      size_t offset = 0;
      while (offset < req.len)
      {
        if (!Bluefruit.connected())
          break;

        size_t chunkSize = (req.len - offset > 20) ? 20 : (req.len - offset);
        size_t written = bleuart.write(req.data + offset, chunkSize);

        if (written == 0)
        {
          vTaskDelay(pdMS_TO_TICKS(10));
          continue;
        }

        offset += written;
        vTaskDelay(pdMS_TO_TICKS(5));
      }
    }
    else
    {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
}

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

void connect_callback(uint16_t conn_hdl)
{
  const auto batteryLevel = component::battery::get_battery_minimum_cell_level();
  write_battery_level(static_cast<uint8_t>(batteryLevel / 100));
  hal::lampda_print("Bluetooth connected");
}

void disconnect_callback(uint16_t conn_hdl, uint8_t reason)
{
  // Dont stop advertising here, some BLE drivers can send one command by connections.
  // Instead, restart the advertising
  start_advertising();
  hal::lampda_print("Bluetooth disconnected");
}

void adv_stop_callback(void)
{
  // auto turned off, start again !
  if (not advertisingStoppedByRequest)
  {
    start_advertising();
    hal::lampda_print("BLE Advertising timeout, advertising restarted.");
  }
  else
  {
    __private::stop_advertising();
    hal::lampda_print("BLE Advertising stop requested.");
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

  uart_send_queue = xQueueCreate(UART_TX_QUEUE_SIZE, sizeof(UartSendRequest));
  xTaskCreate(uart_tx_task, "UART_TX", 512, NULL, 3, &uart_tx_task_handle);

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
  hal::lampda_print("Blutooth started under the name:%s", ble_name);

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

bool send_uart(char const* buffer)
{
  // pass
  if (!Bluefruit.connected() || !__private::bleuart.notifyEnabled())
    return true;

  size_t len = strlen(buffer);
  if (!is_activated() || len == 0 || len >= __private::UART_TX_BUFFER_SIZE - 2)
    return false;

  __private::UartSendRequest req;
  memcpy(req.data, buffer, len);

  // Append CRLF if the buffer doesn't already end with a newline
  if (len == 0 || buffer[len - 1] != '\n')
  {
    req.data[len++] = '\r';
    req.data[len++] = '\n';
  }
  req.len = len;

  // Non-blocking send to queue. If queue is full, discard or handle error.
  if (xQueueSend(__private::uart_send_queue, &req, 0) != pdPASS) // 0 means don't block
    return false;
  return true;
}

Inputs read_uart()
{
  Inputs ret;
  if (not Bluefruit.connected())
    return ret;

  if (__private::bleuart.available())
  {
    uint8_t charRead = 0;

    // read available serial data
    do
    {
      // get the new byte:
      const char inChar = (char)__private::bleuart.read();
      // if the incoming character is a newline, finish parsing
      if (inChar == '\n')
      {
        // do not add empty strings and null terminated only strings
        if (charRead != 0)
        {
          // add null termination if needed
          if (charRead < Inputs::maxCommandSize)
          {
            if (ret.commandList[ret.commandCount][charRead] != '\0')
              ret.commandList[ret.commandCount][charRead] = '\0';
          }
          else
          {
            ret.commandList[ret.commandCount][Inputs::maxCommandSize - 1] = '\0';
          }
          ret.commandCount += 1;
        }
        else
        {
          for (size_t i = 0; i < Inputs::maxCommandSize; i++)
            ret.commandList[ret.commandCount][i] = '\0';
        }

        charRead = 0;
      }
      else if (charRead < Inputs::maxCommandSize)
      {
        // add it to the inputString:
        if (inChar >= 32)
        {
          ret.commandList[ret.commandCount][charRead] = inChar;
          charRead += 1;
        }
      }
    } while (__private::bleuart.available() && ret.commandCount < Inputs::maxCommands);
  }
  return ret;
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
} // namespace hal
} // namespace lampda
