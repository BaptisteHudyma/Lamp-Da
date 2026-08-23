
extern "C" {
// hack to use prints in c files
#include "src/system/bsp/text_out.h"
}

#include "src/system/hal/bluetooth.h"
#include "src/system/hal/queues.h"
#include "src/system/hal/serial.h"
#include "src/system/hal/time.h"

#include "src/system/bsp/threads.h"

#include <array>
#include <cstring>
#include <cstddef>
#include <cstdarg>

static constexpr size_t max_tx_buffer_size = 1024;

namespace lampda {
namespace bsp {

namespace __private {

// UART TX Task structures
static constexpr size_t DESIRED_MEMORY = 2048;    // power of two
static constexpr size_t UART_TX_BUFFER_SIZE = 32; // number of small messages we could send
static constexpr size_t UART_TX_QUEUE_SIZE = DESIRED_MEMORY / UART_TX_BUFFER_SIZE;

struct UartSendRequest
{
  char data[UART_TX_BUFFER_SIZE];
  size_t len;
};

static hal::queues::QueueHandle_t uart_send_queue = nullptr;

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
      size_t written = hal::serial::write(req.data + offset, chunkSize);

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
    hal::bluetooth::send_uart(errMsg);
    return;
  }

  _buffer.fill('\0');
  vsprintf(_buffer.data(), format, args);

  // send serial first
  if (not __private::send_uart(_buffer.data()))
  {
    const char errMsg[] = "Too much data for Serial transmition\r\n";
    hal::bluetooth::send_uart(errMsg);
  }

  // then bluetooth
  if (not hal::bluetooth::send_uart(_buffer.data()))
  {
    const char errMsg[] = "Too much data for BLE transmition\r\n";
    __private::send_uart(errMsg);
  }
}
} // namespace __private

void lampda_print_init()
{
  __private::uart_send_queue =
          hal::queues::HAL_create_queue(sizeof(__private::UartSendRequest), __private::UART_TX_QUEUE_SIZE);
  bsp::threads::start_thread(__private::uart_tx_task, bsp::threads::print_taskName, 3, 512);

  hal::serial::init();
}

/// C linkage to print functions
void lampda_print_raw(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  __private::low_level_print(format, args);
  va_end(args);
}

/// C linkage to print functions
void lampda_print(const char* format, ...)
{
  va_list args;

  // header
  lampda_print_raw("%d> ", hal::time_ms());
  // core
  va_start(args, format);
  __private::low_level_print(format, args);
  va_end(args);
  // end
  lampda_print_raw("\r\n");
}

} // namespace bsp
} // namespace lampda
