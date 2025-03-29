/**************************************************************************/
/*!
  @file     BQ76905.h
  @author   Baptiste Hudyma

Library for basic interfacing with BQ76905 battery balancer IC from TI

*/
/**************************************************************************/

#ifndef BQ76905_H
#define BQ76905_H

#include "src/system/platform/i2c.h"
#include <cstdint>

namespace bq76905 {

using byte = uint8_t;

// Platform specific
constexpr uint8_t i2cObjectIndex = 0;
// \Platform specific

constexpr bool usesStopBit = true;

constexpr uint16_t DEVICE_NUMBER = 0x7605;

// addresses

constexpr uint8_t SAFETY_ALERT_A_ADDR = 0x02;
constexpr uint8_t SAFETY_ALERT_B_ADDR = 0x04;

constexpr uint8_t BATTERY_STATUS_ADDR = 0x12;

constexpr uint8_t CELL_1_VOLTAGE_ADDR = 0x14;
constexpr uint8_t CELL_2_VOLTAGE_ADDR = 0x16;
constexpr uint8_t CELL_3_VOLTAGE_ADDR = 0x18;
constexpr uint8_t CELL_4_VOLTAGE_ADDR = 0x1A;
constexpr uint8_t CELL_5_VOLTAGE_ADDR = 0x1C;

constexpr uint8_t REG_18_VOLTAGE_ADDR = 0x22;

constexpr uint8_t VSS_VOLTAGE_ADDR = 0x24;
constexpr uint8_t STACK_VOLTAGE_ADDR = 0x26;

constexpr uint8_t INT_TEMPERATURE_ADDR = 0x28;
constexpr uint8_t TS_MEASURMENT_ADDR = 0x2A;

constexpr uint8_t RAW_CURRENT_ADDR = 0x36;
constexpr uint8_t CURRENT_ADDR = 0x3A;
constexpr uint8_t CC1_CURRENT_ADDR = 0x3C;

constexpr uint8_t ALARM_STATUS_ADDR = 0x62;
constexpr uint8_t ALARM_RAW_STATUS_ADDR = 0x64;
constexpr uint8_t ALARM_ENABLE_ADDR = 0x66;

constexpr uint8_t FET_CONTROL_ADDR = 0x68;
constexpr uint8_t REGOUT_CONTROL_ADDR = 0x69;

constexpr uint8_t DSG_FET_DRIVER_PWM_CONTROL_ADDR = 0x6A;
constexpr uint8_t CHG_FET_DRIVER_PWM_CONTROL_ADDR = 0x6C;

// adresses for subcommands

constexpr uint8_t SUBCOMMAND_ADDR = 0x3E;
constexpr uint8_t SUBCOMMAND_DATA_ADDR = 0x40;
constexpr uint8_t SUBCOMMAND_DATA_CHECKSUM_ADDR = 0x60;

constexpr uint16_t SUBCOMMAND_DEVICE_NUMBER_ADDR = 0x0001;
constexpr uint16_t SUBCOMMAND_FW_VERSION_ADDR = 0x0002;
constexpr uint16_t SUBCOMMAND_HW_VERSION_ADDR = 0x0003;

constexpr uint16_t SUBCOMMAND_PASSQ_ADDR = 0x0003;
constexpr uint16_t SUBCOMMAND_RESET_PASSQ_ADDR = 0x0005;

constexpr uint16_t SUBCOMMAND_EXIT_DEEPSLEEP_ADDR = 0x000E;
constexpr uint16_t SUBCOMMAND_DEEPSLEEP_ADDR = 0x000F;
constexpr uint16_t SUBCOMMAND_SHUTDOWN_ADDR = 0x0010;
constexpr uint16_t SUBCOMMAND_RESET_ADDR = 0x0012;

constexpr uint16_t SUBCOMMAND_FET_ENABLE_ADDR = 0x0022;

constexpr uint16_t SUBCOMMAND_SEAL_ADDR = 0x0030;

constexpr uint16_t SUBCOMMAND_SECURITY_KEYS_ADDR = 0x0035;

constexpr uint16_t SUBCOMMAND_CB_ACTIVE_CELLS_ADDR = 0x0083;

constexpr uint16_t SUBCOMMAND_SET_CFGUPDATE_ADDR = 0x0090;
constexpr uint16_t SUBCOMMAND_EXIT_CFGUPDATE_ADDR = 0x0092;

constexpr uint16_t SUBCOMMAND_PROG_TIMER_ADDR = 0x0094;

constexpr uint16_t SUBCOMMAND_SLEEP_ENABLE_ADDR = 0x0099;
constexpr uint16_t SUBCOMMAND_SLEEP_DISABLE_ADDR = 0x009A;

constexpr uint16_t SUBCOMMAND_PROT_RECOVERY_ADDR = 0x009B;

// configuration registers

// most of them are not implemented to prevent missuse

constexpr uint16_t SUBCOMMAND_CONFIGURATION_DA_ADDR = 0x9019;
constexpr uint16_t SUBCOMMAND_CONFIGURATION_VCELL_MODE_ADDR = 0x901B;
constexpr uint16_t SUBCOMMAND_CONFIGURATION_TS_OFFSET_ADDR = 0x900E;

// compute data checksum for communication
uint8_t checksum(uint8_t lenght, uint8_t* data)
{
  uint8_t checksum = 0;
  for (uint8_t i = 0; i < lenght; ++i)
    checksum += data[i];
  checksum = 0xff & (~checksum);
  return checksum;
}

class BQ76905
{
public:
  BQ76905() {};
  // Initialise the variable here, but it will be written from the main program
  static const byte BQ76905addr = batteryBalancerI2cAddress; // I2C address

  template<typename T> bool readRegEx(T& dataParam)
  {
    // This is a function for reading data words.
    // The number of bytes that make up a word is 2.
    constexpr uint8_t arrLen = 2;
    // Create an array to hold the returned data
    byte valBytes[arrLen];

    if (readDataReg(dataParam.addr, valBytes, arrLen))
    {
      // Cycle through array of data
      dataParam.val0 = (byte)valBytes[0];
      dataParam.val1 = (byte)valBytes[1];
      return true;
    }
    return false;
  }
  // used by external functions to write registers
  template<typename T> static bool writeRegEx(T dataParam)
  {
    byte valBytes[2];
    valBytes[0] = dataParam.val0;
    valBytes[1] = dataParam.val1;
    return writeData(dataParam.addr, 2, valBytes);
  }
// macro to generate bit mask to access bits
#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
// macro to read bits from variable, using mask
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
// macro to write bits into variable, using mask
#define WRITETO(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))
// macro to wrap functions for easy access
// if name is called with empty brackets, read bits and return value
// if name is prefixed with set_, write value in brackets into bits defined in
// FIELD
#define FIELD(data, name, index, size)                                 \
  inline decltype(data) name() { return READFROM(data, index, size); } \
  inline void set_##name(decltype(data) value) { WRITETO(data, index, size, value); }

// macro to wrap functions for easy access, read only
// if name is called with empty brackets, read bits and return value
#define FIELD_RO(data, name, index, size) \
  inline decltype(data) name() { return READFROM(data, index, size); }

  // used to set and reset the error flag
  inline static bool isFlagRaised = false;

  // Base class for register operations
  struct IBaseReadRegister
  {
    virtual ~IBaseReadRegister() {};

    virtual uint16_t address() const = 0;

    uint16_t get()
    {
      byte valBytes[2];
      if (readDataReg(address(), valBytes, 2))
      {
        // Cycle through array of data
        byte val0 = (byte)valBytes[0];
        byte val1 = (byte)valBytes[1];

        // fuse them
        uint16_t res = val1 << 8 | val0;
        // convert to real units
        return res;
      }
      isFlagRaised = true;
      // error
      return 0;
    }
  };

  struct ISubcommandRegister
  {
    virtual ~ISubcommandRegister() {};

    virtual uint16_t command_address() const = 0;

  protected:
    void raw_send()
    {
      // write command header
      if (!writeData16(SUBCOMMAND_ADDR, command_address()))
      {
        isFlagRaised = true;
        return;
      }
    }
  };

  struct ISubcommandReadRegister : public ISubcommandRegister
  {
    virtual ~ISubcommandReadRegister() {};

  protected:
    uint32_t raw_read()
    {
      // set command
      raw_send();

      delay_us(2000);

      constexpr uint8_t dataSize = 12;
      byte valBytes[dataSize];
      if (readDataReg(SUBCOMMAND_DATA_ADDR, valBytes, dataSize))
      {
        // fuse them (TODO: return the whole register instead of just 32 bits)
        uint32_t res = valBytes[3] << 24 | valBytes[2] << 16 | valBytes[1] << 8 | valBytes[0];
        return res;
      }
      isFlagRaised = true;
      return 0;
    }

    uint32_t raw_read_big_endian()
    {
      // set command
      raw_send();

      delay_us(2000);

      byte valBytes[12];
      if (readDataReg(SUBCOMMAND_DATA_ADDR, valBytes, 12))
      {
        // fuse them (TODO: return the whole register instead of just 32 bits)
        uint32_t res = valBytes[2] << 24 | valBytes[3] << 16 | valBytes[0] << 8 | valBytes[1];
        return res;
      }
      isFlagRaised = true;
      return 0;
    }

    void raw_write8(uint8_t data)
    {
      const uint8_t dataSize = 3;

      // 2 bytes for adress (always), max data to send on 32 bits
      byte valBytes[dataSize];
      valBytes[0] = command_address() & 0xff;
      valBytes[1] = (command_address() >> 8) & 0xff;
      valBytes[2] = data;

      raw_write(dataSize, valBytes);
    }

    void raw_write16(uint16_t data)
    {
      const uint8_t dataSize = 4;

      // 2 bytes for adress (always), max data to send on 32 bits
      byte valBytes[dataSize];
      valBytes[0] = command_address() & 0xff;
      valBytes[1] = (command_address() >> 8) & 0xff;
      valBytes[2] = data & 0xff;
      valBytes[3] = (data >> 8) & 0xff;

      raw_write(dataSize, valBytes);
    }

  private:
    // internal command write
    void raw_write(uint8_t lenght, uint8_t* data)
    {
      if (not writeData(SUBCOMMAND_ADDR, lenght, data))
      {
        isFlagRaised = true;
        return;
      }

      delay_us(2000);

      byte checkSumData[2];
      checkSumData[0] = checksum(lenght, data);
      checkSumData[1] = lenght + 2;

      // Write Data Checksum and length, required for RAM writes
      if (not writeData(SUBCOMMAND_DATA_CHECKSUM_ADDR, 2, checkSumData))
      {
        isFlagRaised = true;
        return;
      }
    }
  };

  struct Regt
  {
    // Provides individual alert signals when enabled safety alerts have triggered
    // Provides individual fault signals when enabled safety faults have triggered
    struct SafetyAlertAt
    {
      uint8_t addr = SAFETY_ALERT_A_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;
      // Cell Overvoltage Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, COV_ALERT, 0x07, 0x01)
      // Cell Undervoltage Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, CUV_ALERT, 0x06, 0x01)
      // Short Circuit in Discharge Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, SCD_ALERT, 0x05, 0x01)
      // Overcurrent in Discharge 1 Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, OCD1_ALERT, 0x04, 0x01)
      // Overcurrent in Discharge 2 Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, OCD2_ALERT, 0x03, 0x01)
      // Overcurrent in Charge Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, OCC_ALERT, 0x02, 0x01)

      // Cell Overvoltage Safety Fault
      //     0 = Indicates protection fault has not triggered
      //     1 = indicates protection fault has triggered
      FIELD_RO(val1, COV_FAULT, 0x07, 0x01)
      // Cell Undervoltage Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, CUV_FAULT, 0x06, 0x01)
      // Short Circuit in Discharge Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, SCD_FAULT, 0x05, 0x01)
      // Overcurrent in Discharge 1 Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, OCD1_FAULT, 0x04, 0x01)
      // Overcurrent in Discharge 2 Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, OCD2_FAULT, 0x03, 0x01)
      // Overcurrent in Charge Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, OCC_FAULT, 0x02, 0x01)
      // Current Protection Latch Safety Fault
      //   0 = Indicates the number of attempted current protection recoveries has not yet exceeded the latch count.
      //   1 = Indicates the number of attempted current protection recoveries has exceeded the latch count, and
      //   autorecovery based on time is disabled
      FIELD_RO(val1, CURLATCH, 0x01, 0x01)
      // REGOUT Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, REGOUT, 0x00, 0x01)
    } safetyAlertA;

    // Provides individual alert signals when enabled safety alerts have triggered.
    // Provides individual fault signals when enabled safety faults have triggered.
    struct SafetyAlertBt
    {
      uint8_t addr = SAFETY_ALERT_B_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // Overtemperature in Discharge Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, OTD_ALERT, 0x07, 0x01)
      // Overtemperature in Charge Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, OTC_ALERT, 0x06, 0x01)
      // Undertemperature in Discharge Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, UTD_ALERT, 0x05, 0x01)
      // Undertemperature in Charge Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, UTC_ALERT, 0x04, 0x01)
      // Internal Overtemperature Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, OTINT_ALERT, 0x03, 0x01)
      // Host Watchdog Safety Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, HWD_ALERT, 0x02, 0x01)
      // VREF Measurement Diagnostic Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, VREF_ALERT, 0x01, 0x01)
      // VSS Measurement Diagnostic Alert
      //   0 = Indicates protection alert has not triggered
      //   1 = indicates protection alert has triggered
      FIELD_RO(val0, VSS_ALERT, 0x00, 0x01)

      // Overtemperature in Discharge Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, OTD_FAULT, 0x07, 0x01)
      // Overtemperature in Charge Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, OTC_FAULT, 0x06, 0x01)
      // Undertemperature in Discharge Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, UTD_FAULT, 0x05, 0x01)
      // Undertemperature in Charge Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, UTC_FAULT, 0x04, 0x01)
      // Internal Overtemperature Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, OTINT_FAULT, 0x03, 0x01)
      // Host Watchdog Safety Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, HWD_FAULT, 0x02, 0x01)
      // VREF Measurement Diagnostic Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, VREF_FAULT, 0x01, 0x01)
      // VSS Measurement Diagnostic Fault
      //   0 = Indicates protection fault has not triggered
      //   1 = indicates protection fault has triggered
      FIELD_RO(val1, VSS_FAULT, 0x00, 0x01)
    } safetyAlertB;

    // Provides flags related to battery status.
    struct BatteryStatust
    {
      uint8_t addr = BATTERY_STATUS_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // This bit is set when the device fully resets. It is cleared upon exit of CONFIG_UPDATE mode. It can be used by
      // the host to determine if any RAM configuration changes were lost due to a reset.
      //   0 = Full reset has not occurred since last exit of CONFIG_UPDATE mode.
      //   1 = Full reset has occurred since last exit of CONFIG_UPDATE and reconfiguration of any RAM settings is
      //   required.
      FIELD_RO(val0, POR, 0x07, 0x01)
      // This bit indicates whether or not SLEEP mode is allowed based on configuration and commands.
      // TheSettings:Configuration:Power Config[SLEEP_EN] bit sets the default state of this bit. The host can send
      // commands to enable or disable SLEEP mode based on system requirements. When this bit is set, the device can
      // transition to SLEEP mode when other SLEEP criteria are met.
      //   0 = SLEEP mode is disabled by the host.
      //   1 = SLEEP mode is allowed when other SLEEP conditions are met.
      FIELD_RO(val0, SLEEP_EN, 0x06, 0x01)
      // This bit indicates whether or not the device is in CONFIG_UPDATE mode. It is set after the SET_CFGUPDATE()
      // subcommand is received and fully processed. Configuration settings can be changed only while this bit is set.
      //   0 = Device is not in CONFIG_UPDATE mode.
      //   1 = Device is in CONFIG_UPDATE mode.
      FIELD_RO(val0, CFGUPDATE, 0x05, 0x01)
      // This bit indicates whether the ALERT pin is asserted (pulled low).
      //   0 = ALERT pin is not asserted (stays in hi-Z mode).
      //   1 = ALERT pin is asserted (pulled low)
      FIELD_RO(val0, ALERTPIN, 0x04, 0x01)
      // This bit indicates whether the CHG driver is enabled.
      //   0 = CHG driver is disabled.
      //   1 = CHG driver is enabled
      FIELD_RO(val0, CHG, 0x03, 0x01)
      // This bit indicates whether the DSG driver is enabled.
      //   0 = DSG driver is disabled.
      //   1 = DSG driver is enabled
      FIELD_RO(val0, DSG, 0x02, 0x01)
      // This bit indicates the value of the debounced CHG Detector signal.
      //   0 = CHG Detector debounced signal is low.
      //   1 = CHG Detector debounced signal is high.
      FIELD_RO(val0, CHGDETFLAG, 0x01, 0x01)

      // This flag asserts if the device is in SLEEP mode
      //   0 = Device is not in SLEEP mode
      //   1 = Device is in SLEEP mode
      FIELD_RO(val1, SLEEP, 0x07, 0x01)
      // This flag asserts if the device is in DEEPSLEEP mode
      //   0 = Device is not in DEEPSLEEP mode
      //   1 = Device is in DEEPSLEEP mode
      FIELD_RO(val1, DEEPSLEEP, 0x06, 0x01)
      // This flag asserts if an enabled safety alert is present.
      //   0 = Indicates an enabled safety alert is not present
      //   1 = Indicates an enabled safety alert is present
      FIELD_RO(val1, SA, 0x05, 0x01)
      // This flag asserts if an enabled safety fault is present.
      //   0 = Indicates an enabled safety fault is not present
      //   1 = Indicates an enabled safety fault is present
      FIELD_RO(val1, SS, 0x04, 0x01)
      // SEC1:0 indicate the present security state of the device.
      //   When in SEALED mode, device configuration cannot be read or written and some
      //   commands are restricted.
      //   When in FULLACCESS mode, unrestricted read and write access is allowed and all
      //   commands are accepted.
      //   0 = 0: Device has not initialized yet.
      //   1 = 1: Device is in FULLACCESS mode.
      //   2 = 2: Unused.
      //   3 = 3: Device is in SEALED mode.
      FIELD_RO(val1, SEC, 0x02, 0x02)
      // This bit is set when the device is in autonomous FET control mode. The default value of this bit is set by the
      // Settings:FET Options[FET_EN] bit in Data Memory upon exit of CONFIG_UPDATE mode. Its value can be modified
      // during operation using the FET_ENABLE() subcommand.
      //   0 = Device is not in autonomous FET control mode, FETs are only enabled through manual command.
      //   1 = Device is in autonomous FET control mode, FETs can be enabled by the device if no conditions or commands
      //   prevent them being enabled.
      FIELD_RO(val1, FET_EN, 0x00, 0x01)
    } batteryStatus;

    // Latched signal used to assert the ALERT pin. Write a bit high to clear the latched bit.
    struct AlarmStatust
    {
      uint8_t addr = ALARM_STATUS_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // This bit is latched when a full scan is complete (including cell voltages, top-of-stack voltage, temperature,
      // and diagnostic measurements), and the bit is included in the mask. The bit is cleared when written with a "1".
      // A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val0, FULLSCAN, 0x07, 0x01)
      // This bit is latched when a voltage ADC measurement scan is complete (this includes the cell voltage
      // measurements and one additional measurement), and the bit is included in the mask. The bit is cleared when
      // written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val0, ADSCAN, 0x06, 0x01)
      // This bit is latched when the device is wakened from SLEEP mode, and the bit is included in the mask. The bit is
      // cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val0, WAKE, 0x05, 0x01)
      // This bit is latched when the device enters SLEEP mode, and the bit is included in the mask. The bit is cleared
      // when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val0, SLEEP, 0x04, 0x01)
      // This bit is latched when the programmable timer expires, and the bit is included in the mask. The bit is
      // cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val0, TIMER_ALARM, 0x03, 0x01)
      // This bit is latched when the device completes the startup measurement sequence (which runs after an initial
      // powerup, after a device reset, when the device exits CONFIG_UPDATE mode, and when it exits DEEPSLEEP mode) and
      // the bit is included in the mask. The bit is cleared when written with a "1". A bit set here causes the ALERT
      // pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val0, INITCOMP, 0x02, 0x01)
      // This bit is latched when the debounced CHG Detector signal is different from the last debounced value.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val0, CDTOGGLE, 0x01, 0x01)
      // This bit is latched when the POR bit in Battery Status is asserted.
      //  0 = Flag is not set
      //  1 = Flag is set
      FIELD(val0, POR, 0x00, 0x01)

      // This bit is latched when a bit in Safety Status B() is set, and the bit is included in the mask. The bit is
      // cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val1, SSB, 0x06, 0x01)
      // This bit is latched when a bit in Safety Alert A() is set, and the bit is included in the mask. The bit is
      // cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val1, SAA, 0x05, 0x01)
      // This bit is latched when a bit in Safety Alert B() is set, and the bit is included in the mask. The bit is
      // cleared when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val1, SAB, 0x04, 0x01)
      // This bit is latched when the CHG driver is disabled, and the bit is included in the mask. The bit is cleared
      // when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val1, XCHG, 0x03, 0x01)
      // This bit is latched when the DSG driver is disabled, and the bit is included in the mask. The bit is cleared
      // when written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val1, XDSG, 0x02, 0x01)
      // This bit is latched when either a cell voltage has been measured below Shutdown Cell Voltage, or the stack
      // voltage has been measured below Shutdown Stack Voltage. The bit is cleared when written with a "1". A bit set
      // here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val1, SHUTV, 0x01, 0x01)
      // This bit is latched when cell balancing is active, and the bit is included in the mask. The bit is cleared when
      // written with a "1". A bit set here causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD(val1, CB, 0x00, 0x01)
    } alarmStatus;

    // Unlatched value of flags which can be selected to be latched (using Alarm Enable()) and used to assert the ALERT
    // pin.
    struct AlarmRawStatust
    {
      uint8_t addr = ALARM_RAW_STATUS_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // This bit pulses high briefly when a full scan is complete (including cell voltages, top-of-stack voltage,
      // temperature, and diagnostic measurements).
      FIELD_RO(val0, FULLSCAN, 0x07, 0x01)
      // This bit pulses high briefly when a voltage ADC measurement scan is complete (this includes the cell voltage
      // measurements and one additional measurement).
      FIELD_RO(val0, ADSCAN, 0x06, 0x01)
      // This bit pulses high briefly when the device is wakened from SLEEP mode.
      FIELD_RO(val0, WAKE, 0x05, 0x01)
      // This bit pulses high briefly when the device enters SLEEP mode.
      //  0 = Flag is not set
      //  1 = Flag is set
      FIELD_RO(val0, SLEEP, 0x04, 0x01)
      // This bit pulses high briefly when the programmable timer expires.
      FIELD_RO(val0, TIMER_ALARM, 0x03, 0x01)
      // This bit pulses high briefly when the device has completed the startup measurement sequence (after powerup or
      // reset or exit of CONFIG_UPDATE mode or exit of DEEPSLEEP).
      FIELD_RO(val0, INITCOMP, 0x02, 0x01)
      // This bit is set when the CHG Detector output is set, indicating that the CHG pin has been detected above a
      // level of approximately 2 V.
      //   0 = CHG Detector output is not set
      //   1 = CHG Detector output is set
      FIELD_RO(val0, CDRAW, 0x01, 0x01)
      // This bit is set if the POR bit in Battery Status is asserted.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val0, POR, 0x00, 0x01)

      // This bit is set when a bit in Safety Status A() is set.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val1, SSA, 0x07, 0x01)
      // This bit is set when a bit in Safety Status B() is set.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val1, SSB, 0x06, 0x01)
      // This bit is set when a bit in Safety Alert A() is set.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val1, SAA, 0x05, 0x01)
      // This bit is set when a bit in Safety Alert B() is set.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val1, SAB, 0x04, 0x01)
      // This bit is set when the CHG driver is disabled.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val1, XCHG, 0x03, 0x01)
      // This bit is set when the DSG driver is disabled.
      //  0 = Flag is not set
      //  1 = Flag is set
      FIELD_RO(val1, XDSG, 0x02, 0x01)
      // This bit is set when either a cell voltage has been measured below Shutdown Cell Voltage, or the stack voltage
      // has been measured below Shutdown Stack Voltage. The bit is cleared when written with a "1". A bit set here
      // causes the ALERT pin to be asserted low.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val1, SHUTV, 0x01, 0x01)
      // This bit is set when cell balancing is active.
      //   0 = Flag is not set
      //   1 = Flag is set
      FIELD_RO(val1, CB, 0x00, 0x01)

    } alarmRawStatus;

    // Mask for Alarm Status(). Can be written to change during operation to change which alarm sources are enabled. The
    // default value of this parameter is set by Settings:Configuration:Default Alarm Mask
    struct AlarmEnablet
    {
      uint8_t addr = ALARM_ENABLE_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //   0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //   1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val0, FULLSCAN, 0x07, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //   0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //   1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val0, ADSCAN, 0x06, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //   0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //   1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val0, WAKE, 0x05, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //   0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //   1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val0, SLEEP, 0x04, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //   0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //   1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val0, TIMER_ALARM, 0x03, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //   0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //   1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val0, INITCOMP, 0x02, 0x01)
      // Setting this bit allows the internally determined value of CDTOGGLE to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      // This flag is set whenever the debounced CHG Detector signal differs from the previous debounced value.
      //   0 = The CDTOGGLE signal is not included in Alarm Status()
      //   1 = The CDTOGGLE signal is included in Alarm Status()
      FIELD(val0, CDTOGGLE, 0x01, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //   0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //   1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val0, POR, 0x00, 0x01)

      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, SSA, 0x07, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, SSB, 0x06, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, SAA, 0x05, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, SAB, 0x04, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, XCHG, 0x03, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, XDSG, 0x02, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, SHUTV, 0x01, 0x01)
      // Setting this bit allows the corresponding bit in Alarm Raw Status() to be mapped to the corresponding bit in
      // Alarm Status() and to control the ALERT pin.
      //  0 = This bit in Alarm Raw Status() is not included in Alarm Status()
      //  1 = This bit in Alarm Raw Status() is included in Alarm Status()
      FIELD(val1, CB, 0x00, 0x01)
    } alarmEnable;

    // FET Control: Allows host control of individual FET drivers.
    struct FetControlt
    {
      uint8_t addr = FET_CONTROL_ADDR;
      byte val0 = 0x00;

      // CHG FET driver control. This bit only operates if the HOST_FETOFF_EN bit in data memory is set.
      //   0 = CHG FET driver is allowed to turn on if other conditions are met.
      //   1 = CHG FET driver is forced off.
      FIELD(val0, CHG_OFF, 0x03, 0x01)
      // DSG FET driver control. This bit only operates if the HOST_FETOFF_EN bit in data memory is set.
      //  0 = DSG FET driver is allowed to turn on if other conditions are met.
      //  1 = DSG FET driver is forced off.
      FIELD(val0, DSG_OFF, 0x02, 0x01)
      // CHG FET driver control. This bit only operates if the HOST_FETON_EN bit in data memory is set.
      //  0 = CHG FET driver is allowed to turn on if other conditions are met.
      //  1 = CHG FET driver is forced on
      FIELD(val0, CHG_ON, 0x01, 0x01)
      // DSG FET driver control. This bit only operates if the HOST_FETON_EN bit in data memory is set.
      //  0 = DSG FET driver is allowed to turn on if other conditions are met.
      //  1 = DSG FET driver is forced on.
      FIELD(val0, DSG_ON, 0x00, 0x01)
    } fetControl;

    // REGOUT Control: Changes voltage regulator settings.
    struct RegoutControlt
    {
      uint8_t addr = REGOUT_CONTROL_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // Control for TS pullup to stay biased continuously.
      //  0 = TS pullup resistor is not continuously connected.
      //  1 = TS pullup resistor is continuously connected.
      FIELD(val0, TS_ON, 0x04, 0x01)
      // REGOUT LDO enable.
      //  0 = REGOUT LDO is disabled
      //  1 = REGOUT LDO is enabled
      FIELD(val0, REG_EN, 0x03, 0x01)
      // REGOUT LDO voltage control.
      //  0 = REGOUT LDO is set to 1.8V
      //  1 = REGOUT LDO is set to 1.8V
      //  2 = REGOUT LDO is set to 1.8V
      //  3 = REGOUT LDO is set to 1.8V
      //  4 = REGOUT LDO is set to 2.5 V
      //  5 = REGOUT LDO is set to 3.0 V
      //  6 = REGOUT LDO is set to 3.3 V
      //  7 = REGOUT LDO is set to 5 V
      FIELD(val0, REGOUTV, 0x00, 0x03)

    } regoutControl;

    // Controls the PWM mode of the DSG FET driver. Values are not used until the second byte is written.
    struct DsgFetDriverPwmControlt
    {
      uint8_t addr = DSG_FET_DRIVER_PWM_CONTROL_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // Time the DSG FET driver is disabled each cycle when PWM mode is enabled.
      // Settings from 122.1 μs to 31.128 ms in steps of 122.1 μs
      // A setting of 0 disables PWM mode, such that this command has no effect
      FIELD(val0, DSGPWMOFF, 0x00, 0x08)

      // DSG FET driver PWM mode control
      //  0 = DSG FET driver PWM mode is disabled
      //  1 = DSG FET driver PWM mode is enabled
      FIELD(val1, DSGPWMEN, 0x07, 0x01)
      // Time the DSG FET driver is enabled when PWM mode is enabled.
      // Settings from 30.52 μs to 3.876 ms in steps of 30.52 μs
      // A setting of 0 disables PWM mode, such that this command has no effect
      FIELD(val1, DSGPWMON, 0x00, 0x07)
    } dsgFetDriverPwmControl;

    // Controls the PWM mode of the CHG FET driver. Values are not used until the second byte is written.
    struct ChFetDriverPwmControlt
    {
      uint8_t addr = CHG_FET_DRIVER_PWM_CONTROL_ADDR;
      byte val0 = 0x00;
      byte val1 = 0x00;

      // Time the CHG FET driver is disabled each cycle when PWM mode is enabled.
      // Settings from 488.4 μs to 124.512 ms in steps of 488.4 μs
      // A setting of 0 disables PWM mode, such that this command has no effect.
      FIELD(val0, CHGPWMOFF, 0x00, 0x08)

      // CHG FET driver PWM mode control
      // 0 = CHG FET driver PWM mode is disabled
      // 1 = CHG FET driver PWM mode is enabled
      FIELD(val1, DSGPWMEN, 0x07, 0x01)
      // Time the CHG FET driver is enabled when PWM mode is enabled.
      // Settings from 30.52 μs to 3.876 ms in steps of 30.52 μs
      // A setting of 0 disables PWM mode, such that this command has no effect.
      FIELD(val1, CHGPWMON, 0x00, 0x07)
    } chFetDriverPwmControl;

    // 16-bit voltage on cell 1 (mV)
    struct Cell1Voltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return CELL_1_VOLTAGE_ADDR; };
    } cell1Voltage;

    // 16-bit voltage on cell 2 (mV)
    struct Cell2Voltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return CELL_2_VOLTAGE_ADDR; };
    } cell2Voltage;

    // 16-bit voltage on cell 3 (mV)
    struct Cell3Voltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return CELL_3_VOLTAGE_ADDR; };
    } cell3Voltage;

    // 16-bit voltage on cell 4 (mV)
    struct Cell4Voltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return CELL_4_VOLTAGE_ADDR; };
    } cell4Voltage;

    // 16-bit voltage on cell 5 (mV)
    struct Cell5Voltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return CELL_5_VOLTAGE_ADDR; };
    } cell5Voltage;

    // Internal 1.8V regulator voltage measured using bandgap reference, used for diagnostic of VREF1 vs VREF2.
    struct Reg18Voltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return REG_18_VOLTAGE_ADDR; };
    } reg18Voltage;

    // Measurement of VSS using ADC, used for diagnostic of ADC input mux (mV)
    struct VSSVoltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return VSS_VOLTAGE_ADDR; };
    } VSSVoltage;

    // 16-bit voltage on top of stack (mV)
    struct StackVoltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return STACK_VOLTAGE_ADDR; };
    } stackVoltage;

    // This is the most recent measured internal die temperature (degrees).
    struct IntTemperatureVoltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return INT_TEMPERATURE_ADDR; };
    } intTemperatureVoltage;

    // ADC measurement of the TS pin
    struct TsMeasurmentVoltaget : public IBaseReadRegister
    {
      uint16_t address() const override { return TS_MEASURMENT_ADDR; };
    } tsMeasurmentVoltage;

    /**
     *
     *    SUBCOMMANDS
     *
     */

    // The DEVICE_NUMBER subcommand reports the device number that identifies the product. The data is returned in
    // little-endian format
    struct DeviceNumbert : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_DEVICE_NUMBER_ADDR; }

      uint16_t get()
      {
        const uint32_t res = raw_read();
        const uint16_t number = res & UINT16_MAX;
        return number;
      }
    } deviceNumber;

    // The FW_VERSION subcommand returns three 16-bit word values.
    struct FirmwareVersiont : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_FW_VERSION_ADDR; }

      // Bytes 0-1: Device Number (Big-Endian): Device number in big-endian format for compatibility with legacy
      // products.
      uint16_t get_device_number()
      {
        const uint32_t res = raw_read_big_endian();
        // first two bytes
        const uint16_t number = res & UINT16_MAX;
        return number;
      }

      // Bytes 3-2: Firmware Version (Big-Endian): Device firmware major and minor version number (Big-Endian).
      uint16_t get_firmware_version()
      {
        const uint32_t res = raw_read_big_endian();
        // first two bytes
        const uint16_t number = (res >> 16) & UINT16_MAX;
        return number;
      }

      // not supported yet
      // Bytes 5-4: Build Number (Big-Endian): Firmware build number in big-endian, binary coded decimal format for
      // compatibility with legacy products. uint16_t get_build_number();
    } firmwareVersion;

    // Hardware Version: Reports the device hardware version number
    struct HardwareVersiont : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_HW_VERSION_ADDR; }

      uint16_t get()
      {
        const uint32_t res = raw_read();
        const uint16_t number = res & UINT16_MAX;
        return number;
      }
    } hardwareVersion;

    struct PassQt : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_PASSQ_ADDR; }

      // Accumulated charge lower 32-bits (little-endian byte-by-byte). Lower 32 bits of signed 48-bit result, with the
      // full 48-bit field having units of userA- seconds
      uint32_t get_PassQLsb()
      {
        const uint32_t res = raw_read();
        const uint32_t number = res & UINT32_MAX;
        return number;
      }

      // Accumulated charge upper 16-bits sign-extended to a 32-bit field (little-endian byte-by-byte). Upper bits of
      // signed 48-bit result, with the full 48-bit field having units of userA-seconds.
      // not supported
      /*
      uint16_t get_PassQMsb()
      {
        const uint32_t res = raw_read();
        const uint16_t number = (res >> 32) & UINT16_MAX;
        return number;
      }
      */

      // Accumulated Time (little-endian byte-by-byte), 32-bit unsigned integer in units of 250 ms.
      // not supported
      /*
      uint32_t get_PassTime()
      {
        const uint32_t res = raw_read();
        const uint32_t number = (res >> 48) & UINT32_MAX;
        return number;
      }
      */
    } passQ;

    // This command resets the accumulated charge and timer
    struct ResetPassQt : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_RESET_PASSQ_ADDR; }

      void send() { raw_send(); }
    } resetPassQ;

    // This command is sent to exit DEEPSLEEP mode.
    struct ExitDeepSleept : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_EXIT_DEEPSLEEP_ADDR; }

      void send() { raw_send(); }
    } exitDeepSleep;

    // This command is sent to enter DEEPSLEEP mode. Must be sent twice in a row within 4s to take effect
    struct DeepSleept : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_DEEPSLEEP_ADDR; }

      void send() { raw_send(); }
    } deepSleep;

    // This command is sent to start SHUTDOWN sequence. Must be sent twice in a row within 4s to take effect. If sent a
    // third time, the shutdown delay is skipped
    /* DO NOT USE, no way to wake up without disconecting the battery
    struct Shutdownt : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_SHUTDOWN_ADDR; }

      void send() { raw_send(); }
    } Shutdown;
    */

    // This command is sent to reset the device
    struct Resett : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_RESET_ADDR; }

      void send() { raw_send(); }
    } reset;

    // This command is sent to toggle the FET_EN bit in Battery Status(). FET_EN=0 means manual FET control. FET_EN=1
    // means autonomous device FET control
    struct FetEnablet : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_FET_ENABLE_ADDR; }

      void send() { raw_send(); }
    } fetEnable;

    // This command is sent to place the device in SEALED mode
    struct Sealt : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_SEAL_ADDR; }

      void send() { raw_send(); }
    } seal;

    // WRITE ONLY
    struct SecurityKeyst : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_SECURITY_KEYS_ADDR; }

      // Full Access Key Step 1:
      // This is the first word of the security key that must be sent to transition from SEALED to FULLACCESS mode.
      // Do not choose a word identical to a subcommand address.
      // Full Access Key Step 2:
      // This is the second word of the security key that must be sent to transition from SEALED to FULLACCESS mode.
      // Do not choose a word identical to a subcommand address or the same as the first word. It must be sent within 5
      // seconds of the first word of the key and with no other commands in between.
      void send(const uint16_t FaKey1, const uint16_t FaKey2)
      {
        uint8_t data[4];
        data[0] = FaKey1 | UINT8_MAX;
        data[1] = (FaKey1 >> 8) | UINT8_MAX;
        data[2] = FaKey2 | UINT8_MAX;
        data[3] = (FaKey2 >> 8) | UINT8_MAX;

        // TODO
        // raw_write(4, data);
      }
    } securityKeys;

    // Cell balancing active cells: When read, reports a bit mask of which cells are being actively balanced.
    // When written, starts balancing on the specified cells.
    // Write 0x00 to disable balancing
    struct CbActiveCellst : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_CB_ACTIVE_CELLS_ADDR; }

      // Cell balancing active cells: When read, reports a bit mask of which cells are being actively balanced. When
      // written, starts balancing on the specified cells.
      // Write 0x00 to turn balancing off.
      // Bit 7 is reserved, read/write 0 to this bit
      // Bit 6 is reserved, read/write 0 to this bit
      // Bit 5 corresponds to the fifth active cell
      // Bit 4 corresponds to the fourth active cell
      // Bit 3 corresponds to the third active cell
      // Bit 2 corresponds to the second active cell
      // Bit 1 corresponds to the first active cell (connected between VC1 and VC0)
      // Bit 0 is reserved, read/write 0 to this bit
      uint32_t get()
      {
        const uint32_t res = raw_read();
        return res;
        // TODO
        // const uint8_t number = res & UINT8_MAX;
        // return number;
      }

      void set_balancing(uint8_t cell1, uint8_t cell2, uint8_t cell3, uint8_t cell4, uint8_t cell5)
      {
        uint8_t data = (cell1 & 1) << 1 | (cell2 & 1) << 2 | (cell3 & 1) << 3 | (cell4 & 1) << 4 | (cell5 & 1) << 5;
        raw_write8(data);
      }
    } cbActiveCells;

    // This command is sent to place the device in CONFIG_UPDATE mode
    struct SetCfgUpdatet : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_SET_CFGUPDATE_ADDR; }

      void send() { raw_send(); }
    } setCfgUpdate;

    // This command is sent to exit CONFIG_UPDATE mode
    struct ExitCfgUpdatet : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_EXIT_CFGUPDATE_ADDR; }

      void send() { raw_send(); }
    } exitCfgUpdate;

    // Programmable timer, which allows the REGOUT LDO to be disabled and wakened after a programmed time or by alarm
    struct ProgTimert : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_PROG_TIMER_ADDR; }

      // Control to determine if REGOUT is wakened when an Alarm Status() bit asserts.
      // 0 = Do not re-enable the REGOUT LDO if any bit in Alarm Status() asserts while the timer is running (default).
      // 1 = If [REGOUT_SD]=1 and any bit in Alarm Status() asserts while the timer is running, reenable the REGOUT LDO
      // based on the setting of REGOUT Control()
      uint8_t get_REGOUT_ALARM_WK()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 11) & 0b1;
        return number;
      }

      // Delay before REGOUT is disabled when the timer is initiated while REGOUT is powered, and [REGOUT_SD]=1.
      // 0 = Zero delay (default).
      // 1 = 250ms delay.
      // 2 = 1-sec delay.
      // 3 = 4-sec delay
      uint8_t get_REGOUT_SD_DLY()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 9) & 0b11;
        return number;
      }

      // Control to determine if REGOUT is disabled when the command is sent.
      // 0 = do not disable the REGOUT LDO when command is sent (default).
      // 1 = disable the REGOUT LDO when the timer is initiated, after delay of
      // [REGOUT_SD_DLY]. When the timer expires, re-enable the REGOUT LDO based on the status of REGOUT Control().
      uint8_t get_REGOUT_SD()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 8) & 0b1;
        return number;
      }

      // Timer value programmable from 250 ms to 4 seconds in 250 ms increments (settings 1 to 16), and from 5 seconds
      // to 243 seconds in 1 second increments (settings 17 to 255).
      // A setting of zero disables the timer.
      // Whenever this field is written with a non-zero value, it initiates the timer
      uint8_t get_prog_timer()
      {
        const uint32_t res = raw_read();
        const uint8_t number = res & UINT8_MAX;
        return number;
      }

      void set_timer(uint8_t progTimer, uint8_t regoutSd, uint8_t regoutSdDelay, uint8_t regoutAlarmWake)
      {
        uint16_t data = progTimer & UINT8_MAX;
        data |= ((regoutSd & 0b1) | (regoutSdDelay & 0b11) << 1 | (regoutAlarmWake & 0b1) << 3) << 8;
        raw_write16(data);
      }
    } progTimer;

    // This command is sent to allow the device to enter SLEEP mode
    struct SleepEnablet : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_SLEEP_ENABLE_ADDR; }

      void send() { raw_send(); }
    } sleepEnable;

    // This command is sent to block the device from entering SLEEP mode
    struct SleepDisablet : public ISubcommandRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_SLEEP_DISABLE_ADDR; }

      void send() { raw_send(); }
    } sleepDisable;

    // This command enables the host to allow recovery of selected protection faults
    struct ProtRecoveryt : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_PROT_RECOVERY_ADDR; }

      // Cell Overvoltage or Cell Undervoltage fault recovery
      // 0 = Recovery of an COV/CUV fault is not triggered
      // 1 = Recovery of an COV/CUV fault is triggered.
      uint8_t get_volt_rec()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 6) & 0b1;
        return number;
      }

      // Recovery for a VSS or VREF fault from Safety Status B()
      // 0 = Recovery of a VSS or VREF fault is not triggered
      // 1 = Recovery of a VSS or VREF fault is triggered
      uint8_t get_diag_rec()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 6) & 0b1;
        return number;
      }

      // Short Circuit in Discharge fault recovery
      // 0 = Recovery of an SCD fault is not triggered
      // 1 = Recovery of an SCD fault is triggered
      uint8_t get_scd_rec()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 5) & 0b1;
        return number;
      }

      // Overcurrent in Discharge 1 fault recovery
      // 0 = Recovery of an OCD1 fault is not triggered
      // 1 = Recovery of an OCD1 fault is triggered
      uint8_t get_ocd1_rec()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 4) & 0b1;
        return number;
      }

      // Overcurrent in Discharge 2 fault recovery
      // 0 = Recovery of an OCD2 fault is not triggered
      // 1 = Recovery of an OCD2 fault is triggered.
      uint8_t get_ocd2_rec()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 3) & 0b1;
        return number;
      }

      // Overcurrent in Charge fault recovery
      // 0 = Recovery of an OCC fault is not triggered
      // 1 = Recovery of an OCC fault is triggered.
      uint8_t get_occ_rec()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 2) & 0b1;
        return number;
      }

      // Temperature fault recovery
      // 0 = Recovery of a temperature fault is not triggered
      // 1 = Recovery of a temperature fault is triggered.
      uint8_t get_temp_prec()
      {
        const uint32_t res = raw_read();
        const uint8_t number = (res >> 1) & 0b1;
        return number;
      }

      void set_recovery(uint8_t temperatureRecovery,
                        uint8_t overcurentChargeRecovery,
                        uint8_t overcurentDicharge2Recovery,
                        uint8_t overcurentDicharge1Recovery,
                        uint8_t shortCircuitDischargeRecovery,
                        uint8_t diagRecovery,
                        uint8_t cellVoltageRecovery)
      {
        uint8_t data = (temperatureRecovery & 0b1) << 1 | (overcurentChargeRecovery & 0b1) << 2 |
                       (overcurentDicharge2Recovery & 0b1) << 3 | (overcurentDicharge1Recovery & 0b1) << 4 |
                       (shortCircuitDischargeRecovery & 0b1) << 5 | (diagRecovery & 0b1) << 6 |
                       (cellVoltageRecovery & 0b1) << 7;
        raw_write8(data);
      }
    } protRecovery;

    struct SettingConfiguration_DAt : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_CONFIGURATION_DA_ADDR; }

      void set_disable_ts_reading()
      {
        // TODO handle other parameters
        uint16_t data = 0;
        data |= 1 << 8;
        raw_write16(data);
      }

      uint16_t get()
      {
        uint32_t data = raw_read();
        return data & UINT16_MAX;
      }
    } settingConfiguration_DA;

    // Not every system uses all of the cell input pins. If the system has fewer cells than the device supports, some VC
    // input pins must be shorted together. To prevent action being taken for cell undervoltage conditions on pins that
    // are shorted, set these bits appropriately.
    // 0 = All cell inputs are used for actual cells.
    // 1 = All cell inputs are used for actual cells.
    // 2 = Two actual cells are in use (VC5-VC4A, VC1-VC0), unused cell pins can be shorted to an adjacent cell pin at
    // the device or connected through RC to the cells.
    // 3 = Three actual cells are in use (VC5-VC4A, VC2-VC1, VC1-VC0), unused cell pins can be shorted to an adjacent
    // cell pin at the device or connected through RC to the cells.
    // 4 = Four actual cells are in use (VC5-VC4A, VC4B-VC3A, VC2-VC1, VC1-VC0), unused cell pins can be shorted to an
    // adjacent cell pin at the device or connected through RC to the cells.
    // 5 = All cell inputs are used for actual cells.
    struct ConfigurateVcellt : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_CONFIGURATION_VCELL_MODE_ADDR; }

      uint8_t get()
      {
        const uint32_t res = raw_read();
        const uint8_t number = res & 0b1111;
        return number;
      }

      void set(uint8_t cellNumber)
      {
        if (cellNumber > 5)
          return;

        raw_write8(cellNumber);
      }
    } configurateVcell;

    struct TsOffsett : public ISubcommandReadRegister
    {
      uint16_t command_address() const override { return SUBCOMMAND_CONFIGURATION_TS_OFFSET_ADDR; }

      void set(uint16_t offset) { raw_write16(offset); }
    } tsOffset;
  };

private:
  static bool readDataReg(const byte regAddress, byte* dataVal, const uint8_t arrLen)
  {
    return i2c_readData(i2cObjectIndex, BQ76905addr, regAddress, arrLen, dataVal, usesStopBit ? 1 : 0) == 0;
  }

  static bool writeData16(const byte regAddress, uint16_t data)
  {
    return i2c_write16(i2cObjectIndex, BQ76905addr, regAddress, data, usesStopBit ? 1 : 0) == 0;
  }

  static bool writeData(const byte regAddress, uint8_t lenght, uint8_t* data)
  {
    return i2c_writeData(i2cObjectIndex, BQ76905addr, regAddress, lenght, data, usesStopBit ? 1 : 0) == 0;
  }
};

} // namespace bq76905

#endif