#ifndef RT1715_H
#define RT1715_H

#include "Arduino.h"

namespace rt1715 {

constexpr uint16_t VENDOR_ID = 0xCF;
constexpr uint16_t VENDOR_CF_ID = 0x29;

constexpr uint16_t PRODUCT_ID = 0x11;
constexpr uint16_t PRODUCT_CF_ID = 0x17;

constexpr uint16_t DEVICE_ID = 0x73;
constexpr uint16_t DEVICE__CF_ID = 0x21;

constexpr uint16_t USBC_REV = 0x11;

constexpr uint16_t USB_PD_VER = 0x11;
constexpr uint16_t USB_PD_REV = 0x20;

constexpr uint16_t PD_INTERFACE_VER = 0x10;
constexpr uint16_t PD_INTERFACE_REV = 0x10;

// addresses
constexpr uint16_t VENDOR_ID_ADDR = 0x00;
constexpr uint16_t PRODUCT_ID_ADDR = 0x02;
constexpr uint16_t DEVICE_ID_ADDR = 0x04;
constexpr uint16_t USBC_REV_ADDR = 0x06;
constexpr uint16_t USB_PD_ADDR = 0x08;
constexpr uint16_t PD_INTERFACE_ADDR = 0x0A;

constexpr uint16_t ALERT_ADDR = 0x10;
constexpr uint16_t ALERT_MASK_ADDR = 0x12;
constexpr uint16_t POWER_STATUS_MASK_ADDR = 0x14;
constexpr uint16_t FAULT_STATUS_MASK_ADDR = 0x15;

constexpr uint16_t CONFIG_STANDARD_OUTPUT_ADDR = 0x18;

constexpr uint16_t TCPC_CONTROL_ADDR = 0x19;
constexpr uint16_t ROLE_CONTROL_ADDR = 0x1A;
constexpr uint16_t FAULT_CONTROL_ADDR = 0x1B;
constexpr uint16_t POWER_CONTROL_ADDR = 0x1C;

constexpr uint16_t CC_STATUS_ADDR = 0x1D;
constexpr uint16_t POWER_STATUS_ADDR = 0x1E;
constexpr uint16_t FAULT_STATUS_ADDR = 0x1F;

constexpr uint16_t COMMAND_ADDR = 0x23;

constexpr uint16_t DEVICE_CAPABILITIES_1L_ADDR = 0x24;
constexpr uint16_t DEVICE_CAPABILITIES_1H_ADDR = 0x25;
constexpr uint16_t DEVICE_CAPABILITIES_2L_ADDR = 0x26;

constexpr uint16_t STANDARD_INPUT_CAPABILITIES_ADDR = 0x28;
constexpr uint16_t STANDARD_OUTPUT_CAPABILITIES_ADDR = 0x29;

constexpr uint16_t MESSAGE_HEADER_INFO_ADDR = 0x29;

constexpr uint16_t RECEIVE_DETECT_ADDR = 0x2F;

// RX buffer

constexpr uint16_t RX_BYTE_COUNT_ADDR = 0x30;
constexpr uint16_t RX_BUF_FRAME_TYPE_ADDR = 0x31;
constexpr uint16_t RX_BUF_HEADER_BYTE_01_ADDR = 0x32;

constexpr uint16_t RX_BUF_OBJ1_BYTE_01_ADDR = 0x34;
constexpr uint16_t RX_BUF_OBJ1_BYTE_23_ADDR = 0x36;

constexpr uint16_t RX_BUF_OBJ2_BYTE_01_ADDR = 0x38;
constexpr uint16_t RX_BUF_OBJ2_BYTE_23_ADDR = 0x3A;

constexpr uint16_t RX_BUF_OBJ3_BYTE_01_ADDR = 0x3C;
constexpr uint16_t RX_BUF_OBJ3_BYTE_23_ADDR = 0x3E;

constexpr uint16_t RX_BUF_OBJ4_BYTE_01_ADDR = 0x40;
constexpr uint16_t RX_BUF_OBJ4_BYTE_23_ADDR = 0x42;

constexpr uint16_t RX_BUF_OBJ5_BYTE_01_ADDR = 0x44;
constexpr uint16_t RX_BUF_OBJ5_BYTE_23_ADDR = 0x46;

constexpr uint16_t RX_BUF_OBJ6_BYTE_01_ADDR = 0x48;
constexpr uint16_t RX_BUF_OBJ6_BYTE_23_ADDR = 0x4A;

constexpr uint16_t RX_BUF_OBJ7_BYTE_01_ADDR = 0x4C;
constexpr uint16_t RX_BUF_OBJ7_BYTE_23_ADDR = 0x4E;

// TX buffer

constexpr uint16_t TX_BUF_FRAME_TYPE_ADDR = 0x50;
constexpr uint16_t TX_BYTE_COUNT_ADDR = 0x51;
constexpr uint16_t TX_BUF_HEADER_BYTE_01_ADDR = 0x52;

constexpr uint16_t TX_BUF_OBJ1_BYTE_01_ADDR = 0x54;
constexpr uint16_t TX_BUF_OBJ1_BYTE_23_ADDR = 0x56;

constexpr uint16_t TX_BUF_OBJ2_BYTE_01_ADDR = 0x58;
constexpr uint16_t TX_BUF_OBJ2_BYTE_23_ADDR = 0x5A;

constexpr uint16_t TX_BUF_OBJ3_BYTE_01_ADDR = 0x5C;
constexpr uint16_t TX_BUF_OBJ3_BYTE_23_ADDR = 0x5E;

constexpr uint16_t TX_BUF_OBJ4_BYTE_01_ADDR = 0x60;
constexpr uint16_t TX_BUF_OBJ4_BYTE_23_ADDR = 0x62;

constexpr uint16_t TX_BUF_OBJ5_BYTE_01_ADDR = 0x64;
constexpr uint16_t TX_BUF_OBJ5_BYTE_23_ADDR = 0x66;

constexpr uint16_t TX_BUF_OBJ6_BYTE_01_ADDR = 0x68;
constexpr uint16_t TX_BUF_OBJ6_BYTE_23_ADDR = 0x6A;

constexpr uint16_t TX_BUF_OBJ7_BYTE_01_ADDR = 0x6C;
constexpr uint16_t TX_BUF_OBJ7_BYTE_23_ADDR = 0x6E;

// Other

constexpr uint16_t DIVERSE_1_ADDR = 0x90;
constexpr uint16_t BMCIO_VCONOCP_ADDR = 0x93;

constexpr uint16_t RT_ST_ADDR = 0x97;
constexpr uint16_t RT_INT_ADDR = 0x98;
constexpr uint16_t RT_MASK_ADDR = 0x99;

constexpr uint16_t DIVERSE_2_ADDR = 0x9B;
constexpr uint16_t WAKE_UP_EN_ADDR = 0x9F;
constexpr uint16_t SOFT_RESET_ADDR = 0xA0;
constexpr uint16_t TDRP_ADDR = 0xA2;
constexpr uint16_t DCSRCDRP_ADDR = 0xA3;

class RT1715 {
 public:
  RT1715();
  // Initialise the variable here, but it will be written from the main program
  static const byte RT1715addr = RT1715Devaddr;

  template <typename T>
  static bool readReg(T* dataParam, const uint8_t arrLen) {
    // This is a function for reading data words.
    // The number of bytes that make up a word is either 1 or 2.

    // Create an array to hold the returned data
    byte valBytes[arrLen];
    // Function to handle the I2C comms.
    if (readDataReg(dataParam->addr, valBytes, arrLen)) {
      // Cycle through array of data
      dataParam->val0 = valBytes[0];
      if (arrLen >= 2) dataParam->val1 = valBytes[1];
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  bool readRegEx(T& dataParam) {
    // This is a function for reading data words.
    // The number of bytes that make up a word is 2.
    constexpr uint8_t arrLen = 2;
    // Create an array to hold the returned data
    byte valBytes[arrLen];

    if (readDataReg(dataParam.addr, valBytes, arrLen)) {
      // Cycle through array of data
      dataParam.val0 = (byte)valBytes[0];
      dataParam.val1 = (byte)valBytes[1];
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  static bool writeReg(T dataParam) {
    // This is a function for writing data words.
    // The number of bytes that make up a word is 2.
    // It is called from functions within the structs.
    if (writeDataReg(dataParam->addr, dataParam->val0, dataParam->val1)) {
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  static bool writeRegEx(T dataParam) {
    // This is a function for writing data words.
    // It is called from the main program, without sending pointers
    // It can be used to write the registers once the bits have been twiddled
    if (writeDataReg(dataParam.addr, dataParam.val0, dataParam.val1)) {
      return true;
    } else {
      return false;
    }
  }
  template <typename T>
  static void setBytes(const T& dataParam, uint16_t value, uint16_t minVal,
                       uint16_t maxVal, uint16_t offset, uint16_t resVal) {
    // catch out of bounds
    if (value < minVal) value = minVal;
    if (value > maxVal) value = maxVal;
    // remove offset
    value = value - offset;
    // catch out of resolution
    value = value / resVal;
    value = value * resVal;
    // extract bytes and return to struct variables
    dataParam->val0 = (byte)(value);
    dataParam->val1 = (byte)(value >> 8);
  }
// macro to generate bit mask to access bits
#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
// macro to read bits from variable, using mask
#define READFROM(data, index, size) \
  (((data)&GETMASK((index), (size))) >> (index))
// macro to write bits into variable, using mask
#define WRITETO(data, index, size, value) \
  ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))
// macro to wrap functions for easy access
// if name is called with empty brackets, read bits and return value
// if name is prefixed with set_, write value in brackets into bits defined in
// FIELD
#define FIELD(data, name, index, size)                                 \
  inline decltype(data) name() { return READFROM(data, index, size); } \
  inline void set_##name(decltype(data) value) {                       \
    WRITETO(data, index, size, value);                                 \
  }

// macro to wrap functions for easy access, read only
// if name is called with empty brackets, read bits and return value
#define FIELD_RO(data, name, index, size) \
  inline decltype(data) name() { return READFROM(data, index, size); }

// macro to wrap functions for easy access, write only
// if name is called with empty brackets, read bits and return value
#define FIELD_WO(data, name, index, size)        \
  inline void set_##name(decltype(data) value) { \
    WRITETO(data, index, size, value);           \
  }

  struct Regt {
    struct VendorIDt {  // read only
      // Vendor ID
      byte val0, val1;
      uint8_t addr = VENDOR_ID_ADDR;
      byte get_vendorID() {
        readReg(this, 2);
        return val0;
      }
      byte get_vendorCfID() {
        readReg(this, 2);
        return val1;
      }
    } vendorID;
    struct ProductIDt {  // read only
      // Product ID
      byte val0, val1;
      uint8_t addr = PRODUCT_ID_ADDR;
      byte get_productID() {
        readReg(this, 2);
        return val0;
      }
      byte get_productCfID() {
        readReg(this, 2);
        return val1;
      }
    } productID;
    struct DeviceIDt {  // read only
      // device ID
      byte val0, val1;
      uint8_t addr = DEVICE_ID_ADDR;
      byte get_deviceID() {
        readReg(this, 2);
        return val0;
      }
      byte get_deviceCfID() {
        readReg(this, 2);
        return val1;
      }
    } deviceID;
    struct USB_C_REVt {  // read only
      byte val0, val1;
      uint8_t addr = USBC_REV_ADDR;
      byte get_USBC_revision() {
        readReg(this, 1);
        return val0;
      }
    } USB_C_REV;
    struct USB_PDt {  // read only
      byte val0, val1;
      uint8_t addr = USB_PD_ADDR;
      byte get_USB_PD_version() {
        readReg(this, 2);
        return val0;
      }
      byte get_USB_PD_revision() {
        readReg(this, 2);
        return val1;
      }
    } USB_PD;
    struct USB_PDInterfacet {  // read only
      byte val0, val1;
      uint8_t addr = PD_INTERFACE_ADDR;
      byte get_USB_PD_interface_version() {
        readReg(this, 2);
        return val0;
      }
      byte get_USB_PD_interface_revision() {
        readReg(this, 2);
        return val1;
      }
    } USB_PDInterface;

    struct Alertt {
      byte val0, val1;
      uint8_t addr = ALERT_ADDR;

      // 1: CC status changed
      FIELD(val0, CC_STATUS, 0x00, 0x01)

      // 1: Port status changed
      FIELD(val0, POWER_STATUS, 0x01, 0x01)

      // 1: Receive status changed
      FIELD(val0, RX_SOP_MSG_STATUS, 0x02, 0x01)

      // 1: received hard reset message
      FIELD(val0, RX_HARD_RESET, 0x03, 0x01)

      // 1: SOP* message transmission not successful, no GoodCRC response
      // received on SOP* message transmission.
      FIELD(val0, TX_FAIL, 0x04, 0x01)

      // 1: Reset or SOP* message transmission not sent due to incoming receive
      // message
      FIELD(val0, TX_DISCARD, 0x05, 0x01)

      // 1:Reset or SOP* message transmission successful
      FIELD(val0, TX_SUCCESS, 0x06, 0x01)

      // not supported
      FIELD_RO(val0, ALARM_VBUS_VOLTAGE_H, 0x07, 0x01)

      // Not supported
      FIELD_RO(val1, ALARM_VBUS_VOLTAGE_L, 0x00, 0x01)

      // 1: A Fault has occurred. Read the FAULT_STATUS register
      FIELD(val1, FAULT, 0x01, 0x01)

      // 1: TCPC Rx buffer has overflowed.
      FIELD(val1, RXBUF_OVFLOW, 0x02, 0x01)

      // Not supported
      FIELD_RO(val1, VBUS_SINK_DISCNT, 0x03, 0x01)
    } Alert;
    struct AlertMaskt {
      byte val0, val1;
      uint8_t addr = ALERT_MASK_ADDR;

      FIELD(val0, M_CC_STATUS, 0x00, 0x01)
      FIELD(val0, M_POWER_STATUS, 0x01, 0x01)
      FIELD(val0, M_RX_SOP_MSG_STATUS, 0x02, 0x01)
      FIELD(val0, M_RX_HARD_RESET, 0x03, 0x01)
      FIELD(val0, M_TX_FAIL, 0x04, 0x01)
      FIELD(val0, M_TX_DISCARD, 0x05, 0x01)
      FIELD(val0, M_TX_SUCCESS, 0x06, 0x01)
      FIELD_RO(val0, M_ALARM_VBUS_VOLTAGE_H, 0x07, 0x01)

      FIELD_RO(val1, M_ALARM_VBUS_VOLTAGE_L, 0x00, 0x01)
      FIELD(val1, M_FAULT, 0x01, 0x01)
      FIELD(val1, M_RXBUF_OVFLOW, 0x02, 0x01)
      FIELD_RO(val1, M_VBUS_SINK_DISCNT, 0x03, 0x01)
    } AlertMask;
    struct PowerStatusMaskt {
      byte val0, val1;
      uint8_t addr = POWER_STATUS_MASK_ADDR;

      FIELD_RO(val0, M_SINK_BUS, 0x00, 0x01)  // not supported
      FIELD(val0, M_VCONN_PRESENT, 0x01, 0x01)
      FIELD(val0, M_VBUS_PRESENT, 0x02, 0x01)
      FIELD(val0, M_VBUS_PRESENT_DETC, 0x03, 0x01)
      FIELD_RO(val0, M_SRC_VBUS, 0x04, 0x01)  // not supported
      FIELD(val0, M_SRC_HV, 0x05, 0x01)       // not supported
      FIELD(val0, M_TCPC_INITAL, 0x06, 0x01)
    } PowerStatusMask;
    struct FaultStatusMaskt {
      byte val0, val1;
      uint8_t addr = FAULT_STATUS_MASK_ADDR;

      FIELD(val0, M_I2C_ERROR, 0x00, 0x01)
      FIELD(val0, M_VCONN_OC, 0x01, 0x01)
      FIELD(val0, M_VBUS_OV, 0x02, 0x01)
      FIELD(val0, M_VBUS_OC, 0x03, 0x01)
      FIELD(val0, M_FORCE_DISC_FAIL, 0x04, 0x01)
      FIELD(val0, M_AUTO_DISC_FAIL, 0x05, 0x01)
      FIELD(val0, M_FORCE_OFF_VBUS, 0x06, 0x01)
      FIELD(val0, M_VCON_OV, 0x07, 0x01)
    } FaultStatusMask;

    struct ConfigStandardOutputt {
      byte val0, val1;
      uint8_t addr = CONFIG_STANDARD_OUTPUT_ADDR;

      FIELD_RO(val0, CONNECT_ORIENT, 0x00, 0x01)        // no support
      FIELD_RO(val0, CONNECT_PRESENT, 0x01, 0x01)       // no support
      FIELD_RO(val0, MUX_CTRL, 0x02, 0x02)              // no support
      FIELD_RO(val0, ACTIVE_CABLE_CONNECT, 0x04, 0x01)  // no support
      FIELD_RO(val0, AUDIO_ACC_CONNECT, 0x05, 0x01)     // no support
      FIELD_RO(val0, DBG_ACC_CONNECT_O, 0x06, 0x01)     // no support
      FIELD_RO(val0, H_IMPEDENCE, 0x07, 0x01)           // no support
    } ConfigStandardOutput;

    struct TCPCControlt {
      byte val0, val1;
      uint8_t addr = TCPC_CONTROL_ADDR;

      // 0b : When VCONN is enabled, apply it to the CC2 pin. Monitor the CC1
      // pin for BMC communications if PD messaging is enabled. (default)
      // 1b : When VCONN is enabled, apply it to the CC1 pin. Monitor the CC2
      // pin for BMC communications if PD messaging is enabled
      FIELD(val0, PLUG_ORIENT, 0x00, 0x01)

      // 0b : Normal Operation. Incoming messages enabled by RECEIVE_DETECT
      // passed to TCPM via Alert. (default)
      // 1b : BIST Test Mode. Incoming messages enabled by RECEIVE_DETECT result
      // in GoodCRC response but may not be passed to the TCPM via Alert. TCPC
      // may temporarily store incoming messages in the Receive Message Buffer,
      // but this may or may not result in a Receive SOP* Message Status or a Rx
      // Buffer Overflow alert
      FIELD(val0, BIST_TEST_MODE, 0x01, 0x01)

      FIELD_RO(val0, I2C_CLK_STRECTH, 0x02, 0x02)  // not supported
    } TCPCControl;
    struct RoleControlt {
      byte val0, val1;
      uint8_t addr = ROLE_CONTROL_ADDR;

      // 00b : Reserved
      // 01b : Rp (Use Rp definition in B5..4)
      // 10b : Rd (default)
      // 11b : Open (Disconnect or don’t care) Set to 11b if enabling DRP in
      // B7..6
      FIELD(val0, CC1, 0x00, 0x02)

      // 00b : Reserved
      // 01b : Rp (Use Rp definition in B5..4)
      // 10b : Rd (default)
      // 11b : Open (Disconnect or don’t care) Set to 11b if enabling DRP in
      // B7..6
      FIELD(val0, CC2, 0x02, 0x02)

      // 00b : Rp default (default)
      // 01b : Rp 1.5A
      // 10b : Rp 3.0
      // 11b : Reserved
      FIELD(val0, RP_VALUE, 0x04, 0x02)

      // 0b : No DRP. Bits B3..0 determine Rp/Rd/Ra settings (default)
      // 1b: DRP
      FIELD(val0, DRP, 0x06, 0x01)
    } RoleControl;
    struct FaultControlt {
      byte val0, val1;
      uint8_t addr = FAULT_CONTROL_ADDR;

      // 0b : Fault detection circuit enabled (default)
      // 1b : Fault detection circuit disabled
      FIELD(val0, DIS_VCON_OC, 0x00, 0x01)

      FIELD_RO(val0, DIS_VBUS_OV, 0x01, 0x01)                // not supported
      FIELD_RO(val0, DIS_VBUS_OC, 0x02, 0x01)                // not supported
      FIELD_RO(val0, DIS_VBUS_DISC_FAULT_TIMER, 0x03, 0x01)  // not supported
      FIELD_RO(val0, DIS_FORCE_OFF_VBUS, 0x04, 0x01)         // not supported

      // 0b : Fault detection circuit enabled (default)
      // 1b : Fault detection circuit disabled
      FIELD_RO(val0, DIS_VCON_OV, 0x07, 0x01)

    } FaultControl;
    struct PowerControlt {
      byte val0, val1;
      uint8_t addr = POWER_CONTROL_ADDR;

      // 0b : Disable VCONN Source (default)
      // 1b : Enable VCONN Source to CC Required
      FIELD(val0, EN_VCONN, 0x00, 0x01)

      // 0b : TCPC delivers at least 1W on VCONN (default)
      // 1b : TCPC delivers at least the power indicated in
      // DEVICE_CAPABILITIES.VCONNPowerSupported
      FIELD(val0, VCONN_POWER_SPT, 0x01, 0x01)

      FIELD_RO(val0, FORCE_DISC, 0x02, 0x01)         // not supported
      FIELD_RO(val0, BLEED_DISC, 0x03, 0x01)         // not supported
      FIELD_RO(val0, AUTO_DISC_DISCNCT, 0x04, 0x01)  // not supported
      FIELD_RO(val0, DIS_VOL_ALARM, 0x05, 0x01)      // not supported
      FIELD_RO(val0, VBUS_VOL_MONITOR, 0x06, 0x01)   // not supported
    } PowerControl;

    struct CCStatust {
      byte val0, val1;
      uint8_t addr = CC_STATUS_ADDR;

      /*
        If (ROLE_CONTROL.CC1 = Rp) or (DrpResult = 0)
        - 00b : SRC.Open(Open, Rp)(default)
        - 01b : SRC.Ra(below maximum vRa)
        - 10b : SRC.Rd(within the vRd range)
        - 11b : reserved

        If (ROLE_CONTROL.CC1 = Rd) or DrpResult = 1)
        - 00b : SNK.Open (Below maximum vRa) (default)
        - 01b: SNK.Default (Above minimum vRd-Connect)
        - 10b : SNK.Power1.5 (Above minimum vRd-Connect) Detects Rp-1.5A
        - 11b : SNK.Power3.0 (Above minimum vRd-Connect) Detects Rp-3.0A

        If ROLE_CONTROL.CC1 = Ra, this field is set to 00b
        If ROLE_CONTROL.CC1 = Open, this field is set to 00b

        This field always returns 00b if (DrpStatus = 1) or
        (POWER_CONTROL.EnableVCONN = 1 and POWER_CONTROL.PlugOrientation = 0).
        Otherwise, the returned value depends upon ROLE_CONTROL.CC1.
      */
      FIELD_RO(val0, CC1_STATUS, 0x00, 0x02)

      /*
      If (ROLE_CONTROL.CC2 = Rp) or (DrpResult = 0)
      - 00b : SRC.Open(Open, Rp)(default)
      - 01b : SRC.Ra(below maximum vRa)
      - 10b : SRC.Rd(within the vRd range)
      - 11b : reserved

      If (ROLE_CONTROL.CC2 = Rd) or DrpResult = 1)
      - 00b : SNK.Open (Below maximum vRa) (default)
      - 01b: SNK.Default (Above minimum vRd-Connect)
      - 10b : SNK.Power1.5 (Above minimum vRd-Connect) Detects Rp-1.5A
      - 11b : SNK.Power3.0 (Above minimum vRd-Connect) Detects Rp-3.0A

      If ROLE_CONTROL.CC2 = Ra, this field is set to 00b
      If ROLE_CONTROL.CC2 = Open, this field is set to 00b

      This field always returns 00b if (DrpStatus = 1) or
      (POWER_CONTROL.EnableVCONN = 1 and POWER_CONTROL.PlugOrientation = 0).
      Otherwise, the returned value depends upon ROLE_CONTROL.CC2.
    */
      FIELD_RO(val0, CC2_STATUS, 0x02, 0x02)

      // 0b : the TCPC is presenting Rp (default)
      // 1b : the TCPC is presenting Rd
      FIELD_RO(val0, DRP_RESULT, 0x04, 0x01)

      // 0b : the TCPC has stopped toggling or (ROLE_CONTROL.DRP = 00) (default)
      // 1b : the TCPC is toggling
      FIELD_RO(val0, DRP_STATUS, 0x05, 0x01)

    } CCStatus;
    struct PowerStatust {
      byte val0, val1;
      uint8_t addr = POWER_STATUS_ADDR;

      FIELD_RO(val0, SINK_VBUS, 0x00, 0x01)  // no support

      // 0b : VCONN is not present (default)
      // 1b : This bit is asserted when VCONN present CC1 or CC2. Threshold is
      // fixed at 2.4V
      FIELD_RO(val0, VCONN_PRESENT, 0x01, 0x01)

      // 0b : VBUS Disconnected (default)
      // 1b : VBUS Connected
      FIELD_RO(val0, VBUS_PRESENT, 0x02, 0x01)

      // 0b : VBUS present detection disabled
      // 1b : VBUS present detection enabled (default)
      FIELD_RO(val0, VBUS_PRESENT_DETC, 0x03, 0x01)

      FIELD_RO(val0, SRC_VBUS, 0x04, 0x01)  // not supported
      FIELD_RO(val0, SRC_HV, 0x05, 0x01)    // not supported

      // 0b : The TCPC has completed initialization and all registers are
      // valid(default)
      // 1b : The TCPC is still performing internal initialization and the only
      // registers that are guaranteed to return the correct values are 00h..0Fh
      FIELD_RO(val0, TCPC_INITAL, 0x06, 0x01)

      FIELD_RO(val0, DBG_ACC_CONNECT, 0x07, 0x01)  // no support
    } PowerStatus;
    struct FaultStatust {
      byte val0, val1;
      uint8_t addr = FAULT_STATUS_ADDR;

      // 0b : No Error (default)
      // 1b : I2C error has occurred.
      FIELD(val0, I2C_ERROR, 0x00, 0x01)

      // 0b : No fault detected (default)
      // 1b : Over-current VCONN fault latched
      FIELD(val0, VCON_OC, 0x01, 0x01)

      FIELD_RO(val0, VBUS_OV, 0x02, 0x01)          // not supported
      FIELD_RO(val0, VBUS_OC, 0x03, 0x01)          // not supported
      FIELD_RO(val0, FORCE_DISC_FAIL, 0x04, 0x01)  // not supported
      FIELD_RO(val0, AUTO_DISC_FAIL, 0x05, 0x01)   // not supported
      FIELD_RO(val0, FORCE_OFF_VBUS, 0x06, 0x01)   // not supported

      // 0b : Not in an over-voltage protection state (default)
      // 1b : Over-voltage fault latched.
      FIELD(val0, VCON_OV, 0x07, 0x01)
    } FaultStatus;

    struct Commandt {
      byte val0, val1;
      uint8_t addr = COMMAND_ADDR;

      // 0010 0010b : DisableVbusDetect: Disable Vbus present and vSafe0V
      // detection.
      // 0011 0011b : EnableVbusDetect: Enable Vbus present and vSafe0V
      // detection.
      // 1001 1001b : Start DRP Toggling if ROLE_CONTROL.DRP = 1b.
      // If ROLE_CONTROL.CC1/CC2= 01b start with Rp, if ROLE_CONTROL.CC1/CC2=10b
      // start with Rd.
      FIELD(val0, COMMAND, 0x00, 0x08)
    } Command;

    struct DeviceCapabilities1lt {
      byte val0, val1;
      uint8_t addr = DEVICE_CAPABILITIES_1L_ADDR;

      // 0b : TCPC is not capable of controlling the source path to VBUS
      // (default)
      // 1b : TCPC is capable of controlling the source path to VBUS
      FIELD_RO(val0, SOURCE_VBUS, 0x00, 0x01)

      // 0b : TCPC is not capable of controlling the source high voltage path to
      // VBUS (default)
      // 1b : TCPC is capable of controlling the source high voltage path to
      // VBUS
      FIELD_RO(val0, SOURCE_HV_VBUS, 0x01, 0x01)

      // 0b : TCPC is not capable controlling the sink path to the system load
      // (default)
      // 1b : TCPC is capable of controlling the sink path to the system load
      FIELD_RO(val0, CPB_SINK_VBUS, 0x02, 0x01)

      // 0b : TCPC is not capable of switching VCONN
      // 1b : TCPC is capable of switching VCONN (default)
      FIELD_RO(val0, SOURCE_VCONN, 0x03, 0x01)

      // 0b : All SOP* except SOP’_DBG/SOP”_DBG
      // 1b : All SOP* messages are supported (default)
      FIELD_RO(val0, ALL_SOP_SUPPORT, 0x04, 0x01)

      // 000b : Type-C Port Manager can configure the Port as Source only or
      // Sink only (not DRP)
      // 001b : Source only
      // 010b : Sink only
      // 011b : Sink with accessory support (optional)
      // 100b : DRP only
      // 101b : Adapter or Cable (Ra) only
      // 110b : Source, Sink, DRP, Adapter/Cable all supported (default)
      // 111b : Not valid
      FIELD_RO(val0, ROLES_SUPPORT, 0x05, 0x03)

    } DeviceCapabilities1l;
    struct DeviceCapabilities1ht {
      byte val0, val1;
      uint8_t addr = DEVICE_CAPABILITIES_1H_ADDR;

      // 00b : Rp default only
      // 01b : Rp 1.5A and default
      // 10b : Rp 3.0A, 1.5A, and default (default)
      // 11b : Reserved Rp values which may be configured bythe TCPM via the
      // ROLE_CONTROL register
      FIELD_RO(val0, SOURCE_RP_SUPPORT, 0x00, 0x02)

      // 0b : No VBUS voltage measurement nor VBUS Alarms (default)
      // 1b : VBUS voltage measurement and VBUS Alarm
      FIELD_RO(val0, VBUS_MEASURE_ALARM, 0x02, 0x01)

      // 0b : No Force Discharge implemented in TCPC (default)
      // 1b : Force Discharge is implemented in the TCPC
      FIELD_RO(val0, CPB_FORCE_DISC, 0x03, 0x01)

      // 0b : No Bleed Discharge implemented in TCPC (default)
      // 1b : Bleed Discharge is implemented in the TCPC
      FIELD_RO(val0, CPB_BLEED_DISC, 0x04, 0x01)

      // 0b : VBUS OVP is not reported by the TCPC (default)
      // 1b : VBUS OVP is reported by the TCPC
      FIELD_RO(val0, CPB_VBUS_OV, 0x05, 0x01)

      // 0b : VBUS OCP is not reported by the TCPC (default)
      // 1b : VBUS OCP is reported by the TCPC
      FIELD_RO(val0, CPB_VBUS_OC, 0x06, 0x01)
    } DeviceCapabilities1h;
    struct DeviceCapabilities2lt {
      byte val0, val1;
      uint8_t addr = DEVICE_CAPABILITIES_2L_ADDR;

      // 0b : TCPC is not capable of detecting a VCONN fault
      // 1b : TCPC is capable of detecting a VCONN fault (default)
      FIELD_RO(val0, VCONN_OCF, 0x00, 0x01)

      // 000b : 1.0W
      // 001b : 1.5W
      // 010b : 2.0W (default)
      // 011b : 3W
      // 100b : 4W
      // 101b : 5W
      // 110b : 6W
      // 111b : External
      FIELD_RO(val0, VCONN_POWER, 0x01, 0x10)

      // 00b : TCPC has 25mV LSB for its voltage alarm and uses all 10 bits in
      // VBUS_VOLTAGE_ALARM_HI_CFG and VBUS_VOLTAGE_ALARM_LO_CFG.
      // 01b : TCPC has 50mV LSB for its voltage alarm and uses only 9 bits.
      // VBUS_VOLTAGE_ALARM_HI_CFG[0] and VBUS_VOLTAGE_ALARM_LO_CFG[0] are
      // ignored by TCPC.
      // 10b : TCPC has 100mV LSB for its voltage alarm and
      // uses only 8 bits. VBUS_VOLTAGE_ALARM_HI_CFG[1:0] and
      // VBUS_VOLTAGE_ALARM_LO_CFG[1:0] are ignored by TCPC.
      // 11b : Not support this function. (default)
      FIELD_RO(val0, VBUS_VOL_ALARM_LSB, 0x04, 0x11)

      // 0b : VBUS_STOP_DISCHARGE_THRESHOLD not implemented (default)
      // 1b : VBUS_STOP_DISCHARGE_THRESHOLD implemented
      FIELD_RO(val0, STOP_DISC_THD, 0x06, 0x01)

      // 0b : VBUS_SINK_DISCONNECT_THRESHOLD not implemented (default: Use
      // POWER_STATUS.VbusPresent = 0b to indicate a Sink disconnect) (default)
      // 1b : VBUS_SINK_DISCONNECT_THRESHOLD implemented
      FIELD_RO(val0, SINK_DISCONNECT_DET, 0x07, 0x01)
    } DeviceCapabilities2l;

    struct StandardInputCapabilitiest {
      byte val0, val1;
      uint8_t addr = STANDARD_INPUT_CAPABILITIES_ADDR;

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, FORCE_OFF_VBUS_IN, 0x00, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, VBUS_EXT_OCF, 0x01, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, VBUS_EXT_OVF, 0x02, 0x01)
    } StandardInputCapabilities;
    struct StandardOutputCapabilitiest {
      byte val0, val1;
      uint8_t addr = STANDARD_OUTPUT_CAPABILITIES_ADDR;

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, CPB_CONNECT_ORIENT, 0x00, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, CPB_CONNECT_PRESENT, 0x01, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, CPB_MUX_CFG_CTRL, 0x02, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, CPB_ACTIVE_CABLE_IND, 0x03, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, CPB_AUDIO_ADT_ACC_IND, 0x04, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, CPB_VBUS_PRESENT_MNT, 0x05, 0x01)

      // 0b : Not present in TCPC (default)
      // 1b : Present in TCPC
      FIELD_RO(val0, CPB_DBG_ACC_IND, 0x06, 0x01)
    } StandardOutputCapabilities;

    struct MessageHeaderInfot {
      byte val0, val1;
      uint8_t addr = MESSAGE_HEADER_INFO_ADDR;

      // 0b : Sink (default)
      // 1b : Source
      FIELD(val0, POWER_ROLE, 0x00, 0x01)

      // 00b : Revision 1.0
      // 01b : Revision 2.0 (default)
      // 10b : Revision 3.0
      // 11b : Reserved
      FIELD(val0, USBPD_SPECREV, 0x01, 0x02)

      // 0b : Sink (default)
      // 1b : Source
      FIELD(val0, DATA_ROLE, 0x03, 0x01)

      // 0b : Message originated from Source, Sink, or DRP (default)
      // 1b : Message originated from a CablePlug
      FIELD(val0, CABLE_PLUG, 0x04, 0x01)
    } MessageHeaderInf;

    // RX

    struct ReceiveDetectt {
      byte val0, val1;
      uint8_t addr = RECEIVE_DETECT_ADDR;

      // 0b : TCPC does not detect SOP message (default)
      // 1b : TCPC detects SOP message
      FIELD(val0, EN_SOP, 0x00, 0x01)

      // 0b : TCPC does not detect SOP' message (default)
      // 1b : TCPC detects SOP' message
      FIELD(val0, EN_SOP1, 0x01, 0x01)

      // 0b : TCPC does not detect SOP'' message (default)
      // 1b : TCPC detects SOP'' message
      FIELD(val0, EN_SOP2, 0x02, 0x01)

      // 0b : TCPC does not detect SOP_DBG' message (default)
      // 1b : TCPC detects SOP_DBG' message
      FIELD(val0, EN_SOP1DB, 0x03, 0x01)

      // 0b : TCPC does not detect SOP_DBG'' message (default)
      // 1b : TCPC detects SOP_DBG'' message
      FIELD(val0, EN_SOP2DB, 0x04, 0x01)

      // 0b: TCPC does not detect Hard Reset signaling (default)
      // 1b : TCPC detects Hard Reset signaling
      FIELD(val0, EN_HARD_RST, 0x05, 0x01)

      // 0b: TCPC does not detect Cable Reset signaling (default)
      // 1b : TCPC detects Cable Reset signaling
      FIELD(val0, EN_CABLE_RST, 0x06, 0x01)
    } ReceiveDetect;

    struct RxByteCountt {
      byte val0, val1;
      uint8_t addr = RX_BYTE_COUNT_ADDR;

      // Indicates number of bytes in this register that are not stale. The TCPM
      // should read the first RECEIVE_BYTE_COUNT bytes in this register.
      FIELD(val0, RX_BYTE_COUNT, 0x00, 0x08)
    } RxByteCount;

    struct RXBufFrameTypet {
      byte val0, val1;
      uint8_t addr = RX_BUF_FRAME_TYPE_ADDR;

      // Type of received frame
      // 000b : Received SOP (default)
      // 001b : Received SOP'
      // 010b : Received SOP''
      // 011b : Received SOP_DBG’
      // 100b : Received SOP_DBG’’
      // 110b : Received Cable Reset
      // All others are reserved.
      FIELD_RO(val0, RX_FRAME_TYPE, 0x00, 0x03)
    } RXBufFrameType;

    struct RxBufHeaderByte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_HEADER_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of message header
      FIELD_RO(val0, RX_HEAD_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of message header
      FIELD_RO(val1, RX_HEAD_1, 0x01, 0x08)
    } RxBufHeaderByte01;

    // rx objects

    struct RxBufObj1Byte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ1_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 1st data object
      FIELD_RO(val0, RX_OBJ1_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 1st data object
      FIELD_RO(val1, RX_OBJ1_1, 0x01, 0x08)
    } RxBufObj1Byte01;
    struct RxBufObj1Byte23t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ1_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 1st data object
      FIELD_RO(val0, RX_OBJ1_2, 0x00, 0x08)
      // Byte 3 (bits 31..24) of 1st data object
      FIELD_RO(val1, RX_OBJ1_3, 0x01, 0x08)
    } RxBufObj1Byte23;

    struct RxBufObj2Byte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ2_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 2st data object
      FIELD_RO(val0, RX_OBJ2_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 2st data object
      FIELD_RO(val1, RX_OBJ2_1, 0x01, 0x08)
    } RxBufObj2Byte01;
    struct RxBufObj2Byte23t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ2_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 2st data object
      FIELD_RO(val0, RX_OBJ2_2, 0x00, 0x08)
      // Byte 3 (bits 31..24) of 2st data object
      FIELD_RO(val1, RX_OBJ2_3, 0x01, 0x08)
    } RxBufObj2Byte23;

    struct RxBufObj3Byte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ3_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 3st data object
      FIELD_RO(val0, RX_OBJ3_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 3st data object
      FIELD_RO(val1, RX_OBJ3_1, 0x01, 0x08)
    } RxBufObj3Byte01;
    struct RxBufObj3Byte23t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ3_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 3st data object
      FIELD_RO(val0, RX_OBJ3_2, 0x00, 0x08)
      // Byte 3 (bits 31..24) of 3st data object
      FIELD_RO(val1, RX_OBJ3_3, 0x01, 0x08)
    } RxBufObj3Byte23;

    struct RxBufObj4Byte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ4_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 4st data object
      FIELD_RO(val0, RX_OBJ4_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 4st data object
      FIELD_RO(val1, RX_OBJ4_1, 0x01, 0x08)
    } RxBufObj4Byte01;
    struct RxBufObj4Byte23t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ4_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 4st data object
      FIELD_RO(val0, RX_OBJ4_2, 0x00, 0x08)
      // Byte 3 (bits 31..24) of 4st data object
      FIELD_RO(val1, RX_OBJ4_3, 0x01, 0x08)
    } RxBufObj4Byte23;

    struct RxBufObj5Byte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ5_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 5st data object
      FIELD_RO(val0, RX_OBJ5_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 5st data object
      FIELD_RO(val1, RX_OBJ5_1, 0x01, 0x08)
    } RxBufObj5Byte01;
    struct RxBufObj5Byte23t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ5_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 5st data object
      FIELD_RO(val0, RX_OBJ5_2, 0x00, 0x08)
      // Byte 3 (bits 31..24) of 5st data object
      FIELD_RO(val1, RX_OBJ5_3, 0x01, 0x08)
    } RxBufObj5Byte23;

    struct RxBufObj6Byte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ6_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 6st data object
      FIELD_RO(val0, RX_OBJ6_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 6st data object
      FIELD_RO(val1, RX_OBJ6_1, 0x01, 0x08)
    } RxBufObj6Byte01;
    struct RxBufObj6Byte23t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ6_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 6st data object
      FIELD_RO(val0, RX_OBJ6_2, 0x00, 0x08)
      // Byte 3 (bits 31..24) of 6st data object
      FIELD_RO(val1, RX_OBJ6_3, 0x01, 0x08)
    } RxBufObj6Byte23;

    struct RxBufObj7Byte01t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ7_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 7st data object
      FIELD_RO(val0, RX_OBJ7_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 7st data object
      FIELD_RO(val1, RX_OBJ7_1, 0x01, 0x08)
    } RxBufObj7Byte01;
    struct RxBufObj7Byte23t {
      byte val0, val1;
      uint8_t addr = RX_BUF_OBJ7_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 7st data object
      FIELD_RO(val0, RX_OBJ7_2, 0x00, 0x08)
      // Byte 3 (bits 31..24) of 7st data object
      FIELD_RO(val1, RX_OBJ7_3, 0x01, 0x08)
    } RxBufObj7Byte23;

    // TX

    struct TxBufFrameTypet {
      byte val0, val1;
      uint8_t addr = TX_BUF_FRAME_TYPE_ADDR;

      // 000b : Transmit SOP (default)
      // 001b : Transmit SOP'
      // 010b : Transmit SOP''
      // 011b : Transmit SOP_DBG’
      // 100b : Transmit SOP_DBG’’
      // 101b : Transmit Hard Reset
      // 110b : Transmit Cable Reset
      // 111b : Transmit BIST Carrier Mode 2
      //(TCPC shall exit the BIST mode no later than tBISTContMode max)
      FIELD(val0, TX_BUF_FRAME_TYPE, 0x00, 0x03)

      // 00b : No message retry is required (default)
      // 01b : Automatically retry message transmission once
      // 10b : Automatically retry message transmission twice
      // 11b : Automatically retry message transmission three times
      FIELD(val0, TX_RETRY_CNT, 0x04, 0x02)
    } TxBufFrameType;

    struct TxByteCountt {
      byte val0, val1;
      uint8_t addr = TX_BYTE_COUNT_ADDR;

      // The number of bytes the TCPM will write
      FIELD(val0, TX_BYTE_COUNT, 0x00, 0x08)
    } TxByteCount;

    struct TxBufHeaderByte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_HEADER_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of message header
      FIELD(val0, TX_BUF_HEADER_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of message header
      FIELD(val1, TX_BUF_HEADER_BYTE_1, 0x00, 0x08)
    } TxBufHeaderByte01;

    // TX objects
    struct TxBufObj1Byte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ1_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 1st data object
      FIELD(val0, TX_BUF_OBJ1_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 1st data object
      FIELD(val1, TX_BUF_OBJ1_BYTE_1, 0x00, 0x08)
    } TxBufObj1Byte01;
    struct TxBufObj1Byte23t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ1_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 1st data object
      FIELD(val0, TX_BUF_OBJ1_BYTE_2, 0x00, 0x08)
      // Byte 3(bits 31..24)of 1st data object
      FIELD(val1, TX_BUF_OBJ1_BYTE_3, 0x00, 0x08)
    } TxBufObj1Byte23;

    struct TxBufObj2Byte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ2_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 2st data object
      FIELD(val0, TX_BUF_OBJ1_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 2st data object
      FIELD(val1, TX_BUF_OBJ2_BYTE_1, 0x00, 0x08)
    } TxBufObj2Byte01;
    struct TxBufObj2Byte23t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ2_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 2st data object
      FIELD(val0, TX_BUF_OBJ2_BYTE_2, 0x00, 0x08)
      // Byte 3(bits 31..24)of 2st data object
      FIELD(val1, TX_BUF_OBJ2_BYTE_3, 0x00, 0x08)
    } TxBufObj2Byte23;

    struct TxBufObj3Byte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ3_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 3st data object
      FIELD(val0, TX_BUF_OBJ3_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 3st data object
      FIELD(val1, TX_BUF_OBJ3_BYTE_1, 0x00, 0x08)
    } TxBufObj3Byte01;
    struct TxBufObj3Byte23t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ3_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 3st data object
      FIELD(val0, TX_BUF_OBJ3_BYTE_2, 0x00, 0x08)
      // Byte 3(bits 31..24)of 3st data object
      FIELD(val1, TX_BUF_OBJ3_BYTE_3, 0x00, 0x08)
    } TxBufObj3Byte23;

    struct TxBufObj4Byte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ4_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 4st data object
      FIELD(val0, TX_BUF_OBJ4_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 4st data object
      FIELD(val1, TX_BUF_OBJ4_BYTE_1, 0x00, 0x08)
    } TxBufObj4Byte01;
    struct TxBufObj4Byte23t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ4_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 4st data object
      FIELD(val0, TX_BUF_OBJ4_BYTE_2, 0x00, 0x08)
      // Byte 3(bits 31..24)of 4st data object
      FIELD(val1, TX_BUF_OBJ4_BYTE_3, 0x00, 0x08)
    } TxBufObj4Byte23;

    struct TxBufObj5Byte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ5_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 5st data object
      FIELD(val0, TX_BUF_OBJ5_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 5st data object
      FIELD(val1, TX_BUF_OBJ5_BYTE_1, 0x00, 0x08)
    } TxBufObj5Byte01;
    struct TxBufObj5Byte23t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ5_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 5st data object
      FIELD(val0, TX_BUF_OBJ5_BYTE_2, 0x00, 0x08)
      // Byte 3(bits 31..24)of 5st data object
      FIELD(val1, TX_BUF_OBJ5_BYTE_3, 0x00, 0x08)
    } TxBufObj5Byte23;

    struct TxBufObj6Byte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ6_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 6st data object
      FIELD(val0, TX_BUF_OBJ6_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 6st data object
      FIELD(val1, TX_BUF_OBJ6_BYTE_1, 0x00, 0x08)
    } TxBufObj6Byte01;
    struct TxBufObj6Byte23t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ6_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 6st data object
      FIELD(val0, TX_BUF_OBJ6_BYTE_2, 0x00, 0x08)
      // Byte 3(bits 31..24)of 6st data object
      FIELD(val1, TX_BUF_OBJ6_BYTE_3, 0x00, 0x08)
    } TxBufObj6Byte23;

    struct TxBufObj7Byte01t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ7_BYTE_01_ADDR;

      // Byte 0 (bits 7..0) of 7st data object
      FIELD(val0, TX_BUF_OBJ7_BYTE_0, 0x00, 0x08)
      // Byte 1 (bits 15..8) of 7st data object
      FIELD(val1, TX_BUF_OBJ7_BYTE_1, 0x00, 0x08)
    } TxBufObj7Byte01;
    struct TxBufObj7Byte23t {
      byte val0, val1;
      uint8_t addr = TX_BUF_OBJ7_BYTE_23_ADDR;

      // Byte 2 (bits 23..16) of 7st data object
      FIELD(val0, TX_BUF_OBJ7_BYTE_2, 0x00, 0x08)
      // Byte 3(bits 31..24)of 7st data object
      FIELD(val1, TX_BUF_OBJ7_BYTE_3, 0x00, 0x08)
    } TxBufObj7Byte23;

    // Stuff...

    struct Diverse1t {
      byte val0, val1;
      uint8_t addr = DIVERSE_1_ADDR;

      // 24M oscillator for BMC communication
      // 0b : Disable 24M oscillator
      // 1b : Enable 24M oscillator (default)
      // Note : 24M oscillator will be enabled automatically when INT occur.
      FIELD(val0, BMCIO_OSC_EN, 0x00, 0x01)

      // VBUS detection enable
      // 0b : Measure off
      // 1b : Operation (default)
      FIELD(val0, VBUS_DETEN, 0x01, 0x01)

      // BMCIO BandGap enable
      // 0b : BandGap off CC pin function disable
      // 1b : BandGap on (default) CC pin function enable
      FIELD(val0, BMCIO_BG_EN, 0x02, 0x01)

      // Low power mode enable
      // 0b : Standby mode (default)
      //  1b : Low power
      FIELD(val0, BMCIO_LPEN, 0x03, 0x01)

      // Low power mode enable
      // 0b : Low power mode RD (default)
      // 1b : Low power mode RP
      FIELD(val0, BMCIO_LPRPRD, 0x04, 0x01)

      // VCONN OVP occurs and discharge path turn-on
      // 0b : No discharge (default)
      // 1b : Discharg
      FIELD(val0, VCONN_DISCHARGE_EN, 0x05, 0x01)
    } Diverse1;

    struct BmcioVconocpt {
      byte val0, val1;
      uint8_t addr = BMCIO_VCONOCP_ADDR;

      // VCONN over-current control selection
      // 000b : Current level = 200mA
      // 001b : Current level = 300mA
      // 010b : Current level = 400mA
      // 011b : Current level = 500mA
      // 100b : Current level = 600mA (default)
      // 101 to 111b : Reserved
      // If VCONN OCP trigger, the switch turn off timing under 55μs.
      FIELD(val0, BMCIO_VCONOCP, 0x05, 0x03)
    } BmcioVconocp;

    struct RtStt {
      byte val0, val1;
      uint8_t addr = RT_ST_ADDR;

      // 0b : VBUS over 0.8V (default)
      // 1b : VBUS under 0.8V
      FIELD_RO(val0, VBUS_80, 0x01, 0x01)
    } RtSt;

    struct RtIntt {
      byte val0, val1;
      uint8_t addr = RT_INT_ADDR;

      // 0b : Cleared (default)
      // 1b : Low power mode exited
      FIELD(val0, INT_WAKEUP, 0x00, 0x01)

      // 0b : VBUS without under 0.8V (default)
      // 1b : VBUS under 0.8V
      FIELD(val0, INT_VBUS_80, 0x01, 0x01)

      // 0b : Cleared (default)
      // 1b : Ra detach
      FIELD(val0, NT_RA_DETACH, 0x05, 0x01)
    } RtInt;

    struct RtMaskt {
      byte val0, val1;
      uint8_t addr = RT_MASK_ADDR;

      FIELD(val0, M_WAKEUP, 0x00, 0x01)
      FIELD(val0, M_VBUS_80, 0x01, 0x01)
      FIELD(val0, M_RA_DETACH, 0x05, 0x01)
    } RtMask;

    struct Diverse2t {
      byte val0, val1;
      uint8_t addr = DIVERSE_2_ADDR;

      // Enter idle mode timeout time = (AUTOIDLE_TIMEOUT*2+1)*6.4ms
      FIELD(val0, AUTOIDLE_TIMEOUT, 0x00, 0x03)

      // 1 : Auto enter idle mode enable (default)
      // 0 : Auto enter idle mode disable
      FIELD(val0, AUTOIDLE_EN, 0x03, 0x01)

      // 0 : Disable PD3.0 Extended message (default)
      // 1 : Enable PD3.0 Extended message affect GoodCRC receive detect between
      // PD2.0 and PD3.0
      FIELD(val0, ENEXTMSG, 0x04, 0x01)

      // 0 : Shutdown mode (default)
      // 1 : Non-Shutdown mode
      FIELD(val0, Shutdown_OFF, 0x05, 0x01)

      // 0b : Clock_320K from Clock_320K
      // 1b : Clock_300K divided from Clock_24M (default)
      FIELD(val0, CK_300K_SEL, 0x07, 0x01)
    } Diverse2;

    struct WakeUpEnt {
      byte val0, val1;
      uint8_t addr = WAKE_UP_EN_ADDR;

      // 1 : Wakeup function disable
      // 1 : Wakeup function enable (default)
      FIELD(val0, WAKEUP_EN, 0x07, 0x01)
    } WakeUpEn;

    struct SoftResett {
      byte val0, val1;
      uint8_t addr = SOFT_RESET_ADDR;

      // Write 1 to trigger software reset.
      FIELD_WO(val0, SOFT_RESET, 0x00, 0x01)
    } SoftReset;

    struct TDRPt {
      byte val0, val1;
      uint8_t addr = TDRP_ADDR;

      // The period a DRP will complete a Source to Sink and back advertisement.
      // (Period = TDRP * 6.4 + 51.2ms)
      // 0000 : 51.2ms
      // 0001 : 57.6ms
      // 0010 : 64ms
      // 0011 : 70.4ms (default)
      // …
      // 1110 : 140.8ms
      // 1111 : 147.2ms
      FIELD(val0, TDRP, 0x00, 0x04)
    } TDRP;

    struct DCSRCDRPt {
      byte val0, val1;
      uint8_t addr = DCSRCDRP_ADDR;

      // The percent of time that a DRP will advertise Source during tDRP.
      // (DUTY = (DCSRCDRP[9:0] + 1) / 1024)
      // 0000000000 : 1/1024
      // 0000000001 : 2/1024
      // …
      // 0101000111 : 328/1024 (default)
      // …
      // 1111111110 : 1023/1024
      // 1111111111 : 1024/1024
      // Note : Setting with 0xA4[9:8]
      FIELD(val0, TDRP_0, 0x00, 0x08)
      FIELD(val1, TDRP_1, 0x00, 0x02)
    } DCSRCDRP;

  };  // Regt

  // private:
  static bool readDataReg(const byte regAddress, byte* dataVal,
                          const uint8_t arrLen);
  static bool writeDataReg(const byte regAddress, byte dataVal0, byte dataVal1);
  bool read2ByteReg(byte regAddress, byte* val0, byte* val1);
};

}  // namespace rt1715

#endif