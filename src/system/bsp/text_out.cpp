
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

struct serial_backend_ops_t
{
  static constexpr size_t UART_TX_BUFFER_SIZE = 32; // number of small messages we can send

  // UART TX Task structures
  size_t desiredMemorySize_bytes; // power of two
  uint32_t taskName;              /// Name of the task to run process into
  uint16_t (*mtu_size)();
  bool (*is_activated)();
  size_t (*write)(const char* data, size_t len);
  hal::queues::QueueHandle_t uart_send_queue;

  struct UartSendRequest
  {
    char data[UART_TX_BUFFER_SIZE];
    size_t len;
  };

  /// Enqueue some text to display
  bool send_uart(char const* buffer) const
  {
    if (not this->is_activated())
      return true;
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

      const auto res = hal::queues::HAL_queue_send(this->uart_send_queue, &req, 0);
      if (res != hal::queues::HAL_queue_status_t::HAL_QUEUE_OK)
        return false; // Queue full, message dropped
      return true;
    }

    // Large message: split into chunks
    size_t offset = 0;
    while (offset < len)
    {
      size_t chunk_len = (len - offset > MAX_CHUNK_LEN) ? MAX_CHUNK_LEN : (len - offset);
      UartSendRequest req;
      memcpy(req.data, buffer + offset, chunk_len);
      req.len = chunk_len;

      const auto res = hal::queues::HAL_queue_send(this->uart_send_queue, &req, 0);
      if (res != hal::queues::HAL_queue_status_t::HAL_QUEUE_OK)
        return false; // Queue full, message dropped
      // increment offset
      offset += chunk_len;
    }
    return true;
  }

  void start(void (*taskfunc)(void))
  {
    uart_send_queue =
            hal::queues::HAL_create_queue(sizeof(UartSendRequest), desiredMemorySize_bytes / UART_TX_BUFFER_SIZE);
    bsp::threads::start_thread(taskfunc, taskName, 3, 512);
  }
};

/// Task to send out the text to output
static void uart_tx_task(const serial_backend_ops_t* op)
{
  const uint16_t max_mtu = op->mtu_size();
  serial_backend_ops_t::UartSendRequest req;
  // block until a queue msg in received
  const auto res = hal::queues::HAL_queue_receive(op->uart_send_queue, &req, UINT32_MAX);
  if (res == hal::queues::HAL_queue_status_t::HAL_QUEUE_OK)
  {
    if (not op->is_activated() or req.len <= 0)
    {
      return;
    }

    size_t offset = 0;
    while (offset < req.len)
    {
      if (not op->is_activated())
        break;

      size_t chunkSize = (req.len - offset > max_mtu) ? max_mtu : (req.len - offset);
      const char* const buff = req.data + offset;
      size_t written = op->write(buff, chunkSize);

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

static serial_backend_ops_t serial_uart_ops {.desiredMemorySize_bytes = 2048,
                                             .taskName = bsp::threads::print_taskName,
                                             .mtu_size = hal::serial::mtu_size,
                                             .is_activated = hal::serial::is_activated,
                                             .write = hal::serial::write,
                                             .uart_send_queue = nullptr};
static serial_backend_ops_t ble_uart_ops {.desiredMemorySize_bytes = 4096,
                                          .taskName = bsp::threads::ble_cli_taskName,
                                          .mtu_size = hal::bluetooth::serial::mtu_size,
                                          .is_activated = hal::bluetooth::serial::is_activated,
                                          .write = hal::bluetooth::serial::write,
                                          .uart_send_queue = nullptr};

static void serial_uart_task() { uart_tx_task(&serial_uart_ops); }
static void ble_uart_task() { uart_tx_task(&ble_uart_ops); }

void init_all()
{
  serial_uart_ops.start(serial_uart_task);
  ble_uart_ops.start(ble_uart_task);
}

void send_uart(char const* buffer)
{
  serial_uart_ops.send_uart(buffer);
  ble_uart_ops.send_uart(buffer);
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
    send_uart(errMsg);
    return;
  }

  _buffer.fill('\0');
  vsprintf(_buffer.data(), format, args);

  // send serial first
  if (not serial_uart_ops.send_uart(_buffer.data()))
  {
    const char errMsg[] = "Too much data for Serial transmition\r\n";
    ble_uart_ops.send_uart(errMsg);
  }

  // then bluetooth
  if (not ble_uart_ops.send_uart(_buffer.data()))
  {
    const char errMsg[] = "Too much data for BLE transmition\r\n";
    serial_uart_ops.send_uart(errMsg);
  }
}
} // namespace __private

void lampda_print_init()
{
  hal::serial::init();

  __private::init_all();
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
