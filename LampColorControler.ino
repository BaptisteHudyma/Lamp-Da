#include "src/behavior.h"
#include "src/physical/MicroPhone.h"
#include "src/physical/button.h"
#include "src/physical/bluetooth.h"

//#define USE_BLUETOOTH

void setup()
{
  Serial.begin(115200);
 
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();   // Turn OFF all pixels ASAP
  strip.setBrightness(50);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LED_POWER_PIN, OUTPUT);
  pinMode(BATTERY_CHARGE_PIN, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);

#ifdef USE_BLUETOOTH
  bluetooth::startup_sequence();
#endif
}


void handlePowerToggle()
{
  // used to detected power on/off events
  static bool lastIsActivatedValue = isActivated;
  if (isActivated != lastIsActivatedValue)
  {
    // powered on !
    if (isActivated)
    {
      digitalWrite(LED_POWER_PIN, HIGH);

      // activate microphone readings
      sound::enable_microphone(16000);
#ifdef USE_BLUETOOTH
      bluetooth::enable_bluetooth();
#endif
    }
    // powered off !
    else
    {
      // remove all colors from strip
      strip.clear();
      strip.show();  //  Update strip to match

      // deactivate strip power
      digitalWrite(LED_POWER_PIN, LOW);

      // disable bluetooth and microphone
      sound::disable_microphone();
#ifdef USE_BLUETOOTH
      bluetooth::disable_bluetooth();
#endif

      // sleep mode, wake up on tactile interrupt
      /*
      nrf_gpio_cfg_sense_input(digitalPinToInterrupt(BUTTON_PIN), NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_SENSE_HIGH);
      delay(2000);
      NRF_POWER->SYSTEMOFF = 1;
      */
    }

    lastIsActivatedValue = isActivated;
  }
}


void blink_led(const uint toggleFreq)
{
  static uint32_t lastCall = 0;
  static bool ledState = false;

  // led is off, and last call was 100ms before
  if(not ledState and millis() - lastCall > 100)
  {
    ledState = true;
    digitalWrite(LED_BUILTIN, HIGH);
    lastCall = millis();
  }

  // led is on, and last call was long ago 
  if(ledState and millis() - lastCall > toggleFreq)
  {
    ledState = false;
    digitalWrite(LED_BUILTIN, LOW);
    lastCall = millis();
  }
}

void loop() {
  handle_button_events(button_clicked_callback, button_hold_callback);

  handlePowerToggle();

  if (isActivated)
  {
#ifdef USE_BLUETOOTH
    bluetooth::parse_messages();
#endif
    color_mode_update();

    // fast link
    blink_led(2000);
  }
  else
  {
    // blink slowwwwwly
    blink_led(10000);
  }
}
