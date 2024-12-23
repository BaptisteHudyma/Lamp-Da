#include "power_source.h"

#include <Arduino.h>

#include "Wire.h"

#include "PDlib/drivers/usb_pd_driver.h"
#include "PDlib/drivers/tcpm_driver.h"
#include "PDlib/usb_pd.h"

// we only have one device, so always index 0
static constexpr int devicePort = 0;
// USB-C Specific - TCPM start 1
const struct tcpc_config_t tcpc_config[CONFIG_USB_PD_PORT_COUNT] = {
        {0, fusb302_I2C_SLAVE_ADDR, &fusb302_tcpm_drv},
};
// USB-C Specific - TCPM end 1

static bool canUseSourcePower_s = false;

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

int get_vbus_voltage()
{
  static int vbusVoltage = false;
  static uint32_t time = 0;
  // do not spam the system
  if (time == 0 or millis() - time > 100)
  {
    time = millis();
    vbusVoltage = tcpm_get_vbus_voltage(devicePort);
  }
  return vbusVoltage;
}

// check if the charger can use the max power
bool can_use_PD_full_power()
{
  // voltage on VBUS is greater than (negociated voltage minus a threshold)
  return get_available_pd_voltage_mV() > 0 and
         // the algo confirms that we are a sink
         is_sink_ready(devicePort) and
         // the voltage/current rise should be over
         get_vbus_voltage() >= get_available_pd_voltage_mV() - 1000;
}

bool is_power_cable_detected() { return is_power_cable_connected(); }

uint32_t lastPDdetected = 0;
bool is_usb_pd()
{
  const bool isPd = is_pd_conector();
  if (isPd)
  {
    lastPDdetected = millis();
  }
  return isPd;
}

// check if the source is simple USB, with a stabilize delay
bool is_standard_port()
{
  return isPowerSourceDetected_s and
         // let some time pass after a new detection
         millis() - powerSourceDetectedTime_s > 1500 and
         // last pd detected was some time ago
         millis() - lastPDdetected > 1000;
}

bool interruptSet = false;
void ic_interrupt() { interruptSet = true; }

bool is_vbus_powered()
{
  static bool isVbusPresent = false;
  static uint32_t time = 0;
  // do not spam the system
  if (time == 0 or millis() - time > 500)
  {
    time = millis();
    isVbusPresent = pd_is_vbus_present(devicePort);
  }
  return isVbusPresent;
}

struct UsbPDData
{
  bool isPowerCableDetected;
  bool isVbusPowered;
  bool isPowerSourceDetected;
  bool isUsbPd;
  uint16_t vbusVoltage;

  uint16_t maxInputCurrent;
  uint16_t maxInputVoltage;

  char* pdAlgoStatus;

  // when true, this struct has changed !
  bool hasChanged = false;

  void update()
  {
    const bool newIsCableDetected = is_power_cable_detected();
    if (newIsCableDetected != isPowerCableDetected)
    {
      hasChanged = true;
      isPowerCableDetected = newIsCableDetected;
    }

    const bool newisVbusPowered = is_vbus_powered();
    if (newisVbusPowered != isVbusPowered)
    {
      hasChanged = true;
      isVbusPowered = newisVbusPowered;
    }

    const bool cc = isPowerSourceDetected_s;
    if (cc != isPowerSourceDetected)
    {
      hasChanged = true;
      isPowerSourceDetected = isPowerSourceDetected_s;
    }

    const bool newisUsbPd = is_usb_pd();
    if (newisUsbPd != isUsbPd)
    {
      hasChanged = true;
      isUsbPd = newisUsbPd;
    }

    uint16_t newvbusVoltage = get_vbus_voltage();
    if (newvbusVoltage != vbusVoltage)
    {
      // hasChanged = true;
      vbusVoltage = newvbusVoltage;
    }

    uint16_t newmaxInputCurrent = get_available_pd_current_mA();
    if (newmaxInputCurrent != maxInputCurrent)
    {
      hasChanged = true;
      maxInputCurrent = newmaxInputCurrent;
    }

    uint16_t newmaxInputVoltage = get_available_pd_voltage_mV();
    if (newmaxInputVoltage != maxInputVoltage)
    {
      hasChanged = true;
      maxInputVoltage = newmaxInputVoltage;
    }

    char* newStatus = get_state_cstr(devicePort);
    if (newStatus != pdAlgoStatus)
    {
      hasChanged = true;
      pdAlgoStatus = newStatus;
    }
  }

  void serial_show()
  {
    if (hasChanged)
    {
      Serial.print(millis());
      Serial.print(": ");
      Serial.print(isPowerCableDetected);
      Serial.print(isVbusPowered);
      Serial.print(isPowerSourceDetected);
      Serial.print(isUsbPd);
      Serial.print(": ");
      // Serial.print(vbusVoltage / 1000.0);
      // Serial.print("V, ");
      Serial.println(pdAlgoStatus);

      hasChanged = false;
    }
  }
};
UsbPDData data;
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

int reset = 0;
void loop()
{
  //  handle alerts
  if (interruptSet)
  {
    interruptSet = false;
    tcpc_alert(devicePort);
  }
  pd_run_state_machine(devicePort, reset);
  reset = 0;

  data.update();
  data.serial_show();

  // standard code

  static uint32_t lastVbusValid = 0;

  // source detected
  const uint32_t time = millis();
  if (is_vbus_powered())
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

  if (
          // valid input source
          isPowerSourceDetected_s and
                  // is valid source (pd with power, or non pd)
                  (is_usb_pd() and can_use_PD_full_power()) or
          // is standard usb c
          (is_standard_port()))
  {
    canUseSourcePower_s = true;
  }
  else
  {
    // no deglitch, this needs to be reactive
    canUseSourcePower_s = false;
  }
}

void shutdown()
{
  // nothing ?
}

uint16_t get_max_input_current()
{
  // safety
  if (!isPowerSourceDetected_s)
    return 0;

  // power delivery detected
  if (is_usb_pd())
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
  // no usb pd since some time, and vbus seems stable so try to use it
  else if (is_standard_port())
  {
    // maximum USB current
    return 1500;
  }
  // we dont known for now the type of connection
  return 0;
}

bool is_power_available() { return isPowerSourceDetected_s; }

bool can_use_power() { return canUseSourcePower_s; }

bool is_powered_with_vbus() { return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0x00; }

OTGParameters get_otg_parameters()
{
  const auto& otg = get_OTG_requested_parameters();
  OTGParameters tmp;
  tmp.requestedCurrent_mA = otg.requestedCurrent_mA;
  tmp.requestedVoltage_mV = otg.requestedVoltage_mV;
  return tmp;
}

} // namespace powerSource