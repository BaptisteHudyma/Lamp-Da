#pragma once

#include <bluefruit.h>

namespace lampda {
namespace bluetooth {

/**
 * \brief Set a ELK Bluetooth service.
 * Used to control the lamp wia bluetooth
 */
class BLEElkService : public BLEService
{
public:
  static const uint8_t UUID128_SERVICE[16];
  static const uint8_t UUID128_CHR_WRITE[16];

  BLEElkService();

  virtual ~BLEElkService() = default;

  err_t begin(void) override;

protected:
  /// callback for a received command
  void elk_commmand_handle(uint16_t conn_hdl, uint8_t* data, uint16_t len) const;

private:
  BLECharacteristic _writeCharac;

  static void elk_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
};

} // namespace bluetooth
} // namespace lampda
