#include "elk_service.h"

#include <cstdint>

#include "src/system/common/elk_decoder.h"

#include "src/system/hal/ble.h"

#include "src/system/bsp/ble.h"
#include "src/system/bsp/text_out.h"

#include "src/system/logic/inputs_bluetooth.h"

namespace lampda::bsp::ble::services {

static void on_elk_cmd_received(hal::ble::hal_ble_conn_handle_t conn_handle,
                                uint16_t char_handle,
                                const uint8_t* data,
                                size_t length,
                                uint16_t offset)
{
  if (not lampda::bsp::ble::is_connection_allowed(conn_handle))
  {
    bsp::lampda_print("Unallowed connection, refusing elk message");
    return;
  }

  common::elk::Package elkPackage;
  if (common::elk::decode_ELK_message(data, length, elkPackage))
  {
    // call the logic handle
    logic::inputs_bluetooth::handle_BLE_ELK_command(elkPackage);
  }
  else
  {
    bsp::lampda_print("Unsupported or invalid ELK message: ");
    for (uint16_t i = 0; i < length; i++)
    {
      bsp::lampda_print_raw("%x ", data[i] & 0xFF);
    }
    bsp::lampda_print_raw("\n");
  }
}

void init_elk_service()
{
  hal::ble::hal_ble_service_t elkService = {.uuid = {.type = hal::ble::hal_ble_uuid_type_t::HAL_BLE_UUID_TYPE_16BIT,
                                                     .value = {.uuid128 = {0xfb,
                                                                           0x34,
                                                                           0x9b,
                                                                           0x5f,
                                                                           0x80,
                                                                           0x00,
                                                                           0x00,
                                                                           0x80,
                                                                           0x00,
                                                                           0x10,
                                                                           0x00,
                                                                           0x00,
                                                                           0xf0,
                                                                           0xff,
                                                                           0x00,
                                                                           0x00}}}};
  hal::ble::hal_ble_add_service(&elkService);

  hal::ble::hal_ble_characteristic_t elkWriteCharac = {
          .uuid = {.type = hal::ble::hal_ble_uuid_type_t::HAL_BLE_UUID_TYPE_16BIT,
                   .value = {.uuid128 = {0xfb,
                                         0x34,
                                         0x9b,
                                         0x5f,
                                         0x80,
                                         0x00,
                                         0x00,
                                         0x80,
                                         0x00,
                                         0x10,
                                         0x00,
                                         0x00,
                                         0xf3,
                                         0xff,
                                         0x00,
                                         0x00}}},
          .properties = hal::ble::hal_ble_gatt_prop_t::HAL_BLE_GATT_PROP_WRITE,
          .permissions = hal::ble::hal_ble_gatt_perm_t::HAL_BLE_GATT_PERM_WRITE_ENCRYPTED,
          .max_length = 9,
          .write_cb = on_elk_cmd_received};
  hal::ble::hal_ble_add_characteristic(&elkWriteCharac);
}

} // namespace lampda::bsp::ble::services
