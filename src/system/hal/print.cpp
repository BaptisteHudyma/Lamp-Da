#ifndef HAL_PRINT_CPP
#define HAL_PRINT_CPP

#include "print.h"

#include <Arduino.h>

#include "src/system/hal/bluetooth.h"
#include "src/system/hal/time.h"
#include "src/system/hal/queues.h"

#include "src/system/bsp/threads.h"

extern "C" {
// hack to use prints in c files
#include "src/system/utils/print.h"
}

namespace lampda {
namespace hal {

namespace __private {

// mutex to prevent lockups
StaticSemaphore_t _PrintMutex;
SemaphoreHandle_t printMutex = xSemaphoreCreateMutexStatic(&_PrintMutex);

void _lockPrintMutex(void) { xSemaphoreTake(printMutex, portMAX_DELAY); }
void _unlockPrintMutex(void) { xSemaphoreGive(printMutex); }

// UART TX Task structures
static constexpr size_t DESIRED_MEMORY = 2048;    // power of two
static constexpr size_t UART_TX_BUFFER_SIZE = 32; // number of small messages we could send
static constexpr size_t UART_TX_QUEUE_SIZE = DESIRED_MEMORY / UART_TX_BUFFER_SIZE;

struct UartSendRequest
{
  char data[UART_TX_BUFFER_SIZE];
  size_t len;
};

static QueueHandle_t uart_send_queue = nullptr;

void uart_tx_task()
{
  uint16_t max_mtu = 256;
  UartSendRequest req;
  // block until a queue msg in received
  const auto res = hal::queues::HAL_queue_receive(uart_send_queue, &req, UINT32_MAX);
  if (res == hal::queues::HAL_queue_status_t::HAL_QUEUE_OK)
  {
    if (req.len <= 0)
    {
      return;
    }

    size_t offset = 0;
    while (offset < req.len)
    {
      size_t chunkSize = (req.len - offset > max_mtu) ? max_mtu : (req.len - offset);
      size_t written = Serial.write(req.data + offset, chunkSize);

      if (written == 0)
      {
        hal::delay_ms(10);
        return;
      }

      offset += written;
      hal::delay_ms(1);
    }
  }
}

bool send_uart(char const* buffer)
{
  size_t len = strlen(buffer);
  if (len == 0)
    return true;

  // Max payload per chunk
  constexpr size_t MAX_CHUNK_LEN = UART_TX_BUFFER_SIZE;

  if (len <= MAX_CHUNK_LEN)
  {
    // Single chunk path
    UartSendRequest req;
    memcpy(req.data, buffer, len);
    req.len = len;

    const auto res = hal::queues::HAL_queue_send(__private::uart_send_queue, &req, 0);
    if (res != hal::queues::HAL_queue_status_t::HAL_QUEUE_OK)
      return false; // Queue full, message dropped
    return true;
  }

  // Large message: split into chunks
  size_t offset = 0;
  while (offset < len)
  {
    size_t chunk_len = (len - offset > MAX_CHUNK_LEN) ? MAX_CHUNK_LEN : (len - offset);
    __private::UartSendRequest req;
    memcpy(req.data, buffer + offset, chunk_len);
    req.len = chunk_len;

    const auto res = hal::queues::HAL_queue_send(__private::uart_send_queue, &req, 0);
    if (res != hal::queues::HAL_queue_status_t::HAL_QUEUE_OK)
      return false; // Queue full, message dropped
    // increment offset
    offset += chunk_len;
  }
  return true;
}

static std::array<char, max_tx_buffer_size> _buffer;
static void low_level_print(const char* format, va_list args)
{
  size_t len = strlen(format);
  if (len == 0)
    return;
  if (len > 0.9 * max_tx_buffer_size)
  {
    const char errMsg[] = "Cannot display: msg too big\r\n";
    __private::send_uart(errMsg);
    bluetooth::send_uart(errMsg);
    return;
  }

  _lockPrintMutex();

  _buffer.fill('\0');
  vsprintf(_buffer.data(), format, args);

  // send serial first
  if (not __private::send_uart(_buffer.data()))
  {
    const char errMsg[] = "Too much data for Serial transmition\r\n";
    bluetooth::send_uart(errMsg);
  }

  // then bluetooth
  if (not bluetooth::send_uart(_buffer.data()))
  {
    const char errMsg[] = "Too much data for BLE transmition\r\n";
    __private::send_uart(errMsg);
  }

  _unlockPrintMutex();
}

} // namespace __private

// only keep the chars inside a certain ascii range
bool is_ignore_char(char c) { return c < 32; }

void init_prints()
{
  __private::uart_send_queue =
          hal::queues::HAL_create_queue(sizeof(__private::UartSendRequest), __private::UART_TX_QUEUE_SIZE);
  bsp::threads::start_thread(__private::uart_tx_task, bsp::threads::print_taskName, 3, 512);

  Serial.begin(115200);
}

void lampda_print_raw(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  __private::low_level_print(format, args);
  va_end(args);
}

void lampda_print(const char* format, ...)
{
  va_list args;

  // header
  lampda_print_raw("%d> ", millis());
  // core
  va_start(args, format);
  __private::low_level_print(format, args);
  va_end(args);
  // end
  lampda_print_raw("\r\n");
}

constexpr uint8_t maxReadLinePerLoop = 5;
constexpr uint8_t maxLineLenght = 200;

Inputs read_inputs()
{
  __private::_lockPrintMutex();

  Inputs ret;

  if (Serial.available())
  {
    uint8_t charRead = 0;

    // read available serial data
    do
    {
      // get the new byte:
      const char inChar = (char)Serial.read();
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
        if (not is_ignore_char(inChar))
        {
          ret.commandList[ret.commandCount][charRead] = inChar;
          charRead += 1;
        }
      }
    } while (Serial.available() && ret.commandCount < Inputs::maxCommands);
  }

  __private::_unlockPrintMutex();
  return ret;
}

} // namespace hal
} // namespace lampda

#endif
