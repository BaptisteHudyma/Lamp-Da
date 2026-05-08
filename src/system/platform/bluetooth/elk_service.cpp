#include "elk_service.h"

#include "src/system/platform/print.h"

#include "src/system/utils/elk_decoder.h"

#include "src/user/functions.h"

#include <tuple>
#include <cassert>

namespace lampda {
namespace bluetooth {

/// ELK main service
const uint8_t BLEElkService::UUID128_SERVICE[16] = {
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xf0, 0xff, 0x00, 0x00};

/// Characteristic write service
const uint8_t BLEElkService::UUID128_CHR_WRITE[16] = {
        0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xf3, 0xff, 0x00, 0x00};

// Constructor
BLEElkService::BLEElkService() : BLEService(UUID128_SERVICE), _writeCharac(UUID128_CHR_WRITE) {}

err_t BLEElkService::begin(void)
{
  VERIFY_STATUS(BLEService::begin());

  _writeCharac.setProperties(CHR_PROPS_WRITE);
  _writeCharac.setUserDescriptor("ELK BLE led control");
  _writeCharac.setWriteCallback(elk_write_cb, true);
  VERIFY_STATUS(_writeCharac.begin());

  return ERROR_NONE;
}

void BLEElkService::elk_commmand_handle(uint16_t conn_hdl, uint8_t* data, uint16_t len) const
{
  std::ignore = conn_hdl;

  utils::ELK::Package package;
  if (utils::ELK::decode_ELK_message(data, len, package))
  {
    // call user handle
    user::handle_bluetooth_ELK_command(package);
  }
  else
  {
    platform::lampda_print("Unsupported or invalid ELK message: ");
    for (uint16_t i = 0; i < len; i++)
    {
      platform::lampda_print_raw("%x ", data[i] & 0xFF);
    }
    platform::lampda_print_raw("\n");
  }
}

void BLEElkService::elk_write_cb(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len)
{
  BLEElkService const& svc = (BLEElkService&)chr->parentService();
  svc.elk_commmand_handle(conn_hdl, data, len);
}

} // namespace bluetooth
} // namespace lampda
