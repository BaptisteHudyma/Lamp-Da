#include "power_source.h"

#include <Arduino.h>

#include "PDlib/usb_pd_driver.h"
#include "Wire.h"

#include "PDlib/tcpm_driver.h"
#include "PDlib/usb_pd.h"

// we only have one device, so always index 0
static constexpr int devicePort = 0;
// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
        {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};
// USB-C Specific - TCPM end 1

namespace powerSource {

bool writeI2cData(const uint8_t deviceAddr, const uint8_t registerAdd, const uint8_t size, uint8_t* buf)
{
  Wire.beginTransmission(deviceAddr);
  Wire.write(registerAdd);
  uint8_t count = size;
  while (count > 0)
  {
    Wire.write(*buf++);
    count--;
  }
  Wire.endTransmission();
  return true;
}

bool readI2cData(const uint8_t deviceAddr, const uint8_t registerAdd, const uint8_t size, uint8_t* buf)
{
  Wire.beginTransmission(deviceAddr);
  Wire.write(registerAdd);
  Wire.endTransmission();
  Wire.requestFrom(deviceAddr, size);
  uint8_t count = size;
  while (Wire.available() && count > 0)
  {
    *buf++ = Wire.read();
    count--;
  }
  return count == 0;
}

// set to true when a power source is detected
inline static bool isPowerSourceDetected_s = false;
// set to the time when the source is detected
inline static uint32_t powerSourceDetectedTime_s = 0;

// check if the charger can use the max power
bool can_use_PD_full_power()
{
  // voltage on VBUS is greater than (negociated voltage minus a threshold)
  return get_available_pd_voltage_mV() > 0 &&
         get_available_pd_current_mA() > 0; // and get_vbus_voltage(devicePort) >= availableVoltage - 2000;
}

// check if the source had time to stabilize
bool is_vbus_stable()
{
  return isPowerSourceDetected_s and
         // let some time pass after a new detection
         millis() - powerSourceDetectedTime_s > 1500;
}

bool interruptSet = false;
void ic_interrupt() { interruptSet = true; }

bool is_usb_pd() { return get_pd_source_cnt() != 0; }

/**
 *
 *      HEADER FUNCTIONS BELOW
 *
 */

bool setup()
{
  // 0 is success
  const bool initSucceeded = (tcpm_init(devicePort) == 0);
  delay(50);
  pd_init(devicePort);
  delay(50);

  pinMode(CHARGE_INT,
          INPUT_PULLUP_SENSE); // Set FUSB302 int pin input ant pull up
  attachInterrupt(digitalPinToInterrupt(CHARGE_INT), ic_interrupt, CHANGE);

  return initSucceeded;
}

int reset = 1;
void loop()
{
  // handle alerts
  if (interruptSet)
  {
    interruptSet = false;
    tcpc_alert(devicePort);
  }
  pd_run_state_machine(devicePort, reset);
  reset = 0;

  /*
    Serial.print(is_usb_pd());
    Serial.print(pd_is_vbus_present(devicePort));
    Serial.print(" ");
    Serial.print(get_available_pd_current_mA());
    Serial.print("mA --  ");
    Serial.print(get_available_pd_voltage_mV());
    Serial.print("mV --  ");
    Serial.println(get_state_cstr(devicePort));
    */

  // standard code

  static uint32_t lastVbusValid = 0;

  // source detected
  const uint32_t time = millis();
  if (pd_is_vbus_present(devicePort))
  {
    if (not isPowerSourceDetected_s)
    {
      powerSourceDetectedTime_s = time;
      isPowerSourceDetected_s = true;
    }
    lastVbusValid = time;
  }
  // deglitch time
  else if (isPowerSourceDetected_s and
           // plugged in power can flicker
           time - powerSourceDetectedTime_s > 100 and
           // vbus can become briefly invalid when using the ICO algorithm
           time - lastVbusValid > 500)
  {
    isPowerSourceDetected_s = false;
    reset = 1;
    reset_cache();
  }
}

uint16_t get_max_input_current()
{
  // vbus had time to stabilize
  if (is_vbus_stable())
  {
    if (is_not_usb_power_delivery())
    {
      // maximum USB current
      return 1500;
    }
    // power is available
    else
    {
      /*const auto ma = get_next_pdo_amps();
      const auto mv = get_next_pdo_voltage();
      if (ma > 0 && mv > 0)
      {
        Serial.print("- ");
        Serial.print(ma);
        Serial.print("ma, ");
        Serial.print(mv);
        Serial.println("mv");
      }
      else
      {
        Serial.println("");
      }*/

      if (can_use_PD_full_power())
      {
        // do not use the whole current capabilities, or the source will cut us off
        return get_available_pd_current_mA() * 0.90;
      }
    }
  }
  // max USB standard current
  return 100;
}

bool is_not_usb_power_delivery()
{
  // TODO : this is a hack:
  // the real condition should be "we sent messages and got not answers after N tries"
  // not power delivery is not power sources offered
  return not is_usb_pd();
}

bool is_power_available() { return isPowerSourceDetected_s; }

bool is_powered_with_vbus() { return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0x00; }

} // namespace powerSource