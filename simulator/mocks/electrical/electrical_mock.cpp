
#include "src/system/platform/gpio.h"
#include "src/system/platform/print.h"
#include "src/system/platform/time.h"

#include "src/system/utils/input_output.h"

#include "simulator/include/hardware_influencer.h"

#include <thread>
#include <atomic>

namespace __private {
DigitalPin enableVbusGate(DigitalPin::GPIO::Output_EnableVbusGate);
DigitalPin enablePowerGate(DigitalPin::GPIO::Output_EnableOutputGate);

DigitalPin dischargeVbus(DigitalPin::GPIO::Output_DischargeVbus);
DigitalPin vbusDirection(DigitalPin::GPIO::Output_VbusDirection);
DigitalPin fastRoleSwap(DigitalPin::GPIO::Output_VbusFastRoleSwap);
} // namespace __private

std::atomic<bool> canRunElectricalSimuThread = false;
std::thread electricalSimuThread;

bool is_output_enabled() { return __private::enablePowerGate.is_high(); }

void start_electrical_mock()
{
  mock_electrical::powerRailVoltage = 0;
  mock_electrical::vbusVoltage = 0;
  mock_electrical::outputVoltage = 0;

  canRunElectricalSimuThread = true;
  electricalSimuThread = std::thread([&]() {
    while (canRunElectricalSimuThread)
    {
      // vbus gate propagate voltage/current
      if (__private::enableVbusGate.is_high())
      {
        // power can flow from output to vbus
        if (__private::vbusDirection.is_high())
        {
          mock_electrical::vbusVoltage =
                  max<float>(mock_electrical::powerRailVoltage, mock_electrical::inputVbusVoltage);
        }

        // max of the two voltages
        mock_electrical::powerRailVoltage =
                std::max<float>(mock_electrical::chargeOtgOutput, mock_electrical::inputVbusVoltage);
      }
      else
      {
        // gate open, no flow
        mock_electrical::vbusVoltage = max<float>(0.0f, mock_electrical::inputVbusVoltage);
        mock_electrical::powerRailVoltage = mock_electrical::chargeOtgOutput;
      }

      // Output power gate
      if (__private::enablePowerGate.is_high())
      {
        // power can go to output
        mock_electrical::outputVoltage = max<float>(0.0f, mock_electrical::powerRailVoltage);
      }
      else
      {
        mock_electrical::outputVoltage = 0;
      }

      delay_ms(1);
    }
  });
}

void stop_electrical_mock() { canRunElectricalSimuThread = false; }
