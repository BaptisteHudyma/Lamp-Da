/*! \file electrical_mock.cpp
    \brief General electrical simulation of the Lampda board.
*/

#include "src/system/hal/gpio.h"
#include "src/system/hal/print.h"
#include "src/system/hal/time.h"
#include "src/system/hal/threads.h"

#include "src/system/utils/input_output.h"
#include "src/system/utils/utils.h"

#include "simulator/include/hardware_influencer.h"

#include <thread>
#include <atomic>

namespace simulator {

namespace __private {
::lampda::hal::gpio::DigitalPin enableVbusGate(::lampda::hal::gpio::DigitalPin::GPIO::Output_EnableVbusGate);
::lampda::hal::gpio::DigitalPin enablePowerGate(::lampda::hal::gpio::DigitalPin::GPIO::Output_EnableOutputGate);

::lampda::hal::gpio::DigitalPin dischargeVbus(::lampda::hal::gpio::DigitalPin::GPIO::Output_DischargeVbus);
::lampda::hal::gpio::DigitalPin vbusDirection(::lampda::hal::gpio::DigitalPin::GPIO::Output_VbusDirection);
::lampda::hal::gpio::DigitalPin fastRoleSwap(::lampda::hal::gpio::DigitalPin::GPIO::Output_VbusFastRoleSwap);
} // namespace __private

bool is_output_enabled() { return __private::enablePowerGate.is_high(); }

void elec_mock_loop()
{
  // vbus gate propagate voltage/current
  if (__private::enableVbusGate.is_high())
  {
    // power can flow from output to vbus
    if (__private::vbusDirection.is_high())
    {
      mock_electrical::vbusVoltage =
              std::max<float>(mock_electrical::powerRailVoltage, mock_electrical::inputVbusVoltage);
    }

    // max of the two voltages
    mock_electrical::powerRailVoltage =
            std::max<float>(mock_electrical::chargeOtgOutput, mock_electrical::inputVbusVoltage);
  }
  else
  {
    // gate open, no flow
    mock_electrical::vbusVoltage = std::max<float>(0.0f, mock_electrical::inputVbusVoltage);
    mock_electrical::powerRailVoltage = mock_electrical::chargeOtgOutput;
  }

  // Output power gate
  if (__private::enablePowerGate.is_high())
  {
    // power can go to output
    mock_electrical::outputVoltage = std::max<float>(0.0f, mock_electrical::powerRailVoltage);
  }
  else
  {
    mock_electrical::outputVoltage = 0;
  }

  ::lampda::hal::delay_ms(1);
}

void start_electrical_mock()
{
  mock_electrical::powerRailVoltage = 0;
  mock_electrical::vbusVoltage = 0;
  mock_electrical::outputVoltage = 0;

  lampda::hal::threads::start_thread(elec_mock_loop, lampda::utils::hash("elec_mock"), 0, 255);
}

void stop_electrical_mock() {}

} // namespace simulator
