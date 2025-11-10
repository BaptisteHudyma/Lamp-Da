#include "imu_wrapper.h"

#include "src/system/platform/i2c.h"
#include "src/system/platform/time.h"
#include "src/system/platform/print.h"

#include "LSM6DS3.h"

namespace imu {

namespace __internal {
// Create a instance of class LSM6DS3
LSM6DS3 IMU(I2C_MODE, imuI2cAddress); // I2C device address
} // namespace __internal

bool Wrapper::init() const
{
  // start with a reset & reboot
  __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL3_C,
                                LSM6DS3_ACC_GYRO_BOOT_t::LSM6DS3_ACC_GYRO_BOOT_REBOOT_MODE |
                                        LSM6DS3_ACC_GYRO_SW_RESET_t::LSM6DS3_ACC_GYRO_SW_RESET_RESET_DEVICE);
  // let device reset
  delay_ms(50);

  if (__internal::IMU.begin() != status_t::IMU_SUCCESS)
    return false;

  uint8_t error = status_t::IMU_SUCCESS;

  // free fall thresholds setup
  const uint8_t timingThreshold =
          (6 << LSM6DS3_ACC_GYRO_FREE_FALL_DUR_POSITION) & LSM6DS3_ACC_GYRO_FREE_FALL_DUR_MASK; // 6 samples for event
  const uint8_t accelerationThreashold =
          LSM6DS3_ACC_GYRO_FF_THS_t::LSM6DS3_ACC_GYRO_FF_THS_5; // acceleration threshold for event
  const uint8_t freeFallParams = timingThreshold | accelerationThreashold;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_FREE_FALL, freeFallParams);

  // disable all
  disable_interrupt1();
  disable_interrupt2();

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::shutdown() const
{
  // TODO
  return false;
}

Reading Wrapper::get_reading() const
{
  Reading reads;
  // coordinate change to the lamp body
  reads.accel.x = __internal::IMU.readFloatAccelX();
  reads.accel.y = __internal::IMU.readFloatAccelY();
  reads.accel.z = __internal::IMU.readFloatAccelZ();

  reads.gyro.x = __internal::IMU.readFloatGyroX();
  reads.gyro.y = __internal::IMU.readFloatGyroY();
  reads.gyro.z = __internal::IMU.readFloatGyroZ();
  return reads;
}

bool Wrapper::enable_free_fall_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // Wakeup source: will wake up the imu when free fall detected
  uint8_t wakeUpFlags = 0;
  error += __internal::IMU.readRegister(&wakeUpFlags, LSM6DS3_ACC_GYRO_WAKE_UP_SRC);
  wakeUpFlags |= LSM6DS3_ACC_GYRO_FF_EV_STATUS_t::LSM6DS3_ACC_GYRO_FF_EV_STATUS_DETECTED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_WAKE_UP_SRC, wakeUpFlags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::disable_free_fall_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // Wakeup source: will wake up the imu when free fall detected
  uint8_t wakeUpFlags = 0;
  error += __internal::IMU.readRegister(&wakeUpFlags, LSM6DS3_ACC_GYRO_WAKE_UP_SRC);
  wakeUpFlags &= ~LSM6DS3_ACC_GYRO_FF_EV_STATUS_t::LSM6DS3_ACC_GYRO_FF_EV_STATUS_DETECTED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_WAKE_UP_SRC, wakeUpFlags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::enable_big_motion_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // enable significant motion detection
  uint8_t ctrl10_flags = 0;
  error += __internal::IMU.readRegister(&ctrl10_flags, LSM6DS3_ACC_GYRO_CTRL10_C);
  ctrl10_flags |= LSM6DS3_ACC_GYRO_FUNC_EN_t::LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED;
  ctrl10_flags |= LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_t::LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, ctrl10_flags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::disable_big_motion_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // enable significant motion detection
  uint8_t ctrl10_flags = 0;
  error += __internal::IMU.readRegister(&ctrl10_flags, LSM6DS3_ACC_GYRO_CTRL10_C);
  ctrl10_flags &= ~LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_t::LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, ctrl10_flags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::enable_step_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // enable functions detection
  uint8_t ctrl10_flags = 0;
  error += __internal::IMU.readRegister(&ctrl10_flags, LSM6DS3_ACC_GYRO_CTRL10_C);
  ctrl10_flags |= LSM6DS3_ACC_GYRO_FUNC_EN_t::LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED;
  ctrl10_flags |= LSM6DS3_ACC_GYRO_PEDO_RST_STEP_t::LSM6DS3_ACC_GYRO_PEDO_RST_STEP_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, ctrl10_flags);

  // enable significant motion detection
  uint8_t tapCfg_flags = 0;
  error += __internal::IMU.readRegister(&tapCfg_flags, LSM6DS3_ACC_GYRO_TAP_CFG);
  tapCfg_flags |= LSM6DS3_ACC_GYRO_PEDO_EN_t::LSM6DS3_ACC_GYRO_PEDO_EN_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG, tapCfg_flags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::disable_step_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // enable significant motion detection
  uint8_t tapCfg_flags = 0;
  error += __internal::IMU.readRegister(&tapCfg_flags, LSM6DS3_ACC_GYRO_TAP_CFG);
  tapCfg_flags |= LSM6DS3_ACC_GYRO_PEDO_EN_t::LSM6DS3_ACC_GYRO_PEDO_EN_DISABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG, tapCfg_flags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::enable_tilt_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // enable functions detection
  uint8_t ctrl10_flags = 0;
  error += __internal::IMU.readRegister(&ctrl10_flags, LSM6DS3_ACC_GYRO_CTRL10_C);
  ctrl10_flags |= LSM6DS3_ACC_GYRO_FUNC_EN_t::LSM6DS3_ACC_GYRO_FUNC_EN_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, ctrl10_flags);

  // enable significant motion detection
  uint8_t tapCfg_flags = 0;
  error += __internal::IMU.readRegister(&tapCfg_flags, LSM6DS3_ACC_GYRO_TAP_CFG);
  tapCfg_flags |= LSM6DS3_ACC_GYRO_TILT_EN_t::LSM6DS3_ACC_GYRO_TILT_EN_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG, tapCfg_flags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::disable_tilt_detection() const
{
  uint8_t error = status_t::IMU_SUCCESS;

  // enable significant motion detection
  uint8_t tapCfg_flags = 0;
  error += __internal::IMU.readRegister(&tapCfg_flags, LSM6DS3_ACC_GYRO_TAP_CFG);
  tapCfg_flags &= ~LSM6DS3_ACC_GYRO_TILT_EN_t::LSM6DS3_ACC_GYRO_TILT_EN_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG, tapCfg_flags);

  return error == status_t::IMU_SUCCESS;
}

void Wrapper::disable_detection(const InterruptType interr) const
{
  switch (interr)
  {
    case InterruptType::FreeFall:
      disable_free_fall_detection();
      break;
    case InterruptType::BigMotion:
      disable_big_motion_detection();
      break;

    case InterruptType::Step:
      disable_step_detection();
      break;

    case InterruptType::AngleChange:
      disable_step_detection();
      break;

    default:
      {
        break;
      }
  }
}

bool Wrapper::enable_interrupt1(const InterruptType interr) const
{
  switch (interr)
  {
    case InterruptType::FreeFall:
      {
        uint8_t error = status_t::IMU_SUCCESS;
        // MD1_CFG Functions routing on INT1 register
        uint8_t int1Flag = 0;
        error += __internal::IMU.readRegister(&int1Flag, LSM6DS3_ACC_GYRO_MD1_CFG);
        int1Flag |= LSM6DS3_ACC_GYRO_INT1_FF_t::LSM6DS3_ACC_GYRO_INT1_FF_ENABLED;
        error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, int1Flag);
        return error == status_t::IMU_SUCCESS;
      }

    case InterruptType::BigMotion:
      {
        uint8_t error = status_t::IMU_SUCCESS;
        // INT1_CTRL Functions routing on INT1 register
        uint8_t int1Flag = 0;
        error += __internal::IMU.readRegister(&int1Flag, LSM6DS3_ACC_GYRO_INT1_CTRL);
        int1Flag |= LSM6DS3_ACC_GYRO_INT1_SIGN_MOT_t::LSM6DS3_ACC_GYRO_INT1_SIGN_MOT_ENABLED;
        error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_INT1_CTRL, int1Flag);
        return error == status_t::IMU_SUCCESS;
      }

    case InterruptType::Step:
      {
        uint8_t error = status_t::IMU_SUCCESS;
        // INT1_CTRL Functions routing on INT1 register
        uint8_t int1Flag = 0;
        error += __internal::IMU.readRegister(&int1Flag, LSM6DS3_ACC_GYRO_INT1_CTRL);
        int1Flag |= LSM6DS3_ACC_GYRO_INT1_PEDO_t::LSM6DS3_ACC_GYRO_INT1_PEDO_ENABLED;
        error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_INT1_CTRL, int1Flag);
        return error == status_t::IMU_SUCCESS;
      }

    case InterruptType::AngleChange:
      {
        uint8_t error = status_t::IMU_SUCCESS;
        // MD1_CFG Functions routing on INT1 register
        uint8_t int1Flag = 0;
        error += __internal::IMU.readRegister(&int1Flag, LSM6DS3_ACC_GYRO_MD1_CFG);
        int1Flag |= LSM6DS3_ACC_GYRO_INT1_TILT_t::LSM6DS3_ACC_GYRO_INT1_TILT_ENABLED;
        error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, int1Flag);
        return error == status_t::IMU_SUCCESS;
      }
    default:
      {
        break;
      }
  }
  lampda_print("enable_interrupt1: case not handled");
  return false;
}

void Wrapper::disable_interrupt1() const
{
  const uint8_t int1Flag = LSM6DS3_ACC_GYRO_INT1_TIMER_t::LSM6DS3_ACC_GYRO_INT1_TIMER_DISABLED |
                           LSM6DS3_ACC_GYRO_INT1_TILT_t::LSM6DS3_ACC_GYRO_INT1_TILT_DISABLED |
                           LSM6DS3_ACC_GYRO_INT1_6D_t::LSM6DS3_ACC_GYRO_INT1_6D_DISABLED |
                           LSM6DS3_ACC_GYRO_INT1_DOUBLE_TAP_t::LSM6DS3_ACC_GYRO_INT1_DOUBLE_TAP_DISABLED |
                           LSM6DS3_ACC_GYRO_INT1_FF_t::LSM6DS3_ACC_GYRO_INT1_FF_DISABLED |
                           LSM6DS3_ACC_GYRO_INT1_WU_t::LSM6DS3_ACC_GYRO_INT1_WU_DISABLED |
                           LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_t::LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_DISABLED |
                           LSM6DS3_ACC_GYRO_INT1_SLEEP_t::LSM6DS3_ACC_GYRO_INT1_SLEEP_DISABLED;
  __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, int1Flag);

  const uint8_t int1Flag2 = LSM6DS3_ACC_GYRO_INT1_DRDY_XL_t::LSM6DS3_ACC_GYRO_INT1_DRDY_XL_DISABLED |
                            LSM6DS3_ACC_GYRO_INT1_DRDY_G_t::LSM6DS3_ACC_GYRO_INT1_DRDY_G_DISABLED |
                            LSM6DS3_ACC_GYRO_INT1_BOOT_t::LSM6DS3_ACC_GYRO_INT1_BOOT_DISABLED |
                            LSM6DS3_ACC_GYRO_INT1_FTH_t::LSM6DS3_ACC_GYRO_INT1_FTH_DISABLED |
                            LSM6DS3_ACC_GYRO_INT1_OVR_t::LSM6DS3_ACC_GYRO_INT1_OVR_DISABLED |
                            LSM6DS3_ACC_GYRO_INT1_FULL_FLAG_t::LSM6DS3_ACC_GYRO_INT1_FULL_FLAG_DISABLED |
                            LSM6DS3_ACC_GYRO_INT1_SIGN_MOT_t::LSM6DS3_ACC_GYRO_INT1_SIGN_MOT_DISABLED |
                            LSM6DS3_ACC_GYRO_INT1_PEDO_t::LSM6DS3_ACC_GYRO_INT1_PEDO_DISABLED;
  __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_INT1_CTRL, int1Flag2);
}

bool Wrapper::enable_interrupt2(const InterruptType interr) const
{
  switch (interr)
  {
    case InterruptType::FreeFall:
      {
        uint8_t error = status_t::IMU_SUCCESS;
        // MD1_CFG Functions routing on INT2 register
        uint8_t int2Flag = 0;
        error += __internal::IMU.readRegister(&int2Flag, LSM6DS3_ACC_GYRO_MD2_CFG);
        int2Flag |= LSM6DS3_ACC_GYRO_INT2_FF_t::LSM6DS3_ACC_GYRO_INT2_FF_ENABLED;
        error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD2_CFG, int2Flag);
        return error == status_t::IMU_SUCCESS;
      }

    case InterruptType::BigMotion:
      {
        // unsupported event on interrupt 2
        return false;
      }

    case InterruptType::Step:
      {
        // unsupported event on interrupt 2
        return false;
      }

    case InterruptType::AngleChange:
      {
        uint8_t error = status_t::IMU_SUCCESS;
        // MD1_CFG Functions routing on INT1 register
        uint8_t int2Flag = 0;
        error += __internal::IMU.readRegister(&int2Flag, LSM6DS3_ACC_GYRO_MD2_CFG);
        int2Flag |= LSM6DS3_ACC_GYRO_INT2_TILT_t::LSM6DS3_ACC_GYRO_INT2_TILT_ENABLED;
        error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD2_CFG, int2Flag);
        return error == status_t::IMU_SUCCESS;
      }
    default:
      {
        break;
      }
  }
  lampda_print("enable_interrupt2: case not handled");
  return false;
}

void Wrapper::disable_interrupt2() const
{
  const uint8_t int2Flag = LSM6DS3_ACC_GYRO_INT2_IRON_t::LSM6DS3_ACC_GYRO_INT2_IRON_DISABLED |
                           LSM6DS3_ACC_GYRO_INT2_TILT_t::LSM6DS3_ACC_GYRO_INT2_TILT_DISABLED |
                           LSM6DS3_ACC_GYRO_INT2_6D_t::LSM6DS3_ACC_GYRO_INT2_6D_DISABLED |
                           LSM6DS3_ACC_GYRO_INT2_DOUBLE_TAP_t::LSM6DS3_ACC_GYRO_INT2_DOUBLE_TAP_DISABLED |
                           LSM6DS3_ACC_GYRO_INT2_FF_t::LSM6DS3_ACC_GYRO_INT2_FF_DISABLED |
                           LSM6DS3_ACC_GYRO_INT2_WU_t::LSM6DS3_ACC_GYRO_INT2_WU_DISABLED |
                           LSM6DS3_ACC_GYRO_INT2_SINGLE_TAP_t::LSM6DS3_ACC_GYRO_INT2_SINGLE_TAP_DISABLED |
                           LSM6DS3_ACC_GYRO_INT2_SLEEP_t::LSM6DS3_ACC_GYRO_INT2_SLEEP_DISABLED;
  __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD2_CFG, int2Flag);

  const uint8_t int2Flag2 = LSM6DS3_ACC_GYRO_INT2_DRDY_XL_t::LSM6DS3_ACC_GYRO_INT2_DRDY_XL_DISABLED |
                            LSM6DS3_ACC_GYRO_INT2_DRDY_G_t::LSM6DS3_ACC_GYRO_INT2_DRDY_G_DISABLED |
                            LSM6DS3_ACC_GYRO_INT2_DRDY_TEMP_t::LSM6DS3_ACC_GYRO_INT2_DRDY_TEMP_DISABLED |
                            LSM6DS3_ACC_GYRO_INT2_FTH_t::LSM6DS3_ACC_GYRO_INT2_FTH_DISABLED |
                            LSM6DS3_ACC_GYRO_INT2_OVR_t::LSM6DS3_ACC_GYRO_INT2_OVR_DISABLED |
                            LSM6DS3_ACC_GYRO_INT2_FULL_FLAG_t::LSM6DS3_ACC_GYRO_INT2_FULL_FLAG_DISABLED |
                            LSM6DS3_ACC_GYRO_INT2_STEP_COUNT_OV_t::LSM6DS3_ACC_GYRO_INT2_STEP_COUNT_OV_DISABLED |
                            LSM6DS3_ACC_GYRO_INT2_STEP_DELTA_t::LSM6DS3_ACC_GYRO_INT2_STEP_DELTA_DISABLED;
  __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_INT2_CTRL, int2Flag2);
}

uint16_t Wrapper::get_step_count() const
{
  uint8_t data1 = 0;
  __internal::IMU.readRegister(&data1, LSM6DS3_ACC_GYRO_STEP_COUNTER_H);
  uint8_t data2 = 0;
  __internal::IMU.readRegister(&data2, LSM6DS3_ACC_GYRO_STEP_COUNTER_L);

  uint16_t res = 0;
  res = static_cast<uint16_t>(data1 << 8 | data2);
  return res;
}

bool Wrapper::is_event_detected(const InterruptType interr) const
{
  switch (interr)
  {
    case InterruptType::FreeFall:
      {
        uint8_t wakeup_flags = 0;
        const uint8_t error = __internal::IMU.readRegister(&wakeup_flags, LSM6DS3_ACC_GYRO_WAKE_UP_SRC);
        // wake event free fall
        return (error == status_t::IMU_SUCCESS) and
               (wakeup_flags & LSM6DS3_ACC_GYRO_FF_EV_STATUS_t::LSM6DS3_ACC_GYRO_FF_EV_STATUS_DETECTED);
      }

    case InterruptType::BigMotion:
      {
        uint8_t func_flags = 0;
        const uint8_t error = __internal::IMU.readRegister(&func_flags, LSM6DS3_ACC_GYRO_FUNC_SRC);
        // big motion detected
        return (error == status_t::IMU_SUCCESS) and
               (func_flags & LSM6DS3_ACC_GYRO_SIGN_MOTION_IA_t::LSM6DS3_ACC_GYRO_SIGN_MOTION_IA_DETECTED);
      }

    case InterruptType::Step:
      {
        uint8_t func_flags = 0;
        const uint8_t error = __internal::IMU.readRegister(&func_flags, LSM6DS3_ACC_GYRO_FUNC_SRC);
        // step detected
        return (error == status_t::IMU_SUCCESS) and
               (func_flags & LSM6DS3_ACC_GYRO_STEP_DETECTED_t::LSM6DS3_ACC_GYRO_STEP_DETECTED);
      }

    case InterruptType::AngleChange:
      {
        uint8_t func_flags = 0;
        const uint8_t error = __internal::IMU.readRegister(&func_flags, LSM6DS3_ACC_GYRO_FUNC_SRC);
        // tilt detected
        return (error == status_t::IMU_SUCCESS) and
               (func_flags & LSM6DS3_ACC_GYRO_TILT_IA_t::LSM6DS3_ACC_GYRO_TILT_IA_DETECTED);
      }

    default:
      {
        break;
      }
  }
  lampda_print("is_event_detected: case not handled");
  return false;
}

} // namespace imu
