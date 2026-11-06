
extern "C" {
// hack to use prints in c files
#include "src/system/bsp/text_out.h"
}

#include "src/system/utils/utils.h"

#include "src/system/hal/mutex.h"
#include "src/system/hal/queues.h"
#include "src/system/hal/serial.h"
#include "src/system/hal/time.h"

#include "src/system/bsp/ble.h"
#include "src/system/bsp/threads.h"

#include <array>
#include <cstring>
#include <cstddef>
#include <cstdarg>

#ifdef LMBD_SIMULATION
#include <stdio.h>
#endif

static constexpr size_t max_tx_buffer_size = 1024;

namespace lampda {
namespace bsp {

namespace __private {

struct serial_backend_ops_t
{
  static constexpr size_t UART_TX_BUFFER_SIZE = 32; // number of small messages we can send
  static_assert(is_power_of_two<UART_TX_BUFFER_SIZE>().value, "SerialBackend buffer size must be a power of two");

  // UART TX Task structures
  size_t messageQueueLenght;
  uint32_t taskName; /// Name of the task to run process into
  uint16_t (*mtu_size)();
  bool (*is_activated)();
  size_t (*write)(const char* data, size_t len);
  hal::queues::QueueHandle_t uart_send_queue;
  uint8_t* queueBuffer;
  bsp::threads::TaskBuffer_t* sendTaskBuffer;
  static constexpr uint16_t sendQueueThreadBufferSize = 512;

  struct UartSendRequest
  {
    char data[UART_TX_BUFFER_SIZE];
    size_t len;
  };

  /**
   * \brief Enqueue some text to display, as one block.
   * The text is split into chunks of UART_TX_BUFFER_SIZE. It is queued only if all the chunks fit in the queue, so a
   * line is never sent in part. The caller must hold the print lock, so that no other text is queued in between.
   * \return false if the queue is too full and the text was dropped
   */
  bool send_uart(char const* buffer) const
  {
    if (not this->is_activated())
      return true;
    const size_t len = strlen(buffer);
    if (len == 0)
      return true;

    const size_t chunkCount = (len + UART_TX_BUFFER_SIZE - 1) / UART_TX_BUFFER_SIZE;
    const size_t usedSlots = hal::queues::HAL_queue_get_number_of_items(this->uart_send_queue);
    if (usedSlots >= messageQueueLenght or chunkCount > messageQueueLenght - usedSlots)
      return false; // Queue too full, message dropped

    size_t offset = 0;
    while (offset < len)
    {
      const size_t chunk_len = (len - offset > UART_TX_BUFFER_SIZE) ? UART_TX_BUFFER_SIZE : (len - offset);
      UartSendRequest req;
      memcpy(req.data, buffer + offset, chunk_len);
      req.len = chunk_len;

      const auto res = hal::queues::HAL_queue_send(this->uart_send_queue, &req, 0);
      if (res != hal::queues::HAL_queue_status_t::HAL_QUEUE_OK)
        return false; // Queue full, message dropped
      offset += chunk_len;
    }
    return true;
  }

  void start(void (*taskfunc)(void))
  {
    static uint8_t staticCount = 0;

    uart_send_queue =
            hal::queues::HAL_create_queue(sizeof(UartSendRequest), messageQueueLenght, staticCount, queueBuffer);

    bsp::threads::start_thread(taskfunc, taskName, 3, sendQueueThreadBufferSize, sendTaskBuffer);

    staticCount++;
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
    if (req.len <= 0 or not op->is_activated())
      return;

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
static constexpr size_t serialQueueLenght = 32;
static std::array<uint8_t, serialQueueLenght * sizeof(serial_backend_ops_t::UartSendRequest)> serialOpBuffer;
static std::array<bsp::threads::TaskBuffer_t, serial_backend_ops_t::sendQueueThreadBufferSize> serialTaskBuffer;
static serial_backend_ops_t serial_uart_ops {.messageQueueLenght = serialQueueLenght,
                                             .taskName = bsp::threads::print_taskName,
                                             .mtu_size = hal::serial::mtu_size,
                                             .is_activated = hal::serial::is_activated,
                                             .write = hal::serial::write,
                                             .uart_send_queue = nullptr,
                                             .queueBuffer = serialOpBuffer.data(),
                                             .sendTaskBuffer = serialTaskBuffer.data()};

static constexpr size_t bleQueueLenght = 128;
static std::array<uint8_t, bleQueueLenght * sizeof(serial_backend_ops_t::UartSendRequest)> bleOpBuffer;
static std::array<bsp::threads::TaskBuffer_t, serial_backend_ops_t::sendQueueThreadBufferSize> bleTaskBuffer;
static serial_backend_ops_t ble_uart_ops {.messageQueueLenght = bleQueueLenght,
                                          .taskName = bsp::threads::ble_cli_taskName,
                                          .mtu_size = bsp::ble::serial::mtu_size,
                                          .is_activated = bsp::ble::serial::is_activated,
                                          .write = bsp::ble::serial::write,
                                          .uart_send_queue = nullptr,
                                          .queueBuffer = bleOpBuffer.data(),
                                          .sendTaskBuffer = bleTaskBuffer.data()};

static void serial_uart_task() { uart_tx_task(&serial_uart_ops); }
static void ble_uart_task() { uart_tx_task(&ble_uart_ops); }

/// Serialize all the prints: formatting into _buffer and queueing a full line are done under this lock
static hal::mutex::hal_mutex_t _printLock;

void init_all()
{
  hal::mutex::hal_mutex_init(&_printLock);

  serial_uart_ops.start(serial_uart_task);
  ble_uart_ops.start(ble_uart_task);
}

/// Queue a full text on all outputs. The caller must hold _printLock
static void send_all(char const* buffer)
{
  // send serial first
  if (not serial_uart_ops.send_uart(buffer))
  {
    const char errMsg[] = "Too much data for Serial transmition\r\n";
    ble_uart_ops.send_uart(errMsg);
  }

  // then bluetooth
  if (not ble_uart_ops.send_uart(buffer))
  {
    const char errMsg[] = "Too much data for BLE transmition\r\n";
    serial_uart_ops.send_uart(errMsg);
  }
}

static constexpr char lineEnd[] = "\r\n";
static constexpr size_t lineEndLenght = sizeof(lineEnd) - 1;

static std::array<char, max_tx_buffer_size> _buffer;

/**
 * \brief Format a text and queue it on all outputs, as one block.
 * \param[in] isLogLine If true, add the "<time>> " header and the line end, to make a full log line
 */
static void low_level_print(const bool isLogLine, const char* format, va_list args)
{
  if (format == nullptr or format[0] == '\0')
    return;

  hal::mutex::hal_mutex_lock(&_printLock);

  // keep room for the line end
  const size_t maxTextLenght = _buffer.size() - (isLogLine ? lineEndLenght : 0);

  int len = 0;
  if (isLogLine)
    len = snprintf(_buffer.data(), maxTextLenght, "%lu> ", static_cast<unsigned long>(hal::time_ms()));

  if (len >= 0 and static_cast<size_t>(len) < maxTextLenght)
  {
    const int bodyLen = vsnprintf(_buffer.data() + len, maxTextLenght - len, format, args);
    len = (bodyLen < 0) ? -1 : len + bodyLen;
  }

  // snprintf returns the wanted lenght, so truncation shows as len >= buffer size
  if (len < 0 or static_cast<size_t>(len) >= maxTextLenght)
  {
    send_all("Cannot display: msg too big\r\n");
  }
  else
  {
    if (isLogLine)
      memcpy(_buffer.data() + len, lineEnd, sizeof(lineEnd));
    send_all(_buffer.data());
  }

  hal::mutex::hal_mutex_unlock(&_printLock);
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
  __private::low_level_print(false, format, args);
  va_end(args);
}

/// C linkage to print functions
void lampda_print(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  __private::low_level_print(true, format, args);
  va_end(args);
}

} // namespace bsp
} // namespace lampda
