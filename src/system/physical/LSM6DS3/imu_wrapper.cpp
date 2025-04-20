#include "imu_wrapper.h"

#include "src/system/platform/i2c.h"
#include "src/system/platform/time.h"

#include "src/system/utils/print.h"

#include "LSM6DS3.h"

namespace imu {

namespace __internal {
// Create a instance of class LSM6DS3
LSM6DS3 IMU(I2C_MODE, imuI2cAddress); // I2C device address
} // namespace __internal

bool Wrapper::init()
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

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::shutdown()
{
  // TODO
  return false;
}

Reading get_reading()
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

bool Wrapper::enable_free_fall_detection()
{
  uint8_t error = status_t::IMU_SUCCESS;

  // Wakeup source: will wake up the imu when free fall detected
  uint8_t wakeUpFlags = 0;
  error += __internal::IMU.readRegister(&wakeUpFlags, LSM6DS3_ACC_GYRO_WAKE_UP_SRC);
  wakeUpFlags |= LSM6DS3_ACC_GYRO_FF_EV_STATUS_t::LSM6DS3_ACC_GYRO_FF_EV_STATUS_DETECTED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_WAKE_UP_SRC, wakeUpFlags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::disable_free_fall_detection()
{
  uint8_t error = status_t::IMU_SUCCESS;

  // Wakeup source: will wake up the imu when free fall detected
  uint8_t wakeUpFlags = 0;
  error += __internal::IMU.readRegister(&wakeUpFlags, LSM6DS3_ACC_GYRO_WAKE_UP_SRC);
  wakeUpFlags &= ~LSM6DS3_ACC_GYRO_FF_EV_STATUS_t::LSM6DS3_ACC_GYRO_FF_EV_STATUS_DETECTED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_WAKE_UP_SRC, wakeUpFlags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::enable_big_motion_detection()
{
  // TODO
  uint8_t error = status_t::IMU_SUCCESS;

  // enable significatn motion detection
  uint8_t ctrl10_flags = 0;
  error += __internal::IMU.readRegister(&ctrl10_flags, LSM6DS3_ACC_GYRO_CTRL10_C);
  ctrl10_flags |= LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_t::LSM6DS3_ACC_GYRO_SIGN_MOTION_EN_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, ctrl10_flags);

  // latch interrupt
  uint8_t tapFlags = 0;
  error += __internal::IMU.readRegister(&tapFlags, LSM6DS3_ACC_GYRO_TAP_CFG);
  tapFlags |= LSM6DS3_ACC_GYRO_LIR_t::LSM6DS3_ACC_GYRO_LIR_ENABLED;
  error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG, tapFlags);

  return error == status_t::IMU_SUCCESS;
}

bool Wrapper::disable_big_motion_detection()
{
  // TODO
  return false;
}

bool Wrapper::enable_detection(const InterruptType interr)
{
  switch (interr)
  {
    case InterruptType::Fall:
      return enable_free_fall_detection();
    case InterruptType::BigMotion:
      return enable_big_motion_detection();

    case InterruptType::Step:
      {
        break;
      }

    case InterruptType::AngleChange:
      {
        break;
      }

    case InterruptType::None:
      {
        break;
      }
    default:
      {
        break;
      }
  }
  lampda_print("enable_detection: case not handled");
  return false;
}

void Wrapper::disable_detection(const InterruptType interr)
{
  switch (interr)
  {
    case InterruptType::Fall:
      disable_free_fall_detection();
      break;
    case InterruptType::BigMotion:
      disable_big_motion_detection();
      break;

    case InterruptType::Step:
      {
        break;
      }

    case InterruptType::AngleChange:
      {
        break;
      }

    case InterruptType::None:
      {
        break;
      }
    default:
      {
        break;
      }
  }
}

bool Wrapper::enable_interrupt1(const InterruptType interr)
{
  switch (interr)
  {
    case InterruptType::Fall:
      {
        if (enable_free_fall_detection())
        {
          uint8_t error = status_t::IMU_SUCCESS;
          // MD1_CFG Functions routing on INT1 register
          const uint8_t int1Flag = LSM6DS3_ACC_GYRO_INT1_TIMER_t::LSM6DS3_ACC_GYRO_INT1_TIMER_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_TILT_t::LSM6DS3_ACC_GYRO_INT1_TILT_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_6D_t::LSM6DS3_ACC_GYRO_INT1_6D_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_TAP_t::LSM6DS3_ACC_GYRO_INT1_TAP_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_FF_t::LSM6DS3_ACC_GYRO_INT1_FF_ENABLED | // enable free fall
                                   LSM6DS3_ACC_GYRO_INT1_WU_t::LSM6DS3_ACC_GYRO_INT1_WU_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_t::LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_SLEEP_t::LSM6DS3_ACC_GYRO_INT1_SLEEP_DISABLED;
          error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, int1Flag);
          return error == status_t::IMU_SUCCESS;
        }
      }

    case InterruptType::BigMotion:
      {
        if (enable_big_motion_detection())
        {
          uint8_t error = status_t::IMU_SUCCESS;
          // MD1_CFG Functions routing on INT1 register
          const uint8_t int1Flag = LSM6DS3_ACC_GYRO_INT1_TIMER_t::LSM6DS3_ACC_GYRO_INT1_TIMER_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_TILT_t::LSM6DS3_ACC_GYRO_INT1_TILT_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_6D_t::LSM6DS3_ACC_GYRO_INT1_6D_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_TAP_t::LSM6DS3_ACC_GYRO_INT1_TAP_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_FF_t::LSM6DS3_ACC_GYRO_INT1_FF_ENABLED | // enable free fall
                                   LSM6DS3_ACC_GYRO_INT1_WU_t::LSM6DS3_ACC_GYRO_INT1_WU_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_t::LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_DISABLED |
                                   LSM6DS3_ACC_GYRO_INT1_SLEEP_t::LSM6DS3_ACC_GYRO_INT1_SLEEP_DISABLED;
          error += __internal::IMU.writeRegister(LSM6DS3_ACC_GYRO_MD1_CFG, int1Flag);
          return error == status_t::IMU_SUCCESS;
        }
        break;
      }

    case InterruptType::Step:
      {
        break;
      }

    case InterruptType::AngleChange:
      {
        break;
      }

    case InterruptType::None:
      {
        uint8_t error = status_t::IMU_SUCCESS;

        // MD1_CFG Functions routing on INT1 register
        const uint8_t int1Flag = LSM6DS3_ACC_GYRO_INT1_TIMER_t::LSM6DS3_ACC_GYRO_INT1_TIMER_DISABLED |
                                 LSM6DS3_ACC_GYRO_INT1_TILT_t::LSM6DS3_ACC_GYRO_INT1_TILT_DISABLED |
                                 LSM6DS3_ACC_GYRO_INT1_6D_t::LSM6DS3_ACC_GYRO_INT1_6D_DISABLED |
                                 LSM6DS3_ACC_GYRO_INT1_TAP_t::LSM6DS3_ACC_GYRO_INT1_TAP_DISABLED |
                                 LSM6DS3_ACC_GYRO_INT1_FF_t::LSM6DS3_ACC_GYRO_INT1_FF_DISABLED |
                                 LSM6DS3_ACC_GYRO_INT1_WU_t::LSM6DS3_ACC_GYRO_INT1_WU_DISABLED |
                                 LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_t::LSM6DS3_ACC_GYRO_INT1_SINGLE_TAP_DISABLED |
                                 LSM6DS3_ACC_GYRO_INT1_SLEEP_t::LSM6DS3_ACC_GYRO_INT1_SLEEP_DISABLED;
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

bool Wrapper::is_event_detected(const InterruptType interr)
{
  switch (interr)
  {
    case InterruptType::Fall:
      {
        uint8_t wakeup_flags = 0;
        const uint8_t error = __internal::IMU.readRegister(&wakeup_flags, LSM6DS3_ACC_GYRO_WAKE_UP_SRC);
        // wake event free fall
        return (error == status_t::IMU_SUCCESS) and
               (wakeup_flags & LSM6DS3_ACC_GYRO_FF_EV_STATUS_t::LSM6DS3_ACC_GYRO_FF_EV_STATUS_DETECTED);
      }

    case InterruptType::BigMotion:
      {
      }

    case InterruptType::Step:
      {
      }

    case InterruptType::AngleChange:
      {
      }

    case InterruptType::None:
      {
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