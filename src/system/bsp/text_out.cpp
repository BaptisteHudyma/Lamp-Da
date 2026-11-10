
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

/// Line state of an output, only used by the send task of this output
struct tx_line_state_t
{
  bool atLineStart = true;       ///< The last byte written to the output ends a line
  bool needLineEnd = false;      ///< A line was cut: write a line end before the next text
  bool dropUntilLineEnd = false; ///< A line was cut: drop the rest of it
};

/// Max time to wait for a busy output to accept more data, before the current line is dropped
static constexpr uint32_t writeTimeoutMs = 100;
/// Wait between two write tries on a busy output
static constexpr uint32_t writeRetryDelayMs = 2;

/**
 * \brief Write all of a text to an output, waiting while the output is busy
 * \return The number of bytes written, lower than len if the output was turned off or stayed busy for writeTimeoutMs
 */
static size_t write_all(const serial_backend_ops_t* op, const char* data, const size_t len)
{
  size_t offset = 0;
  uint32_t waitedMs = 0;
  while (offset < len)
  {
    if (not op->is_activated())
      break;

    // the free space of the output can be 0 when it is busy
    const size_t maxChunk = op->mtu_size();
    const size_t chunkSize = (len - offset > maxChunk) ? maxChunk : (len - offset);
    const size_t written = (chunkSize > 0) ? op->write(data + offset, chunkSize) : 0;
    if (written == 0)
    {
      if (waitedMs >= writeTimeoutMs)
        break;
      hal::delay_ms(writeRetryDelayMs);
      waitedMs += writeRetryDelayMs;
      continue;
    }

    offset += written;
    waitedMs = 0;
    hal::delay_ms(1);
  }
  return offset;
}

/**
 * \brief Task to send out the text to output.
 * When the output fails in the middle of a line, the rest of this line is dropped and the line is closed with a line
 * end as soon as the output works again, so that the next line always starts at the beginning of a line.
 */
static void uart_tx_task(const serial_backend_ops_t* op, tx_line_state_t* line)
{
  serial_backend_ops_t::UartSendRequest req;
  // block until a queue msg in received
  const auto res = hal::queues::HAL_queue_receive(op->uart_send_queue, &req, UINT32_MAX);
  if (res != hal::queues::HAL_queue_status_t::HAL_QUEUE_OK or req.len == 0)
    return;

  const char* data = req.data;
  size_t len = req.len;

  // drop the end of a line that was cut, up to its line end
  if (line->dropUntilLineEnd)
  {
    const char* const lineEndPos = static_cast<const char*>(memchr(data, '\n', len));
    if (lineEndPos == nullptr)
      return;
    line->dropUntilLineEnd = false;
    len -= (lineEndPos + 1 - data);
    data = lineEndPos + 1;
    if (len == 0)
      return;
  }

  // close the cut line before writing anything else
  if (line->needLineEnd)
  {
    static constexpr char lineEnd[] = "\r\n";
    if (write_all(op, lineEnd, sizeof(lineEnd) - 1) != sizeof(lineEnd) - 1)
    {
      // still failing: drop this text too
      line->dropUntilLineEnd = (data[len - 1] != '\n');
      return;
    }
    line->needLineEnd = false;
    line->atLineStart = true;
  }

  const size_t written = write_all(op, data, len);
  if (written == len)
  {
    line->atLineStart = (data[len - 1] == '\n');
    return;
  }

  // output failed: the part already written cannot be taken back, so drop the rest of the line
  const bool cutsALine = (written > 0) ? (data[written - 1] != '\n') : not line->atLineStart;
  line->needLineEnd = cutsALine;
  line->dropUntilLineEnd = (data[len - 1] != '\n');
  line->atLineStart = not cutsALine;
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
static_assert(serialQueueLenght * serial_backend_ops_t::UART_TX_BUFFER_SIZE >= max_tx_buffer_size and
                      bleQueueLenght * serial_backend_ops_t::UART_TX_BUFFER_SIZE >= max_tx_buffer_size,
              "Each output queue must be able to hold the longest line");
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

static tx_line_state_t serialLineState;
static tx_line_state_t bleLineState;
static void serial_uart_task() { uart_tx_task(&serial_uart_ops, &serialLineState); }
static void ble_uart_task() { uart_tx_task(&ble_uart_ops, &bleLineState); }

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

int lampda_print_ble_pending()
{
  if (not __private::ble_uart_ops.is_activated())
    return 0;
  return static_cast<int>(hal::queues::HAL_queue_get_number_of_items(__private::ble_uart_ops.uart_send_queue));
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
