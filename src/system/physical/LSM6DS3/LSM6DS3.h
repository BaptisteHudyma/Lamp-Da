/******************************************************************************
    SparkFunLSM6DS3.h
    LSM6DS3 Arduino and Teensy Driver

    Marshall Taylor @ SparkFun Electronics
    May 20, 2015
    https://github.com/sparkfun/LSM6DS3_Breakout
    https://github.com/sparkfun/SparkFun_LSM6DS3_Arduino_Library

    Development environment specifics:
    Arduino IDE 1.6.4
    Teensy loader 1.23

    This code is released under the [MIT
License](http://opensource.org/licenses/MIT).

    Please review the LICENSE.md file included with this example. If you have
any questions or concerns with licensing, please contact
techsupport@sparkfun.com.

    Distributed as-is; no warranty is given.
******************************************************************************/

/**
 * Corrected by Baptiste Hudyma
 * updated to datasheet "DocID026899 Rev 10 "
 * 19/04/2025
 */

#ifndef __LSM6DS3IMU_H__
#define __LSM6DS3IMU_H__

#include "stdint.h"

#define I2C_MODE 0
#define SPI_MODE 1

// Return values
typedef enum
{
  IMU_SUCCESS,
  IMU_HW_ERROR,
  IMU_NOT_SUPPORTED,
  IMU_GENERIC_ERROR,
  IMU_OUT_OF_BOUNDS,
  IMU_ALL_ONES_WARNING,
  //...
} status_t;

// This is the core operational class of the driver.
//   LSM6DS3Core contains only read and write operations towards the IMU.
//   To use the higher level functions, use the class LSM6DS3 which inherits
//   this class.

class LSM6DS3Core
{
public:
  LSM6DS3Core(uint8_t);
  LSM6DS3Core(uint8_t, uint8_t);
  ~LSM6DS3Core() = default;

  status_t beginCore(void);

  // The following utilities read and write to the IMU

  // ReadRegisterRegion takes a uint8 array address as input and reads
  //   a chunk of memory into that array.
  status_t readRegisterRegion(uint8_t*, uint8_t, uint8_t);

  // readRegister reads one 8-bit register
  status_t readRegister(uint8_t*, uint8_t);

  // Reads two 8-bit regs, LSByte then MSByte order, and concatenates them.
  //   Acts as a 16-bit read operation
  status_t readRegisterInt16(int16_t*, uint8_t offset);

  // Writes an 8-bit byte;
  status_t writeRegister(uint8_t, uint8_t);

  // Change to embedded page
  status_t embeddedPage(void);

  // Change to base page
  status_t basePage(void);

private:
  // Communication stuff
  uint8_t I2CAddress;
};

// This struct holds the settings the driver uses to do calculations
struct SensorSettings
{
public:
  // Gyro settings
  uint8_t gyroEnabled;
  uint16_t gyroRange;
  uint16_t gyroSampleRate;
  uint16_t gyroBandWidth;

  uint8_t gyroFifoEnabled;
  uint8_t gyroFifoDecimation;

  // Accelerometer settings
  uint8_t accelEnabled;
  uint8_t accelODROff;
  uint16_t accelRange;
  uint16_t accelSampleRate;
  uint16_t accelBandWidth;

  uint8_t accelFifoEnabled;
  uint8_t accelFifoDecimation;

  // Temperature settings
  uint8_t tempEnabled;

  // Non-basic mode settings
  uint8_t commMode;

  // FIFO control data
  uint16_t fifoThreshold;
  int16_t fifoSampleRate;
  uint8_t fifoModeWord;

  uint16_t tempSensitivity;
};

// This is the highest level class of the driver.
//
//   class LSM6DS3 inherits the core and makes use of the beginCore()
// method through it's own begin() method.  It also contains the
// settings struct to hold user settings.

class LSM6DS3 : public LSM6DS3Core
{
public:
  // IMU settings
  SensorSettings settings;

  // Error checking
  uint16_t allOnesCounter;
  uint16_t nonSuccessCounter;

  // Constructor generates default SensorSettings.
  //(over-ride after construction if desired)
  LSM6DS3(uint8_t busType = I2C_MODE, uint8_t inputArg = 0x6A);
  ~LSM6DS3() = default;

  // Call to apply SensorSettings
  status_t begin(void);

  // Returns the raw bits from the sensor cast as 16-bit signed integers
  int16_t readRawAccelX(void);
  int16_t readRawAccelY(void);
  int16_t readRawAccelZ(void);
  int16_t readRawGyroX(void);
  int16_t readRawGyroY(void);
  int16_t readRawGyroZ(void);

  // Returns the values as floats.  Inside, this calls readRaw___();
  float readFloatAccelX(void);
  float readFloatAccelY(void);
  float readFloatAccelZ(void);
  float readFloatGyroX(void);
  float readFloatGyroY(void);
  float readFloatGyroZ(void);

  // Temperature related methods
  int16_t readRawTemp(void);
  float readTempC(void);
  float readTempF(void);

  // FIFO stuff
  void fifoBegin(void);
  void fifoClear(void);
  int16_t fifoRead(void);
  uint16_t fifoGetStatus(void);
  void fifoEnd(void);

  enum InterruptType
  {
    None,        // no interrupt
    Fall,        // raised during a free fall event
    BigMotion,   // raised with a >6g acceleration
    Step,        // raised on a step event
    AngleChange, // raised on portrait to landscape (or inverse) rotation
  };
  bool enable_interrupt1(const InterruptType interr);

  float calcGyro(int16_t);
  float calcAccel(int16_t);

protected:
  bool enable_free_fall_detection();
  bool enable_big_motion_detection();

private:
};

/****************** Device ID *********************/
#define LSM6DS3_ACC_GYRO_WHO_AM_I   0X69
#define LSM6DS3_C_ACC_GYRO_WHO_AM_I 0x6A

/************** Device Register  *******************/
#define LSM6DS3_ACC_GYRO_TEST_PAGE        0X00
#define LSM6DS3_ACC_GYRO_RAM_ACCESS       0X01
#define LSM6DS3_ACC_GYRO_SENSOR_SYNC_TIME 0X04
#define LSM6DS3_ACC_GYRO_SENSOR_SYNC_EN   0X05
#define LSM6DS3_ACC_GYRO_FIFO_CTRL1       0X06
#define LSM6DS3_ACC_GYRO_FIFO_CTRL2       0X07
#define LSM6DS3_ACC_GYRO_FIFO_CTRL3       0X08
#define LSM6DS3_ACC_GYRO_FIFO_CTRL4       0X09
#define LSM6DS3_ACC_GYRO_FIFO_CTRL5       0X0A
#define LSM6DS3_ACC_GYRO_ORIENT_CFG_G     0X0B
#define LSM6DS3_ACC_GYRO_REFERENCE_G      0X0C
// Each bit in this register enables a signal to be carried through INT1. The pad’s output will supply the OR
// combination of the selected signals.
#define LSM6DS3_ACC_GYRO_INT1_CTRL 0X0D
// Each bit in this register enables a signal to be carried through INT2. The pad’s output will supply the OR
// combination of the selected signals.
#define LSM6DS3_ACC_GYRO_INT2_CTRL       0X0E
#define LSM6DS3_ACC_GYRO_WHO_AM_I_REG    0X0F
#define LSM6DS3_ACC_GYRO_CTRL1_XL        0X10
#define LSM6DS3_ACC_GYRO_CTRL2_G         0X11
#define LSM6DS3_ACC_GYRO_CTRL3_C         0X12
#define LSM6DS3_ACC_GYRO_CTRL4_C         0X13
#define LSM6DS3_ACC_GYRO_CTRL5_C         0X14
#define LSM6DS3_ACC_GYRO_CTRL6_C         0X15
#define LSM6DS3_ACC_GYRO_CTRL7_G         0X16
#define LSM6DS3_ACC_GYRO_CTRL8_XL        0X17
#define LSM6DS3_ACC_GYRO_CTRL9_XL        0X18
#define LSM6DS3_ACC_GYRO_CTRL10_C        0X19
#define LSM6DS3_ACC_GYRO_MASTER_CONFIG   0X1A
#define LSM6DS3_ACC_GYRO_WAKE_UP_SRC     0X1B
#define LSM6DS3_ACC_GYRO_TAP_SRC         0X1C
#define LSM6DS3_ACC_GYRO_D6D_SRC         0X1D
#define LSM6DS3_ACC_GYRO_STATUS_REG      0X1E
#define LSM6DS3_ACC_GYRO_OUT_TEMP_L      0X20
#define LSM6DS3_ACC_GYRO_OUT_TEMP_H      0X21
#define LSM6DS3_ACC_GYRO_OUTX_L_G        0X22
#define LSM6DS3_ACC_GYRO_OUTX_H_G        0X23
#define LSM6DS3_ACC_GYRO_OUTY_L_G        0X24
#define LSM6DS3_ACC_GYRO_OUTY_H_G        0X25
#define LSM6DS3_ACC_GYRO_OUTZ_L_G        0X26
#define LSM6DS3_ACC_GYRO_OUTZ_H_G        0X27
#define LSM6DS3_ACC_GYRO_OUTX_L_XL       0X28
#define LSM6DS3_ACC_GYRO_OUTX_H_XL       0X29
#define LSM6DS3_ACC_GYRO_OUTY_L_XL       0X2A
#define LSM6DS3_ACC_GYRO_OUTY_H_XL       0X2B
#define LSM6DS3_ACC_GYRO_OUTZ_L_XL       0X2C
#define LSM6DS3_ACC_GYRO_OUTZ_H_XL       0X2D
#define LSM6DS3_ACC_GYRO_SENSORHUB1_REG  0X2E
#define LSM6DS3_ACC_GYRO_SENSORHUB2_REG  0X2F
#define LSM6DS3_ACC_GYRO_SENSORHUB3_REG  0X30
#define LSM6DS3_ACC_GYRO_SENSORHUB4_REG  0X31
#define LSM6DS3_ACC_GYRO_SENSORHUB5_REG  0X32
#define LSM6DS3_ACC_GYRO_SENSORHUB6_REG  0X33
#define LSM6DS3_ACC_GYRO_SENSORHUB7_REG  0X34
#define LSM6DS3_ACC_GYRO_SENSORHUB8_REG  0X35
#define LSM6DS3_ACC_GYRO_SENSORHUB9_REG  0X36
#define LSM6DS3_ACC_GYRO_SENSORHUB10_REG 0X37
#define LSM6DS3_ACC_GYRO_SENSORHUB11_REG 0X38
#define LSM6DS3_ACC_GYRO_SENSORHUB12_REG 0X39
#define LSM6DS3_ACC_GYRO_FIFO_STATUS1    0X3A
#define LSM6DS3_ACC_GYRO_FIFO_STATUS2    0X3B
#define LSM6DS3_ACC_GYRO_FIFO_STATUS3    0X3C
#define LSM6DS3_ACC_GYRO_FIFO_STATUS4    0X3D
#define LSM6DS3_ACC_GYRO_FIFO_DATA_OUT_L 0X3E
#define LSM6DS3_ACC_GYRO_FIFO_DATA_OUT_H 0X3F
#define LSM6DS3_ACC_GYRO_TIMESTAMP0_REG  0X40
#define LSM6DS3_ACC_GYRO_TIMESTAMP1_REG  0X41
#define LSM6DS3_ACC_GYRO_TIMESTAMP2_REG  0X42
#define LSM6DS3_ACC_GYRO_STEP_COUNTER_L  0X4B
#define LSM6DS3_ACC_GYRO_STEP_COUNTER_H  0X4C
// Significant motion, tilt, step detector, hard/soft-iron and sensor hub interrupt source register (r).
#define LSM6DS3_ACC_GYRO_FUNC_SRC 0X53
// Timestamp, pedometer, tilt, filtering, and tap recognition functions configuration register (r/w).
#define LSM6DS3_ACC_GYRO_TAP_CFG 0X58
// Portrait/landscape position and tap function threshold register (r/w).
#define LSM6DS3_ACC_GYRO_TAP_THS_6D 0X59
// Tap recognition function setting register (r/w).
#define LSM6DS3_ACC_GYRO_INT_DUR2 0X5A
// Single and double-tap function threshold register (r/w)
#define LSM6DS3_ACC_GYRO_WAKE_UP_THS 0X5B
// Free-fall, wakeup, timestamp and sleep mode functions duration setting register (r/w).
#define LSM6DS3_ACC_GYRO_WAKE_UP_DUR 0X5C
// Free-fall function duration setting register (r/w).
#define LSM6DS3_ACC_GYRO_FREE_FALL 0X5D
// Functions routing on INT1 register (r/w)
#define LSM6DS3_ACC_GYRO_MD1_CFG 0X5E
// Functions routing on INT2 register (r/w).
#define LSM6DS3_ACC_GYRO_MD2_CFG 0X5F

/************** Access Device RAM  *******************/
#define LSM6DS3_ACC_GYRO_ADDR0_TO_RW_RAM  0x62
#define LSM6DS3_ACC_GYRO_ADDR1_TO_RW_RAM  0x63
#define LSM6DS3_ACC_GYRO_DATA_TO_WR_RAM   0x64
#define LSM6DS3_ACC_GYRO_DATA_RD_FROM_RAM 0x65

#define LSM6DS3_ACC_GYRO_RAM_SIZE 4096

/************** Embedded functions register mapping  *******************/
#define LSM6DS3_ACC_GYRO_SLV0_ADD                    0x02
#define LSM6DS3_ACC_GYRO_SLV0_SUBADD                 0x03
#define LSM6DS3_ACC_GYRO_SLAVE0_CONFIG               0x04
#define LSM6DS3_ACC_GYRO_SLV1_ADD                    0x05
#define LSM6DS3_ACC_GYRO_SLV1_SUBADD                 0x06
#define LSM6DS3_ACC_GYRO_SLAVE1_CONFIG               0x07
#define LSM6DS3_ACC_GYRO_SLV2_ADD                    0x08
#define LSM6DS3_ACC_GYRO_SLV2_SUBADD                 0x09
#define LSM6DS3_ACC_GYRO_SLAVE2_CONFIG               0x0A
#define LSM6DS3_ACC_GYRO_SLV3_ADD                    0x0B
#define LSM6DS3_ACC_GYRO_SLV3_SUBADD                 0x0C
#define LSM6DS3_ACC_GYRO_SLAVE3_CONFIG               0x0D
#define LSM6DS3_ACC_GYRO_DATAWRITE_SRC_MODE_SUB_SLV0 0x0E
#define LSM6DS3_ACC_GYRO_CONFIG_PEDO_THS_MIN         0x0F
#define LSM6DS3_ACC_GYRO_CONFIG_TILT_IIR             0x10
#define LSM6DS3_ACC_GYRO_CONFIG_TILT_ACOS            0x11
#define LSM6DS3_ACC_GYRO_CONFIG_TILT_WTIME           0x12
#define LSM6DS3_ACC_GYRO_SM_STEP_THS                 0x13
#define LSM6DS3_ACC_GYRO_MAG_SI_XX                   0x24
#define LSM6DS3_ACC_GYRO_MAG_SI_XY                   0x25
#define LSM6DS3_ACC_GYRO_MAG_SI_XZ                   0x26
#define LSM6DS3_ACC_GYRO_MAG_SI_YX                   0x27
#define LSM6DS3_ACC_GYRO_MAG_SI_YY                   0x28
#define LSM6DS3_ACC_GYRO_MAG_SI_YZ                   0x29
#define LSM6DS3_ACC_GYRO_MAG_SI_ZX                   0x2A
#define LSM6DS3_ACC_GYRO_MAG_SI_ZY                   0x2B
#define LSM6DS3_ACC_GYRO_MAG_SI_ZZ                   0x2C
#define LSM6DS3_ACC_GYRO_MAG_OFFX_L                  0x2D
#define LSM6DS3_ACC_GYRO_MAG_OFFX_H                  0x2E
#define LSM6DS3_ACC_GYRO_MAG_OFFY_L                  0x2F
#define LSM6DS3_ACC_GYRO_MAG_OFFY_H                  0x30
#define LSM6DS3_ACC_GYRO_MAG_OFFZ_L                  0x31
#define LSM6DS3_ACC_GYRO_MAG_OFFZ_H                  0x32

/*******************************************************************************
    Register      : TEST_PAGE
    Address       : 0X00
    Bit Group Name: FLASH_PAGE
    Permission    : RW
*******************************************************************************/
#define FLASH_PAGE 0x40

/*******************************************************************************
    Register      : RAM_ACCESS
    Address       : 0X01
    Bit Group Name: PROG_RAM1
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_PROG_RAM1_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_PROG_RAM1_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_PROG_RAM1_t;

/*******************************************************************************
    Register      : RAM_ACCESS
    Address       : 0X01
    Bit Group Name: CUSTOMROM1
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_CUSTOMROM1_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_CUSTOMROM1_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_CUSTOMROM1_t;

/*******************************************************************************
    Register      : RAM_ACCESS
    Address       : 0X01
    Bit Group Name: RAM_PAGE
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_RAM_PAGE_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_RAM_PAGE_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_RAM_PAGE_t;

/*******************************************************************************
    Register      : SENSOR_SYNC_TIME
    Address       : 0X04
    Bit Group Name: TPH
    Permission    : RW
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_TPH_MASK     0xFF
#define LSM6DS3_ACC_GYRO_TPH_POSITION 0

/*******************************************************************************
    Register      : SENSOR_SYNC_EN
    Address       : 0X05
    Bit Group Name: SYNC_EN
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SYNC_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_SYNC_EN_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_SYNC_EN_t;

/*******************************************************************************
    Register      : SENSOR_SYNC_EN
    Address       : 0X05
    Bit Group Name: HP_RST
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_HP_RST_RST_OFF = 0x00,
  LSM6DS3_ACC_GYRO_HP_RST_RST_ON = 0x02,
} LSM6DS3_ACC_GYRO_HP_RST_t;

/*******************************************************************************
    Register      : FIFO_CTRL1
    Address       : 0X06
    Bit Group Name: WTM_FIFO
    Permission    : RW
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_WTM_FIFO_CTRL1_MASK     0xFF
#define LSM6DS3_ACC_GYRO_WTM_FIFO_CTRL1_POSITION 0
#define LSM6DS3_ACC_GYRO_WTM_FIFO_CTRL2_MASK     0x0F
#define LSM6DS3_ACC_GYRO_WTM_FIFO_CTRL2_POSITION 0

/*******************************************************************************
    Register      : FIFO_CTRL2
    Address       : 0X07
    Bit Group Name: TIM_PEDO_FIFO_DRDY
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TIM_PEDO_FIFO_DRDY_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TIM_PEDO_FIFO_DRDY_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_TIM_PEDO_FIFO_DRDY_t;

/*******************************************************************************
    Register      : FIFO_CTRL2
    Address       : 0X07
    Bit Group Name: TIM_PEDO_FIFO_EN
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TIM_PEDO_FIFO_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TIM_PEDO_FIFO_EN_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_TIM_PEDO_FIFO_EN_t;

/*******************************************************************************
    Register      : FIFO_CTRL3
    Address       : 0X08
    Bit Group Name: DEC_FIFO_XL
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_DATA_NOT_IN_FIFO = 0x00,
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_NO_DECIMATION = 0x01,
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_DECIMATION_BY_2 = 0x02,
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_DECIMATION_BY_3 = 0x03,
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_DECIMATION_BY_4 = 0x04,
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_DECIMATION_BY_8 = 0x05,
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_DECIMATION_BY_16 = 0x06,
  LSM6DS3_ACC_GYRO_DEC_FIFO_XL_DECIMATION_BY_32 = 0x07,
} LSM6DS3_ACC_GYRO_DEC_FIFO_XL_t;

/*******************************************************************************
    Register      : FIFO_CTRL3
    Address       : 0X08
    Bit Group Name: DEC_FIFO_G
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_DATA_NOT_IN_FIFO = 0x00,
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_NO_DECIMATION = 0x08,
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_DECIMATION_BY_2 = 0x10,
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_DECIMATION_BY_3 = 0x18,
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_DECIMATION_BY_4 = 0x20,
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_DECIMATION_BY_8 = 0x28,
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_DECIMATION_BY_16 = 0x30,
  LSM6DS3_ACC_GYRO_DEC_FIFO_G_DECIMATION_BY_32 = 0x38,
} LSM6DS3_ACC_GYRO_DEC_FIFO_G_t;

/*******************************************************************************
    Register      : FIFO_CTRL4
    Address       : 0X09
    Bit Group Name: DEC_FIFO_SLV0
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_DATA_NOT_IN_FIFO = 0x00,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_NO_DECIMATION = 0x01,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_DECIMATION_BY_2 = 0x02,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_DECIMATION_BY_3 = 0x03,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_DECIMATION_BY_4 = 0x04,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_DECIMATION_BY_8 = 0x05,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_DECIMATION_BY_16 = 0x06,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_DECIMATION_BY_32 = 0x07,
} LSM6DS3_ACC_GYRO_DEC_FIFO_SLV0_t;

/*******************************************************************************
    Register      : FIFO_CTRL4
    Address       : 0X09
    Bit Group Name: DEC_FIFO_SLV1
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_DATA_NOT_IN_FIFO = 0x00,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_NO_DECIMATION = 0x08,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_DECIMATION_BY_2 = 0x10,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_DECIMATION_BY_3 = 0x18,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_DECIMATION_BY_4 = 0x20,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_DECIMATION_BY_8 = 0x28,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_DECIMATION_BY_16 = 0x30,
  LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_DECIMATION_BY_32 = 0x38,
} LSM6DS3_ACC_GYRO_DEC_FIFO_SLV1_t;

/*******************************************************************************
    Register      : FIFO_CTRL4
    Address       : 0X09
    Bit Group Name: HI_DATA_ONLY
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_HI_DATA_ONLY_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_HI_DATA_ONLY_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_HI_DATA_ONLY_t;

/*******************************************************************************
    Register      : FIFO_CTRL5
    Address       : 0X0A
    Bit Group Name: FIFO_MODE
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FIFO_MODE_BYPASS = 0x00,
  LSM6DS3_ACC_GYRO_FIFO_MODE_FIFO = 0x01,
  LSM6DS3_ACC_GYRO_FIFO_MODE_STF = 0x03,
  LSM6DS3_ACC_GYRO_FIFO_MODE_BTS = 0x04,
  LSM6DS3_ACC_GYRO_FIFO_MODE_DYN_STREAM = 0x06,
} LSM6DS3_ACC_GYRO_FIFO_MODE_t;

/*******************************************************************************
    Register      : FIFO_CTRL5
    Address       : 0X0A
    Bit Group Name: ODR_FIFO
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ODR_FIFO_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_ODR_FIFO_10Hz = 0x08,
  LSM6DS3_ACC_GYRO_ODR_FIFO_25Hz = 0x10,
  LSM6DS3_ACC_GYRO_ODR_FIFO_50Hz = 0x18,
  LSM6DS3_ACC_GYRO_ODR_FIFO_100Hz = 0x20,
  LSM6DS3_ACC_GYRO_ODR_FIFO_200Hz = 0x28,
  LSM6DS3_ACC_GYRO_ODR_FIFO_400Hz = 0x30,
  LSM6DS3_ACC_GYRO_ODR_FIFO_800Hz = 0x38,
  LSM6DS3_ACC_GYRO_ODR_FIFO_1600Hz = 0x40,
  LSM6DS3_ACC_GYRO_ODR_FIFO_3300Hz = 0x48,
  LSM6DS3_ACC_GYRO_ODR_FIFO_6600Hz = 0x50,
} LSM6DS3_ACC_GYRO_ODR_FIFO_t;

/*******************************************************************************
    Register      : ORIENT_CFG_G
    Address       : 0X0B
    Bit Group Name: ORIENT
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ORIENT_XYZ = 0x00,
  LSM6DS3_ACC_GYRO_ORIENT_XZY = 0x01,
  LSM6DS3_ACC_GYRO_ORIENT_YXZ = 0x02,
  LSM6DS3_ACC_GYRO_ORIENT_YZX = 0x03,
  LSM6DS3_ACC_GYRO_ORIENT_ZXY = 0x04,
  LSM6DS3_ACC_GYRO_ORIENT_ZYX = 0x05,
} LSM6DS3_ACC_GYRO_ORIENT_t;

/*******************************************************************************
    Register      : ORIENT_CFG_G
    Address       : 0X0B
    Bit Group Name: SIGN_Z_G
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SIGN_Z_G_POSITIVE = 0x00,
  LSM6DS3_ACC_GYRO_SIGN_Z_G_NEGATIVE = 0x08,
} LSM6DS3_ACC_GYRO_SIGN_Z_G_t;

/*******************************************************************************
    Register      : ORIENT_CFG_G
    Address       : 0X0B
    Bit Group Name: SIGN_Y_G
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SIGN_Y_G_POSITIVE = 0x00,
  LSM6DS3_ACC_GYRO_SIGN_Y_G_NEGATIVE = 0x10,
} LSM6DS3_ACC_GYRO_SIGN_Y_G_t;

/*******************************************************************************
    Register      : ORIENT_CFG_G
    Address       : 0X0B
    Bit Group Name: SIGN_X_G
    Permission    : RW
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SIGN_X_G_POSITIVE = 0x00,
  LSM6DS3_ACC_GYRO_SIGN_X_G_NEGATIVE = 0x20,
} LSM6DS3_ACC_GYRO_SIGN_X_G_t;

/*******************************************************************************
    Register      : REFERENCE_G
    Address       : 0X0C
    Bit Group Name: REF_G
    Permission    : RW
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_REF_G_MASK     0xFF
#define LSM6DS3_ACC_GYRO_REF_G_POSITION 0

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_DRDY_XL
    Permission    : RW
    Comment       : Accelerometer Data Ready on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_DRDY_XL_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_DRDY_XL_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_INT1_DRDY_XL_t;

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_DRDY_G
    Permission    : RW
    Comment       : Gyroscope Data Ready on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_DRDY_G_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_DRDY_G_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_INT1_DRDY_G_t;

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_BOOT
    Permission    : RW
    Comment       : Boot status available on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_BOOT_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_BOOT_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_INT1_BOOT_t;

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_FTH
    Permission    : RW
    Comment       : FIFO threshold interrupt on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_FTH_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_FTH_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_INT1_FTH_t;

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_OVR
    Permission    : RW
    Comment       : FIFO overrun interrupt on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_OVR_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_OVR_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_INT1_OVR_t;

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_FSS5
    Permission    : RW
    Comment       : FIFO full flag interrupt enable on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_FULL_FLAG_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_FULL_FLAG_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_INT1_FULL_FLAG_t;

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_SIGN_MOT
    Permission    : RW
    Comment       : Significant motion interrupt enable on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_SIGN_MOT_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_SIGN_MOT_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_INT1_SIGN_MOT_t;

/*******************************************************************************
    Register      : INT1_CTRL
    Address       : 0X0D
    Bit Group Name: INT1_PEDO
    Permission    : RW
    Comment       : Pedometer step recognition interrupt enable on INT1 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_PEDO_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_PEDO_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_INT1_PEDO_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_DRDY_XL
    Permission    : RW
    Comment       : Accelerometer Data Ready on INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_DRDY_XL_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_DRDY_XL_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_INT2_DRDY_XL_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_DRDY_G
    Permission    : RW
    Comment       : Gyroscope Data Ready on INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_DRDY_G_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_DRDY_G_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_INT2_DRDY_G_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_DRDY_TEMP
    Permission    : RW
    Comment       : Temperature Data Ready in INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_DRDY_TEMP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_DRDY_TEMP_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_INT2_DRDY_TEMP_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_FTH
    Permission    : RW
    Comment       : FIFO threshold interrupt on INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_FTH_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_FTH_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_INT2_FTH_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_OVR
    Permission    : RW
    Comment       : FIFO overrun interrupt on INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_OVR_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_OVR_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_INT2_OVR_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_FSS5
    Permission    : RW
    Comment       : FIFO full flag interrupt enable on INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_FULL_FLAG_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_FULL_FLAG_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_INT2_FULL_FLAG_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_SIGN_MOT
    Permission    : RW
    Comment       : Step counter overflow interrupt enable on INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_STEP_COUNT_OV_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_STEP_COUNT_OV_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_INT2_STEP_COUNT_OV_t;

/*******************************************************************************
    Register      : INT2_CTRL
    Address       : 0X0E
    Bit Group Name: INT2_PEDO
    Permission    : RW
    Comment       : Pedometer step recognition interrupt on delta time(1) enable on INT2 pad. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_STEP_DELTA_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_STEP_DELTA_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_INT2_STEP_DELTA_t;

/*******************************************************************************
    Register      : WHO_AM_I
    Address       : 0X0F
    Bit Group Name: WHO_AM_I_BIT
    Permission    : RO
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_WHO_AM_I_BIT_MASK     0xFF
#define LSM6DS3_ACC_GYRO_WHO_AM_I_BIT_POSITION 0

/*******************************************************************************
    Register      : CTRL1_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL1_XL
    Bit Group Name: BW_XL
    Permission    : RW
    Comment       : Anti-aliasing filter bandwidth selection. Default value: 00.
    00: 400 Hz; 01: 200 Hz; 10: 100 Hz; 11: 50 Hz
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_BW_XL_400Hz = 0x00,
  LSM6DS3_ACC_GYRO_BW_XL_200Hz = 0x01,
  LSM6DS3_ACC_GYRO_BW_XL_100Hz = 0x02,
  LSM6DS3_ACC_GYRO_BW_XL_50Hz = 0x03,
} LSM6DS3_ACC_GYRO_BW_XL_t;

/*******************************************************************************
    Register      : CTRL1_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL1_XL
    Bit Group Name: FS_XL
    Permission    : RW
    Comment       : Accelerometer full-scale selection. Default value: 00.
    (00: ±2 g; 01: ±16 g; 10: ±4 g; 11: ±8 g)
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FS_XL_2g = 0x00,
  LSM6DS3_ACC_GYRO_FS_XL_16g = 0x04,
  LSM6DS3_ACC_GYRO_FS_XL_4g = 0x08,
  LSM6DS3_ACC_GYRO_FS_XL_8g = 0x0C,
} LSM6DS3_ACC_GYRO_FS_XL_t;

/*******************************************************************************
    Register      : CTRL1_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL1_XL
    Bit Group Name: ODR_XL
    Permission    : RW
    Comment       : Output data rate and power mode selection. Default value: 0000
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ODR_XL_POWER_DOWN = 0x00,
  LSM6DS3_ACC_GYRO_ODR_XL_13Hz = 0x10,
  LSM6DS3_ACC_GYRO_ODR_XL_26Hz = 0x20,
  LSM6DS3_ACC_GYRO_ODR_XL_52Hz = 0x30,
  LSM6DS3_ACC_GYRO_ODR_XL_104Hz = 0x40,
  LSM6DS3_ACC_GYRO_ODR_XL_208Hz = 0x50,
  LSM6DS3_ACC_GYRO_ODR_XL_416Hz = 0x60,
  LSM6DS3_ACC_GYRO_ODR_XL_833Hz = 0x70,
  LSM6DS3_ACC_GYRO_ODR_XL_1660Hz = 0x80,
  LSM6DS3_ACC_GYRO_ODR_XL_3330Hz = 0x90,
  LSM6DS3_ACC_GYRO_ODR_XL_6660Hz = 0xA0,
} LSM6DS3_ACC_GYRO_ODR_XL_t;

/*******************************************************************************
    Register      : CTRL2_G
    Address       : 0X11
    Bit Group Name: FS_125
    Permission    : RW
    Comment       : Gyroscope full-scale at 125 dps. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FS_125_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_FS_125_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_FS_125_t;

/*******************************************************************************
    Register      : CTRL2_G
    Address       : 0X11
    Bit Group Name: FS_G
    Permission    : RW
    Comment       : Gyroscope full-scale selection. Default value: 00
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FS_G_245dps = 0x00,
  LSM6DS3_ACC_GYRO_FS_G_500dps = 0x04,
  LSM6DS3_ACC_GYRO_FS_G_1000dps = 0x08,
  LSM6DS3_ACC_GYRO_FS_G_2000dps = 0x0C,
} LSM6DS3_ACC_GYRO_FS_G_t;

/*******************************************************************************
    Register      : CTRL2_G
    Address       : 0X11
    Bit Group Name: ODR_G
    Permission    : RW
    Comment       : Gyroscope output data rate selection. Default value: 0000
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ODR_G_POWER_DOWN = 0x00,
  LSM6DS3_ACC_GYRO_ODR_G_13Hz = 0x10,
  LSM6DS3_ACC_GYRO_ODR_G_26Hz = 0x20,
  LSM6DS3_ACC_GYRO_ODR_G_52Hz = 0x30,
  LSM6DS3_ACC_GYRO_ODR_G_104Hz = 0x40,
  LSM6DS3_ACC_GYRO_ODR_G_208Hz = 0x50,
  LSM6DS3_ACC_GYRO_ODR_G_416Hz = 0x60,
  LSM6DS3_ACC_GYRO_ODR_G_833Hz = 0x70,
  LSM6DS3_ACC_GYRO_ODR_G_1660Hz = 0x80,
} LSM6DS3_ACC_GYRO_ODR_G_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: SW_RESET
    Permission    : RW
    Comment       : Software reset. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SW_RESET_NORMAL_MODE = 0x00,
  LSM6DS3_ACC_GYRO_SW_RESET_RESET_DEVICE = 0x01,
} LSM6DS3_ACC_GYRO_SW_RESET_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: BLE
    Permission    : RW
    Comment       : Big/Little Endian Data selection. Default value 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_BLE_LSB = 0x00,
  LSM6DS3_ACC_GYRO_BLE_MSB = 0x02,
} LSM6DS3_ACC_GYRO_BLE_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: IF_INC
    Permission    : RW
    Comment       : Register address automatically incremented during a multiple byte access with a serial interface
(I2C or SPI). Default value: 1
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_IF_INC_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_IF_INC_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_IF_INC_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: SIM
    Permission    : RW
    Comment       : SPI Serial Interface Mode selection. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SIM_4_WIRE = 0x00,
  LSM6DS3_ACC_GYRO_SIM_3_WIRE = 0x08,
} LSM6DS3_ACC_GYRO_SIM_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: PP_OD
    Permission    : RW
    Comment       : Push-pull/open-drain selection on INT1 and INT2 pads. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_PP_OD_PUSH_PULL = 0x00,
  LSM6DS3_ACC_GYRO_PP_OD_OPEN_DRAIN = 0x10,
} LSM6DS3_ACC_GYRO_PP_OD_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: INT_ACT_LEVEL
    Permission    : RW
    Comment       : Interrupt activation level. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT_ACT_LEVEL_ACTIVE_HI = 0x00,
  LSM6DS3_ACC_GYRO_INT_ACT_LEVEL_ACTIVE_LO = 0x20,
} LSM6DS3_ACC_GYRO_INT_ACT_LEVEL_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: BDU
    Permission    : RW
    Comment       : Block Data Update. Default value:
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_BDU_CONTINUOS = 0x00,
  LSM6DS3_ACC_GYRO_BDU_BLOCK_UPDATE = 0x40,
} LSM6DS3_ACC_GYRO_BDU_t;

/*******************************************************************************
    Register      : CTRL3_C
    Address       : LSM6DS3_ACC_GYRO_CTRL3_C
    Bit Group Name: BOOT
    Permission    : RW
    Comment       : Reboot memory content. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_BOOT_NORMAL_MODE = 0x00,
  LSM6DS3_ACC_GYRO_BOOT_REBOOT_MODE = 0x80,
} LSM6DS3_ACC_GYRO_BOOT_t;

/*******************************************************************************
    Register      : CTRL4_C
    Address       : LSM6DS3_ACC_GYRO_CTRL4_C
    Bit Group Name: STOP_ON_FTH
    Permission    : RW
    Comment       : Enable FIFO threshold level use. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_STOP_ON_FTH_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_STOP_ON_FTH_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_STOP_ON_FTH_t;

/*******************************************************************************
    Register      : CTRL4_C
    Address       : LSM6DS3_ACC_GYRO_CTRL4_C
    Bit Group Name: I2C_DISABLE
    Permission    : RW
    Comment       : Disable I2C interface. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_I2C_DISABLE_I2C_AND_SPI = 0x00,
  LSM6DS3_ACC_GYRO_I2C_DISABLE_SPI_ONLY = 0x04,
} LSM6DS3_ACC_GYRO_I2C_DISABLE_t;

/*******************************************************************************
    Register      : CTRL4_C
    Address       : LSM6DS3_ACC_GYRO_CTRL4_C
    Bit Group Name: DRDY_MSK
    Permission    : RW
    Comment       : Data-ready mask enable. If enabled, when switching from Power-Down to an active mode, the
accelerometer and gyroscope data-ready signals are masked until the settling of the sensor filters is completed. Default
value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DRDY_MSK_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_DRDY_MSK_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_DRDY_MSK_t;

/*******************************************************************************
    Register      : CTRL4_C
    Address       : LSM6DS3_ACC_GYRO_CTRL4_C
    Bit Group Name: FIFO_TEMP_EN
    Permission    : RW
    Comment       : Enable temperature data as 4th FIFO data set(3). Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FIFO_TEMP_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_FIFO_TEMP_EN_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_FIFO_TEMP_EN_t;

/*******************************************************************************
    Register      : CTRL4_C
    Address       : LSM6DS3_ACC_GYRO_CTRL4_C
    Bit Group Name: INT2_ON_INT1
    Permission    : RW
    Comment       : All interrupt signals available on INT1 pad enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_ON_INT1_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_ON_INT1_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_INT2_ON_INT1_t;

/*******************************************************************************
    Register      : CTRL4_C
    Address       : LSM6DS3_ACC_GYRO_CTRL4_C
    Bit Group Name: SLEEP_G
    Permission    : RW
    Comment       : Gyroscope sleep mode enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SLEEP_G_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_SLEEP_G_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_SLEEP_G_t;

/*******************************************************************************
    Register      : CTRL4_C
    Address       : LSM6DS3_ACC_GYRO_CTRL4_C
    Bit Group Name: BW_SCAL_ODR
    Permission    : RW
    Comment       : Accelerometer bandwidth selection. Default value: 0
    0: bandwidth determined by ODR selection, refer to Table 48;
    1: bandwidth determined by setting BW_XL[1:0] in CTRL1_XL (10h) regist
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_BW_SCAL_ODR_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_BW_SCAL_ODR_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_BW_SCAL_ODR_t;

/*******************************************************************************
    Register      : CTRL5_C
    Address       : LSM6DS3_ACC_GYRO_CTRL5_C
    Bit Group Name: ST_XL
    Permission    : RW
    Comment       : Linear acceleration sensor self-test enable. Default value: 00
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ST_XL_NORMAL_MODE = 0x00,
  LSM6DS3_ACC_GYRO_ST_XL_POS_SIGN_TEST = 0x01,
  LSM6DS3_ACC_GYRO_ST_XL_NEG_SIGN_TEST = 0x02,
  LSM6DS3_ACC_GYRO_ST_XL_NA = 0x03,
} LSM6DS3_ACC_GYRO_ST_XL_t;

/*******************************************************************************
    Register      : CTRL5_C
    Address       : LSM6DS3_ACC_GYRO_CTRL5_C
    Bit Group Name: ST_G
    Permission    : RW
    Comment       : Angular rate sensor self-test enable. Default value: 00
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ST_G_NORMAL_MODE = 0x00,
  LSM6DS3_ACC_GYRO_ST_G_POS_SIGN_TEST = 0x04,
  LSM6DS3_ACC_GYRO_ST_G_NA = 0x08,
  LSM6DS3_ACC_GYRO_ST_G_NEG_SIGN_TEST = 0x0C,
} LSM6DS3_ACC_GYRO_ST_G_t;

/*******************************************************************************
    Register      : CTRL5_C
    Address       : LSM6DS3_ACC_GYRO_CTRL5_C
    Bit Group Name: ROUNDING
    Permission    : RW
    Comment       : Circular burst-mode (rounding) read from output registers. Default: 000
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ROUNDING_NONE = 0x00,
  LSM6DS3_ACC_GYRO_ROUNDING_ACCEL = 0x20,
  LSM6DS3_ACC_GYRO_ROUNDING_GYRO = 0x40,
  LSM6DS3_ACC_GYRO_ROUNDING_GYRO_ACCEL = 0x60,
  // Registers from SENSORHUB1_REG (2Eh) to SENSORHUB6_REG (33h) only
  LSM6DS3_ACC_GYRO_ROUNDING_REG_SENSORHUB = 0x80,
  // Accelerometer + registers from SENSORHUB1_REG (2Eh) to SENSORHUB6_REG (33h)
  LSM6DS3_ACC_GYRO_ROUNDING_ACCEL_REG_SENSORHUB = 0xA0,
  // Gyroscope + accelerometer + registers from SENSORHUB1_REG (2Eh) to SENSORHUB6_REG (33h) and registers from
  // SENSORHUB7_REG (34h) to SENSORHUB12_REG(39h)
  LSM6DS3_ACC_GYRO_ROUNDING_GYRO_REG_SENSORHUB = 0xC0,
  // Gyroscope + accelerometer + registers from SENSORHUB1_REG (2Eh) to SENSORHUB6_REG (33h)
  LSM6DS3_ACC_GYRO_ROUNDING_ACCEL_GYRO_REG_SENSORHUB = 0xE0,
} LSM6DS3_ACC_GYRO_ROUNDING_t;

/*******************************************************************************
    Register      : CTRL6_C
    Address       : LSM6DS3_ACC_GYRO_CTRL6_C
    Bit Group Name: XL_HM_MODE
    Permission    : RW
    Comment       : High-performance operating mode disable for accelerometer(1). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_XL_HM_MODE_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_XL_HM_MODE_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_XL_HM_MODE_t;

/*******************************************************************************
    Register      : CTRL6_C
    Address       : LSM6DS3_ACC_GYRO_CTRL6_C
    Bit Group Name: LVL2_EN
    Permission    : RW
    Comment       : Gyroscope level-sensitive latched enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_LVL2_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_LVL2_EN_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_LVL2_EN_t;

/*******************************************************************************
    Register      : CTRL6_C
    Address       : LSM6DS3_ACC_GYRO_CTRL6_C
    Bit Group Name: LVL_EN
    Permission    : RW
    Comment       : Gyroscope data level-sensitive trigger enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_LVL_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_LVL_EN_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_LVL_EN_t;

/*******************************************************************************
    Register      : CTRL6_C
    Address       : LSM6DS3_ACC_GYRO_CTRL6_C
    Bit Group Name: TRIG_EN
    Permission    : RW
    Comment       : Gyroscope data edge-sensitive trigger enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TRIG_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TRIG_EN_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_TRIG_EN_t;

/*******************************************************************************
    Register      : CTRL7_G
    Address       : LSM6DS3_ACC_GYRO_CTRL7_G
    Bit Group Name: ROUNDING_STATUS
    Permission    : RW
    Comment       : Source register rounding function enable on STATUS_REG (1Eh), FUNC_SRC (53h) and WAKE_UP_SRC (1Bh)
registers. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ROUNDING_STATUS_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_ROUNDING_STATUS_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_ROUNDING_STATUS_t;

/*******************************************************************************
    Register      : CTRL7_G
    Address       : LSM6DS3_ACC_GYRO_CTRL7_G
    Bit Group Name: HP_G_RST
    Permission    : RW
    Comment       : Gyro digital HP filter reset. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_HP_G_RST_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_HP_G_RST_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_HP_G_RST_t;

/*******************************************************************************
    Register      : CTRL7_G
    Address       : LSM6DS3_ACC_GYRO_CTRL7_G
    Bit Group Name: HPCF_G
    Permission    : RW
    Comment       : Gyroscope high-pass filter cutoff frequency selection. Default value: 00.
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_HPCF_G_0_0081HZ = 0x00,
  LSM6DS3_ACC_GYRO_HPCF_G_0_0324HZ = 0x10,
  LSM6DS3_ACC_GYRO_HPCF_G_2_0700HZ = 0x20,
  LSM6DS3_ACC_GYRO_HPCF_G_16_320HZ = 0x30,
} LSM6DS3_ACC_GYRO_HPCF_G_t;

/*******************************************************************************
    Register      : CTRL7_G
    Address       : LSM6DS3_ACC_GYRO_CTRL7_G
    Bit Group Name: HP_G_EN
    Permission    : RW
    Comment       : Gyroscope digital high-pass filter enable. The filter is enabled only if the gyro is in HP mode.
Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_HPG_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_HPG_EN_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_HPG_EN_t;

/*******************************************************************************
    Register      : CTRL7_G
    Address       : LSM6DS3_ACC_GYRO_CTRL7_G
    Bit Group Name: G_HM_MODE
    Permission    : RW
    Comment       : High-performance operating mode disable for gyroscope. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_G_HM_MODE_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_G_HM_MODE_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_G_HM_MODE_t;

/*******************************************************************************
    Register      : CTRL8_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL8_XL
    Bit Group Name: LOW_PASS_ON_6D
    Permission    : RW
    Comment       : Low-pass filter on 6D function selection.
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_LOW_PASS_ON_6D_OFF = 0x00,
  LSM6DS3_ACC_GYRO_LOW_PASS_ON_6D_ON = 0x01,
} LSM6DS3_ACC_GYRO_LOW_PASS_ON_6D_t;

/*******************************************************************************
    Register      : CTRL8_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL8_XL
    Bit Group Name: HP_SLOPE_XL_EN
    Permission    : RW
    Comment       : Accelerometer slope filter / high-pass filter selection
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_HP_SLOPE_XL_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_HP_SLOPE_XL_EN_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_HP_SLOPE_XL_EN_t;

/*******************************************************************************
    Register      : CTRL8_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL8_XL
    Bit Group Name: HPCF_XL
    Permission    : RW
    Comment       : Accelerometer slope filter and high-pass filter configuration and cutoff setting. Refer to Table 68.
It is also used to select the cutoff frequency of the LPF2 filter, as shown in Table 69. This low-pass filter can also
be used in the 6D/4D functionality by setting the LOW_PASS_ON_6D bit of CTRL8_XL (17h) to 1.
*******************************************************************************/
typedef enum
{
  // ODR_XL/4
  LSM6DS3_ACC_GYRO_HPCF_XL_SLOPE = 0x00,
  // ODR_XL/100
  LSM6DS3_ACC_GYRO_HPCF_XL_GP_100 = 0x20,
  // ODR_XL/9
  LSM6DS3_ACC_GYRO_HPCF_XL_GP_9 = 0x40,
  // ODR_XL/400
  LSM6DS3_ACC_GYRO_HPCF_XL_GP_400 = 0x60,
} LSM6DS3_ACC_GYRO_HPCF_XL_t;

/*******************************************************************************
    Register      : CTRL8_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL8_XL
    Bit Group Name: LPF2_XL_EN
    Permission    : RW
    Comment       : Accelerometer low-pass filter LPF2 selection. Refer to Figure 6.
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_LPF2_XL_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_LPF2_XL_EN_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_LPF2_XL_EN_t;

/*******************************************************************************
    Register      : CTRL9_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL9_XL
    Bit Group Name: SOFT_EN
    Permission    : RW
    Comment       : Enable soft-iron correction algorithm for magnetometer(1). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SOFT_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_SOFT_EN_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_SOFT_EN_t;

/*******************************************************************************
    Register      : CTRL9_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL9_XL
    Bit Group Name: XEN_XL
    Permission    : RW
    Comment       : Accelerometer X-axis output enable. Default value: 1
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_XEN_XL_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_XEN_XL_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_XEN_XL_t;

/*******************************************************************************
    Register      : CTRL9_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL9_XL
    Bit Group Name: YEN_XL
    Permission    : RW
    Comment       : Accelerometer Y-axis output enable. Default value: 1
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_YEN_XL_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_YEN_XL_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_YEN_XL_t;

/*******************************************************************************
    Register      : CTRL9_XL
    Address       : LSM6DS3_ACC_GYRO_CTRL9_XL
    Bit Group Name: ZEN_XL
    Permission    : RW
    Comment       : Accelerometer Z-axis output enable. Default value: 1
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ZEN_XL_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_ZEN_XL_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_ZEN_XL_t;

/*******************************************************************************
    Register      : CTRL10_C
    Address       : LSM6DS3_ACC_GYRO_CTRL10_C
    Bit Group Name: SIGN_MOTION_EN
    Permission    : RW
    Comment       : Enable significant motion function. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_t;

/*******************************************************************************
    Register      : CTRL10_C
    Address       : LSM6DS3_ACC_GYRO_CTRL10_C
    Bit Group Name: PEDO_RST_STEP
    Permission    : RW
    Comment       : Reset pedometer step counter. Default value:
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_PEDO_RST_STEP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_PEDO_RST_STEP_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_PEDO_RST_STEP_t;

/*******************************************************************************
    Register      : CTRL10_C
    Address       : LSM6DS3_ACC_GYRO_CTRL10_C
    Bit Group Name: FUNC_EN
    Permission    : RW
    Comment       : Enable embedded functionalities (pedometer, tilt, significant motion, sensor hub and ironing) and
accelerometer HP and LPF2 filters (refer to Figure 6). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FUNC_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_FUNC_EN_t;

/*******************************************************************************
    Register      : CTRL10_C
    Address       : LSM6DS3_ACC_GYRO_CTRL10_C
    Bit Group Name: XEN_G
    Permission    : RW
    Comment       : Gyroscope pitch axis (X) output enable. Default value: 1
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_XEN_G_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_XEN_G_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_XEN_G_t;

/*******************************************************************************
    Register      : CTRL10_C
    Address       : LSM6DS3_ACC_GYRO_CTRL10_C
    Bit Group Name: YEN_G
    Permission    : RW
    Comment       : Gyroscope roll axis (Y) output enable. Default value: 1
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_YEN_G_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_YEN_G_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_YEN_G_t;

/*******************************************************************************
    Register      : CTRL10_C
    Address       : LSM6DS3_ACC_GYRO_CTRL10_C
    Bit Group Name: ZEN_G
    Permission    : RW
    Comment       : Gyroscope yaw axis (Z) output enable. Default value: 1
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_ZEN_G_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_ZEN_G_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_ZEN_G_t;

/*******************************************************************************
    Register      : MASTER_CONFIG
    Address       : LSM6DS3_ACC_GYRO_MASTER_CONFIG
    Bit Group Name: MASTER_ON
    Permission    : RW
    Comment       : Sensor hub I2C master enable. Default: 0
    (0: master I2C of sensor hub disabled; 1: master I2C of sensor hub enabled)
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_MASTER_ON_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_MASTER_ON_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_MASTER_ON_t;

/*******************************************************************************
    Register      : MASTER_CONFIG
    Address       : LSM6DS3_ACC_GYRO_MASTER_CONFIG
    Bit Group Name: IRON_EN
    Permission    : RW
    Comment       : Enable hard-iron correction algorithm for magnetometer. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_IRON_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_IRON_EN_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_IRON_EN_t;

/*******************************************************************************
    Register      : MASTER_CONFIG
    Address       : LSM6DS3_ACC_GYRO_MASTER_CONFIG
    Bit Group Name: PASS_THRU_MODE
    Permission    : RW
    Comment       : I2C interface pass-through. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_PASS_THRU_MODE_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_PASS_THRU_MODE_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_PASS_THRU_MODE_t;

/*******************************************************************************
    Register      : MASTER_CONFIG
    Address       : LSM6DS3_ACC_GYRO_MASTER_CONFIG
    Bit Group Name: PULL_UP_EN
    Permission    : RW
    Comment       : Auxiliary I2C pull-up. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_PULL_UP_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_PULL_UP_EN_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_PULL_UP_EN_t;

/*******************************************************************************
    Register      : MASTER_CONFIG
    Address       : LSM6DS3_ACC_GYRO_MASTER_CONFIG
    Bit Group Name: START_CONFIG
    Permission    : RW
    Comment       : Sensor Hub trigger signal selection. Default valu
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_START_CONFIG_XL_G_DRDY = 0x00,
  LSM6DS3_ACC_GYRO_START_CONFIG_EXT_INT2 = 0x10,
} LSM6DS3_ACC_GYRO_START_CONFIG_t;

/*******************************************************************************
    Register      : MASTER_CONFIG
    Address       : LSM6DS3_ACC_GYRO_MASTER_CONFIG
    Bit Group Name: DATA_VAL_SEL_FIFO
    Permission    : RW
    Comment       : Selection of FIFO data-valid signal. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DATA_VAL_SEL_FIFO_XL_G_DRDY = 0x00,
  LSM6DS3_ACC_GYRO_DATA_VAL_SEL_FIFO_SHUB_DRDY = 0x40,
} LSM6DS3_ACC_GYRO_DATA_VAL_SEL_FIFO_t;

/*******************************************************************************
    Register      : MASTER_CONFIG
    Address       : LSM6DS3_ACC_GYRO_MASTER_CONFIG
    Bit Group Name: DRDY_ON_INT1
    Permission    : RW
    Comment       : Manage the Master DRDY signal on INT1 pad. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DRDY_ON_INT1_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_DRDY_ON_INT1_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_DRDY_ON_INT1_t;

/*******************************************************************************
    Register      : WAKE_UP_SRC
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_SRC
    Bit Group Name: Z_WU
    Permission    : RO
    Comment       : Wakeup event detection status on Z-axis. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_Z_WU_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_Z_WU_DETECTED = 0x01,
} LSM6DS3_ACC_GYRO_Z_WU_t;

/*******************************************************************************
    Register      : WAKE_UP_SRC
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_SRC
    Bit Group Name: Y_WU
    Permission    : RO
    Comment       : Wakeup event detection status on Y-axis. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_Y_WU_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_Y_WU_DETECTED = 0x02,
} LSM6DS3_ACC_GYRO_Y_WU_t;

/*******************************************************************************
    Register      : WAKE_UP_SRC
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_SRC
    Bit Group Name: X_WU
    Permission    : RO
    Comment       : Wakeup event detection status on X-axis. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_X_WU_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_X_WU_DETECTED = 0x04,
} LSM6DS3_ACC_GYRO_X_WU_t;

/*******************************************************************************
    Register      : WAKE_UP_SRC
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_SRC
    Bit Group Name: WU_EV_STATUS
    Permission    : RO
    Comment       : Wakeup event detection status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_WU_EV_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_WU_EV_STATUS_DETECTED = 0x08,
} LSM6DS3_ACC_GYRO_WU_EV_STATUS_t;

/*******************************************************************************
    Register      : WAKE_UP_SRC
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_SRC
    Bit Group Name: SLEEP_EV_STATUS
    Permission    : RO
    Comment       : Sleep event status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SLEEP_EV_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_SLEEP_EV_STATUS_DETECTED = 0x10,
} LSM6DS3_ACC_GYRO_SLEEP_EV_STATUS_t;

/*******************************************************************************
    Register      : WAKE_UP_SRC
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_SRC
    Bit Group Name: FF_EV_STATUS
    Permission    : RO
    Comment       : Free-fall event detection status. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FF_EV_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_FF_EV_STATUS_DETECTED = 0x20,
} LSM6DS3_ACC_GYRO_FF_EV_STATUS_t;

/*******************************************************************************
    Register      : TAP_SRC
    Address       : LSM6DS3_ACC_GYRO_TAP_SRC
    Bit Group Name: Z_TAP
    Permission    : RO
    Comment       : Tap event detection status on Z-axis. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_Z_TAP_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_Z_TAP_DETECTED = 0x01,
} LSM6DS3_ACC_GYRO_Z_TAP_t;

/*******************************************************************************
    Register      : TAP_SRC
    Address       : LSM6DS3_ACC_GYRO_TAP_SRC
    Bit Group Name: Y_TAP
    Permission    : RO
    Comment       : Tap event detection status on Y-axis. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_Y_TAP_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_Y_TAP_DETECTED = 0x02,
} LSM6DS3_ACC_GYRO_Y_TAP_t;

/*******************************************************************************
    Register      : TAP_SRC
    Address       : LSM6DS3_ACC_GYRO_TAP_SRC
    Bit Group Name: X_TAP
    Permission    : RO
    Comment       : Tap event detection status on X-axis. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_X_TAP_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_X_TAP_DETECTED = 0x04,
} LSM6DS3_ACC_GYRO_X_TAP_t;

/*******************************************************************************
    Register      : TAP_SRC
    Address       : LSM6DS3_ACC_GYRO_TAP_SRC
    Bit Group Name: TAP_SIGN
    Permission    : RO
    Comment       : Sign of acceleration detected by tap event. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TAP_SIGN_POS_SIGN = 0x00,
  LSM6DS3_ACC_GYRO_TAP_SIGN_NEG_SIGN = 0x08,
} LSM6DS3_ACC_GYRO_TAP_SIGN_t;

/*******************************************************************************
    Register      : TAP_SRC
    Address       : LSM6DS3_ACC_GYRO_TAP_SRC
    Bit Group Name: DOUBLE_TAP_EV_STATUS
    Permission    : RO
    Comment       : Double-tap event detection status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DOUBLE_TAP_EV_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_DOUBLE_TAP_EV_STATUS_DETECTED = 0x10,
} LSM6DS3_ACC_GYRO_DOUBLE_TAP_EV_STATUS_t;

/*******************************************************************************
    Register      : TAP_SRC
    Address       : LSM6DS3_ACC_GYRO_TAP_SRC
    Bit Group Name: SINGLE_TAP_EV_STATUS
    Permission    : RO
    Comment       : Single-tap event status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SINGLE_TAP_EV_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_SINGLE_TAP_EV_STATUS_DETECTED = 0x20,
} LSM6DS3_ACC_GYRO_SINGLE_TAP_EV_STATUS_t;

/*******************************************************************************
    Register      : TAP_SRC
    Address       : LSM6DS3_ACC_GYRO_TAP_SRC
    Bit Group Name: TAP_EV_STATUS
    Permission    : RO
    Comment       : Tap event detection status. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TAP_EV_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_TAP_EV_STATUS_DETECTED = 0x40,
} LSM6DS3_ACC_GYRO_TAP_EV_STATUS_t;

/*******************************************************************************
    Register      : D6D_SRC
    Address       : LSM6DS3_ACC_GYRO_D6D_SRC
    Bit Group Name: DSD_XL
    Permission    : RO
    Comment       : X-axis low event (under threshold). Default value:
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DSD_XL_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_DSD_XL_DETECTED = 0x01,
} LSM6DS3_ACC_GYRO_DSD_XL_t;

/*******************************************************************************
    Register      : D6D_SRC
    Address       : LSM6DS3_ACC_GYRO_D6D_SRC
    Bit Group Name: DSD_XH
    Permission    : RO
    Comment       : X-axis high event (over threshold). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DSD_XH_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_DSD_XH_DETECTED = 0x02,
} LSM6DS3_ACC_GYRO_DSD_XH_t;

/*******************************************************************************
    Register      : D6D_SRC
    Address       : LSM6DS3_ACC_GYRO_D6D_SRC
    Bit Group Name: DSD_YL
    Permission    : RO
    Comment       : Y-axis low event (under threshold). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DSD_YL_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_DSD_YL_DETECTED = 0x04,
} LSM6DS3_ACC_GYRO_DSD_YL_t;

/*******************************************************************************
    Register      : D6D_SRC
    Address       : LSM6DS3_ACC_GYRO_D6D_SRC
    Bit Group Name: DSD_YH
    Permission    : RO
    Comment       : Y-axis high event (over threshold). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DSD_YH_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_DSD_YH_DETECTED = 0x08,
} LSM6DS3_ACC_GYRO_DSD_YH_t;

/*******************************************************************************
    Register      : D6D_SRC
    Address       : LSM6DS3_ACC_GYRO_D6D_SRC
    Bit Group Name: DSD_ZL
    Permission    : RO
    Comment       : Z-axis low event (under threshold). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DSD_ZL_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_DSD_ZL_DETECTED = 0x10,
} LSM6DS3_ACC_GYRO_DSD_ZL_t;

/*******************************************************************************
    Register      : D6D_SRC
    Address       : LSM6DS3_ACC_GYRO_D6D_SRC
    Bit Group Name: DSD_ZH
    Permission    : RO
    Comment       : Z-axis high event (over threshold). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_DSD_ZH_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_DSD_ZH_DETECTED = 0x20,
} LSM6DS3_ACC_GYRO_DSD_ZH_t;

/*******************************************************************************
    Register      : D6D_SRC
    Address       : LSM6DS3_ACC_GYRO_D6D_SRC
    Bit Group Name: D6D_EV_STATUS
    Permission    : RO
    Comment       : Interrupt active for change position portrait, landscape, face-up, face-down. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_D6D_EV_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_D6D_EV_STATUS_DETECTED = 0x40,
} LSM6DS3_ACC_GYRO_D6D_EV_STATUS_t;

/*******************************************************************************
    Register      : STATUS_REG
    Address       : LSM6DS3_ACC_GYRO_STATUS_REG
    Bit Group Name: XLDA
    Permission    : RO
    Comment       : Accelerometer new data available. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_XLDA_NO_DATA_AVAIL = 0x00,
  LSM6DS3_ACC_GYRO_XLDA_DATA_AVAIL = 0x01,
} LSM6DS3_ACC_GYRO_XLDA_t;

/*******************************************************************************
    Register      : STATUS_REG
    Address       : LSM6DS3_ACC_GYRO_STATUS_REG
    Bit Group Name: GDA
    Permission    : RO
    Comment       : Gyroscope new data available. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_GDA_NO_DATA_AVAIL = 0x00,
  LSM6DS3_ACC_GYRO_GDA_DATA_AVAIL = 0x02,
} LSM6DS3_ACC_GYRO_GDA_t;

/*******************************************************************************
    Register      : STATUS_REG
    Address       : LSM6DS3_ACC_GYRO_STATUS_REG
    Bit Group Name: EV_BOOT
    Permission    : RO
    Comment       : Temperature new data available. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TDA_NO_DATA_AVAIL = 0x00,
  LSM6DS3_ACC_GYRO_TDA_DATA_AVAIL = 0x04,
} LSM6DS3_ACC_GYRO_TDA_t;

/*******************************************************************************
    Register      : FIFO_STATUS1
    Address       : LSM6DS3_ACC_GYRO_FIFO_STATUS1
    Bit Group Name: DIFF_FIFO
    Permission    : RO
    Comment       : Number of unread words (16-bit axes) stored in FIFO
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_DIFF_FIFO_STATUS1_MASK     0xFF
#define LSM6DS3_ACC_GYRO_DIFF_FIFO_STATUS1_POSITION 0

/*******************************************************************************
    Register      : FIFO_STATUS2
    Address       : LSM6DS3_ACC_GYRO_FIFO_STATUS1
    Bit Group Name: DIFF_FIFO
    Permission    : RO
    Comment       : Number of unread words (16-bit axes) stored in FIFO
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_DIFF_FIFO_STATUS2_MASK     0xF
#define LSM6DS3_ACC_GYRO_DIFF_FIFO_STATUS2_POSITION 0

/*******************************************************************************
    Register      : FIFO_STATUS2
    Address       : LSM6DS3_ACC_GYRO_FIFO_STATUS2
    Bit Group Name: FIFO_EMPTY
    Permission    : RO
    Comment       : FIFO empty bit. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FIFO_EMPTY_FIFO_NOT_EMPTY = 0x00,
  LSM6DS3_ACC_GYRO_FIFO_EMPTY_FIFO_EMPTY = 0x10,
} LSM6DS3_ACC_GYRO_FIFO_EMPTY_t;

/*******************************************************************************
    Register      : FIFO_STATUS2
    Address       : LSM6DS3_ACC_GYRO_FIFO_STATUS2
    Bit Group Name: FIFO_FULL
    Permission    : RO
    Comment       : FIFO full status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FIFO_FULL_FIFO_NOT_FULL = 0x00,
  LSM6DS3_ACC_GYRO_FIFO_FULL_FIFO_FULL = 0x20,
} LSM6DS3_ACC_GYRO_FIFO_FULL_t;

/*******************************************************************************
    Register      : FIFO_STATUS2
    Address       : LSM6DS3_ACC_GYRO_FIFO_STATUS2
    Bit Group Name: OVERRUN
    Permission    : RO
    Comment       : IFO overrun status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_OVERRUN_NO_OVERRUN = 0x00,
  LSM6DS3_ACC_GYRO_OVERRUN_OVERRUN = 0x40,
} LSM6DS3_ACC_GYRO_OVERRUN_t;

/*******************************************************************************
    Register      : FIFO_STATUS2
    Address       : LSM6DS3_ACC_GYRO_FIFO_STATUS2
    Bit Group Name: WTM
    Permission    : RO
    Comment       : FIFO watermark status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_WTM_BELOW_WTM = 0x00,
  LSM6DS3_ACC_GYRO_WTM_ABOVE_OR_EQUAL_WTM = 0x80,
} LSM6DS3_ACC_GYRO_WTM_t;

/*******************************************************************************
    Register      : FIFO_STATUS3
    Address       : LSM6DS3_ACC_GYRO_FIFO_STATUS3
    Bit Group Name: FIFO_PATTERN
    Permission    : RO
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_FIFO_STATUS3_PATTERN_MASK     0xFF
#define LSM6DS3_ACC_GYRO_FIFO_STATUS3_PATTERN_POSITION 0
#define LSM6DS3_ACC_GYRO_FIFO_STATUS4_PATTERN_MASK     0x03
#define LSM6DS3_ACC_GYRO_FIFO_STATUS4_PATTERN_POSITION 0

/*******************************************************************************
    Register      : FUNC_SRC
    Address       : LSM6DS3_ACC_GYRO_FUNC_SRC
    Bit Group Name: SENS_HUB_END
    Permission    : RO
    Comment       : Sensor hub communication status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SENS_HUB_END_STILL_ONGOING = 0x00,
  LSM6DS3_ACC_GYRO_SENS_HUB_END_OP_COMPLETED = 0x01,
} LSM6DS3_ACC_GYRO_SENS_HUB_END_t;

/*******************************************************************************
    Register      : FUNC_SRC
    Address       : LSM6DS3_ACC_GYRO_FUNC_SRC
    Bit Group Name: SOFT_IRON_END
    Permission    : RO
    Comment       : Hard/soft-iron calculation status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SOFT_IRON_END_NOT_COMPLETED = 0x00,
  LSM6DS3_ACC_GYRO_SOFT_IRON_END_COMPLETED = 0x02,
} LSM6DS3_ACC_GYRO_SOFT_IRON_END_t;

/*******************************************************************************
    Register      : FUNC_SRC
    Address       : LSM6DS3_ACC_GYRO_FUNC_SRC
    Bit Group Name: STEP_OVERFLOW
    Permission    : RO
    Comment       : Step counter overflow status. Default valu
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_STEP_NOT_OVERFLOWED = 0x00,
  LSM6DS3_ACC_GYRO_STEP_OVERFLOWED = 0x08,
} LSM6DS3_ACC_GYRO_STEP_OVERFLOW_t;

/*******************************************************************************
    Register      : FUNC_SRC
    Address       : LSM6DS3_ACC_GYRO_FUNC_SRC
    Bit Group Name: STEP_DETECTED
    Permission    : RO
    Comment       : Step detector event detection status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_STEP_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_STEP_DETECTED = 0x10,
} LSM6DS3_ACC_GYRO_STEP_DETECTED_t;

/*******************************************************************************
    Register      : FUNC_SRC
    Address       : LSM6DS3_ACC_GYRO_FUNC_SRC
    Bit Group Name: TILT_IA
    Permission    : RO
    Comment       : Tilt event detection status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TILT_IA_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_TILT_IA_DETECTED = 0x20,
} LSM6DS3_ACC_GYRO_TILT_IA_t;

/*******************************************************************************
    Register      : FUNC_SRC
    Address       : LSM6DS3_ACC_GYRO_FUNC_SRC
    Bit Group Name: SIGN_MOTION_IA
    Permission    : RO
    Comment       : Significant motion event detection status. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SIGN_MOTION_IA_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_SIGN_MOTION_IA_DETECTED = 0x40,
} LSM6DS3_ACC_GYRO_SIGN_MOTION_IA_t;

/*******************************************************************************
    Register      : FUNC_SRC
    Address       : LSM6DS3_ACC_GYRO_FUNC_SRC
    Bit Group Name: STEP_COUNT_DELTA_IA
    Permission    : RO
    Comment       : Pedometer step recognition on delta time status. Default value: 0
    (0: no step recognized during delta time; 1: at least one step recognized during delta time)
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_STEP_COUNT_DELTA_IA_STATUS_NOT_DETECTED = 0x00,
  LSM6DS3_ACC_GYRO_STEP_COUNT_DELTA_IA_STATUS_DETECTED = 0x80,
} LSM6DS3_ACC_GYRO_STEP_COUNT_DELTA_IA_STATUS_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: LIR
    Permission    : RW
    Comment       : Latched Interrupt. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_LIR_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_LIR_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_LIR_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: TAP_Z_EN
    Permission    : RW
    Comment       : Enable Z direction in tap recognition. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TAP_Z_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TAP_Z_EN_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_TAP_Z_EN_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: TAP_Y_EN
    Permission    : RW
    Comment       : Enable Y direction in tap recognition. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TAP_Y_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TAP_Y_EN_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_TAP_Y_EN_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: TAP_X_EN
    Permission    : RW
    Comment       : Enable X direction in tap recognition. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TAP_X_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TAP_X_EN_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_TAP_X_EN_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: SLOPE_FDS
    Permission    : RW
    Comment       : Enable accelerometer HP and LPF2 filters (refer to Figure 6). Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SLOPE_FDS_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_SLOPE_FDS_ENABLED = 1 << 4,
} LSM6DS3_ACC_GYRO_SLOPE_FDS_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: TILT_EN
    Permission    : RW
    Comment       : Tilt calculation enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TILT_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TILT_EN_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_TILT_EN_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: PEDO_EN
    Permission    : RW
    Comment       : Pedometer algorithm enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_PEDO_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_PEDO_EN_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_PEDO_EN_t;

/*******************************************************************************
    Register      : TAP_CFG
    Address       : LSM6DS3_ACC_GYRO_TAP_CFG
    Bit Group Name: TIMER_EN
    Permission    : RW
    Comment       : Timestamp count enable, output data are collected in TIMESTAMP0_REG (40h), TIMESTAMP1_REG (41h),
TIMESTAMP2_REG (42h) register. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TIMER_EN_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_TIMER_EN_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_TIMER_EN_t;

/*******************************************************************************
    Register      : TAP_THS_6D
    Address       : LSM6DS3_ACC_GYRO_TAP_THS_6D
    Bit Group Name: TAP_THS
    Permission    : RW
    Comment       : Threshold for tap recognition. Default value: 00000
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_TAP_THS_MASK     0x1F
#define LSM6DS3_ACC_GYRO_TAP_THS_POSITION 0

/*******************************************************************************
    Register      : TAP_THS_6D
    Address       : LSM6DS3_ACC_GYRO_TAP_THS_6D
    Bit Group Name: SIXD_THS
    Permission    : RW
    Comment       : Threshold for D6D function. Default value: 00
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SIXD_THS_80_degree = 0x00,
  LSM6DS3_ACC_GYRO_SIXD_THS_70_degree = 0x20,
  LSM6DS3_ACC_GYRO_SIXD_THS_60_degree = 0x40,
  LSM6DS3_ACC_GYRO_SIXD_THS_50_degree = 0x60,
} LSM6DS3_ACC_GYRO_SIXD_THS_t;

/*******************************************************************************
    Register      : TAP_THS_6D
    Address       : LSM6DS3_ACC_GYRO_TAP_THS_6D
    Bit Group Name: D4D_EN
    Permission    : RW
    Comment       : 4D orientation detection enable. Z-axis position detection is disabled
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_D4D_DISABLED_degree = 0x00,
  LSM6DS3_ACC_GYRO_D4D_ENABLED_degree = 0x80,
} LSM6DS3_ACC_GYRO_D4D_EN_t;

/*******************************************************************************
    Register      : INT_DUR2
    Address       : LSM6DS3_ACC_GYRO_INT_DUR2
    Bit Group Name: SHOCK
    Permission    : RW
    Comment       : Maximum duration of overthreshold event. Default value: 00
    Maximum duration is the maximum time of an overthreshold signal detection to be recognized as a tap event. The
default value of these bits is 00b which corresponds to 4*ODR_XL time. If the SHOCK[1:0] bits are set to a different
value, 1LSB corresponds to 8*ODR_XL time.
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_SHOCK_MASK     0x03
#define LSM6DS3_ACC_GYRO_SHOCK_POSITION 0

/*******************************************************************************
    Register      : INT_DUR2
    Address       : LSM6DS3_ACC_GYRO_INT_DUR2
    Bit Group Name: QUIET
    Permission    : RW
    Comment       : Expected quiet time after a tap detection. Default value: 00
    Quiet time is the time after the first detected tap in which there must not be any overthreshold event. The default
value of these bits is 00b which corresponds to 2*ODR_XL time. If the QUIET[1:0] bits are set to a different value, 1LSB
corresponds to 4*ODR_XL time.
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_QUIET_MASK     0x0C
#define LSM6DS3_ACC_GYRO_QUIET_POSITION 2

/*******************************************************************************
    Register      : INT_DUR2
    Address       : LSM6DS3_ACC_GYRO_INT_DUR2
    Bit Group Name: DUR
    Permission    : RW
    Comment       : Duration of maximum time gap for double tap recognition. Default: 0000
    When double tap recognition is enabled, this register expresses the maximum time between two consecutive detected
taps to determine a double tap event. The default value of these bits is 0000b which corresponds to 16*ODR_XL time. If
the DUR[3:0] bits are set to a different value, 1LSB corresponds to 32*ODR_XL time.
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_DUR_MASK     0xF0
#define LSM6DS3_ACC_GYRO_DUR_POSITION 4

/*******************************************************************************
    Register      : WAKE_UP_THS
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_THS
    Bit Group Name: WK_THS
    Permission    : RW
    Comment       : Threshold for wakeup. Default value: 000000
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_WK_THS_MASK     0x3F
#define LSM6DS3_ACC_GYRO_WK_THS_POSITION 0

/*******************************************************************************
    Register      : WAKE_UP_THS
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_THS
    Bit Group Name: INACTIVITY_ON
    Permission    : RW
    Comment       : Inactivity event enable. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INACTIVITY_ON_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INACTIVITY_ON_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_INACTIVITY_ON_t;

/*******************************************************************************
    Register      : WAKE_UP_THS
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_THS
    Bit Group Name: SINGLE_DOUBLE_TAP
    Permission    : RW
    Comment       : Single/double-tap event enable. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_SINGLE_DOUBLE_TAP_DOUBLE_TAP = 0x00,
  LSM6DS3_ACC_GYRO_SINGLE_DOUBLE_TAP_SINGLE_TAP = 0x80,
} LSM6DS3_ACC_GYRO_SINGLE_DOUBLE_TAP_t;

/*******************************************************************************
    Register      : WAKE_UP_DUR
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_DUR
    Bit Group Name: SLEEP_DUR
    Permission    : RW
    Comment       : Duration to go in sleep mode. Default value: 00001. LSB = 512 ODR
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_SLEEP_DUR_MASK     0x0F
#define LSM6DS3_ACC_GYRO_SLEEP_DUR_POSITION 0

/*******************************************************************************
    Register      : WAKE_UP_DUR
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_DUR
    Bit Group Name: TIMER_HR
    Permission    : RW
    Comment       : Timestamp register resolution setting(1). Default value: 0. (0: 1LSB = 6.4 ms; 1: 1LSB = 25 μs)
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_TIMER_HR_6_4ms = 0x00,
  LSM6DS3_ACC_GYRO_TIMER_HR_25us = 0x10,
} LSM6DS3_ACC_GYRO_TIMER_HR_t;

/*******************************************************************************
    Register      : WAKE_UP_DUR
    Address       : LSM6DS3_ACC_GYRO_WAKE_UP_DUR
    Bit Group Name: WAKE_DUR
    Permission    : RW
    Comment       : Wake up duration event. Default: 001. LSB = 1 ODR_time
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_WAKE_DUR_MASK     0x60
#define LSM6DS3_ACC_GYRO_WAKE_DUR_POSITION 5

/*******************************************************************************
    Register      : FREE_FALL
    Address       : LSM6DS3_ACC_GYRO_FREE_FALL
    Bit Group Name: FF_DUR
    Permission    : RW
    Comment       : Free-fall duration event. Default: 0
*******************************************************************************/
#define LSM6DS3_ACC_GYRO_FREE_FALL_DUR_MASK     0xF8
#define LSM6DS3_ACC_GYRO_FREE_FALL_DUR_POSITION 3

#define LSM6DS3_ACC_GYRO_FF_WAKE_UP_DUR_MASK     0x80
#define LSM6DS3_ACC_GYRO_FF_WAKE_UP_DUR_POSITION 7

/*******************************************************************************
    Register      : FREE_FALL
    Address       : LSM6DS3_ACC_GYRO_FREE_FALL
    Bit Group Name: FF_THS
    Permission    : RW
    Comment       : Free fall threshold setting. Default: 000
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_FF_THS_5 = 0x00,  // 156mg
  LSM6DS3_ACC_GYRO_FF_THS_7 = 0x01,  // 219mg
  LSM6DS3_ACC_GYRO_FF_THS_8 = 0x02,  // 250mg
  LSM6DS3_ACC_GYRO_FF_THS_10 = 0x03, // 312mg
  LSM6DS3_ACC_GYRO_FF_THS_11 = 0x04, // 344mg
  LSM6DS3_ACC_GYRO_FF_THS_13 = 0x05, // 406mg
  LSM6DS3_ACC_GYRO_FF_THS_15 = 0x06, // 469mg
  LSM6DS3_ACC_GYRO_FF_THS_16 = 0x07, // 500mg
} LSM6DS3_ACC_GYRO_FF_THS_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_TIMER
    Permission    : RW
    Comment       : Routing of end counter event of timer on INT1. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_TIMER_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_TIMER_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_INT1_TIMER_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_TILT
    Permission    : RW
    Comment       : Routing of tilt event on INT1. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_TILT_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_TILT_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_INT1_TILT_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_6D
    Permission    : RW
    Comment       : Routing of 6D event on INT1. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_6D_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_6D_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_INT1_6D_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_TAP
    Permission    : RW
    Comment       : Routing of tap event on INT1. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_TAP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_TAP_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_INT1_TAP_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_FF
    Permission    : RW
    Comment       : Routing of free-fall event on INT1. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_FF_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_FF_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_INT1_FF_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_WU
    Permission    : RW
    Comment       : Routing of wakeup event on INT1. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_WU_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_WU_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_INT1_WU_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_SINGLE_TAP
    Permission    : RW
    Comment       : Single-tap recognition routing on INT1. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_t;

/*******************************************************************************
    Register      : MD1_CFG
    Address       : LSM6DS3_ACC_GYRO_MD1_CFG
    Bit Group Name: INT1_SLEEP
    Permission    : RW
    Comment       : Routing on INT1 of inactivity mode. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT1_SLEEP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT1_SLEEP_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_INT1_SLEEP_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_IRON
    Permission    : RW
    Comment       : Routing of soft-iron/hard-iron algorithm end event on INT2. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_IRON_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_IRON_ENABLED = 0x01,
} LSM6DS3_ACC_GYRO_INT2_IRON_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_TILT
    Permission    : RW
    Comment       : Routing of tilt event on INT2. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_TILT_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_TILT_ENABLED = 0x02,
} LSM6DS3_ACC_GYRO_INT2_TILT_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_6D
    Permission    : RW
    Comment       : Routing of 6D event on INT2. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_6D_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_6D_ENABLED = 0x04,
} LSM6DS3_ACC_GYRO_INT2_6D_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_TAP
    Permission    : RW
    Comment       : Routing of tap event on INT2. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_TAP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_TAP_ENABLED = 0x08,
} LSM6DS3_ACC_GYRO_INT2_TAP_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_FF
    Permission    : RW
    Comment       : Routing of free-fall event on INT2. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_FF_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_FF_ENABLED = 0x10,
} LSM6DS3_ACC_GYRO_INT2_FF_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_WU
    Permission    : RW
    Comment       : Routing of wakeup event on INT2. Default value: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_WU_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_WU_ENABLED = 0x20,
} LSM6DS3_ACC_GYRO_INT2_WU_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_SINGLE_TAP
    Permission    : RW
    Comment       : Single-tap recognition routing on INT2. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_SINGLE_TAP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_SINGLE_TAP_ENABLED = 0x40,
} LSM6DS3_ACC_GYRO_INT2_SINGLE_TAP_t;

/*******************************************************************************
    Register      : MD2_CFG
    Address       : LSM6DS3_ACC_GYRO_MD2_CFG
    Bit Group Name: INT2_SLEEP
    Permission    : RW
    Comment       : Routing on INT2 of inactivity mode. Default: 0
*******************************************************************************/
typedef enum
{
  LSM6DS3_ACC_GYRO_INT2_SLEEP_DISABLED = 0x00,
  LSM6DS3_ACC_GYRO_INT2_SLEEP_ENABLED = 0x80,
} LSM6DS3_ACC_GYRO_INT2_SLEEP_t;

#endif // End of __LSM6DS3IMU_H__ definition check
