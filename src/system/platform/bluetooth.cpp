#include "bluetooth.h"

#include <bluefruit.h>

#include "src/system/alerts.h"
#include "src/system/utils/print.h"

#include "src/system/platform/time.h"

namespace bluetooth {

#define ADV_TIMEOUT 30 // seconds. Set this higher to automatically stop advertising after a time
// The following code is for setting a name based on the actual device MAC
// address Where to go looking in memory for the MAC
typedef volatile uint32_t REG32;
#define pREG32           (REG32*)
#define MAC_ADDRESS_HIGH (*(pREG32(0x100000a8)))
#define MAC_ADDRESS_LOW  (*(pREG32(0x100000a4)))

//    READ_BUFSIZE            Size of the read buffer for incoming packets
#define READ_BUFSIZE (20)
/* Buffer to hold incoming characters */
uint8_t packetbuffer[READ_BUFSIZE + 1];

#define PACKET_ACC_LEN      (15)
#define PACKET_GYRO_LEN     (15)
#define PACKET_MAG_LEN      (15)
#define PACKET_QUAT_LEN     (19)
#define PACKET_BUTTON_LEN   (5)
#define PACKET_COLOR_LEN    (6)
#define PACKET_LOCATION_LEN (15)

// Uart over BLE service
BLEUart bleuart;

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

void startup_sequence()
{
  if (isInitialized)
    return;

  Bluefruit.autoConnLed(false);
  Bluefruit.setTxPower(4); // Check bluefruit.h for supported values
  Bluefruit.begin();

  // start uart reader
  bleuart.begin();

  char ble_name[16] = "Lampda-LSD-XXXX"; // Null-terminated string must be 1 longer than you
                                         // set it, for the null
  // Fill in the XXXX in ble_name
  byte_to_str(&ble_name[11], (MAC_ADDRESS_LOW >> 8) & 0xFF);
  byte_to_str(&ble_name[13], MAC_ADDRESS_LOW & 0xFF);
  // Set the name we just made
  Bluefruit.setName(ble_name);

  // Configure and start the BLE Uart service
  lampda_print("Blutooth started under the name:%s", ble_name);

  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include the BLE UART (AKA 'NUS') 128-bit UUID
  Bluefruit.Advertising.addService(bleuart);

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

uint8_t readPacket(BLEUart* ble_uart, uint16_t timeout)
{
  uint16_t origtimeout = timeout, replyidx = 0;

  memset(packetbuffer, 0, READ_BUFSIZE);

  while (timeout--)
  {
    if (replyidx >= 20)
      break;
    if ((packetbuffer[1] == 'A') && (replyidx == PACKET_ACC_LEN))
      break;
    if ((packetbuffer[1] == 'G') && (replyidx == PACKET_GYRO_LEN))
      break;
    if ((packetbuffer[1] == 'M') && (replyidx == PACKET_MAG_LEN))
      break;
    if ((packetbuffer[1] == 'Q') && (replyidx == PACKET_QUAT_LEN))
      break;
    if ((packetbuffer[1] == 'B') && (replyidx == PACKET_BUTTON_LEN))
      break;
    if ((packetbuffer[1] == 'C') && (replyidx == PACKET_COLOR_LEN))
      break;
    if ((packetbuffer[1] == 'L') && (replyidx == PACKET_LOCATION_LEN))
      break;

    while (ble_uart->available())
    {
      char c = ble_uart->read();
      if (c == '!')
      {
        replyidx = 0;
      }
      packetbuffer[replyidx] = c;
      replyidx++;
      timeout = origtimeout;
    }

    if (timeout == 0)
      break;
    delay_ms(1);
  }

  packetbuffer[replyidx] = 0; // null term

  if (!replyidx) // no data or timeout
    return 0;
  if (packetbuffer[0] != '!') // doesn't start with '!' packet beginning
    return 0;

  // check checksum!
  uint8_t xsum = 0;
  uint8_t checksum = packetbuffer[replyidx - 1];

  for (uint8_t i = 0; i < replyidx - 1; i++)
  {
    xsum += packetbuffer[i];
  }
  xsum = ~xsum;

  // Throw an error message if the checksum's don't match
  if (xsum != checksum)
  {
    lampda_print("Checksum mismatch in packet");
    return 0;
  }

  // checksum passed!
  return replyidx;
}

void parse_messages()
{
  if (!isInitialized)
    return;

  // Wait for new data to arrive (1ms timeout)
  uint8_t len = readPacket(&bleuart, 1);
  if (len == 0)
    return;

  // TODO: handle ble messages
}

} // namespace bluetooth
