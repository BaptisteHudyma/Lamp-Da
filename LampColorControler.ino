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
    }

    lastIsActivatedValue = isActivated;
  }
}

bool ledState = false;
void loop() {
  handle_button_events(button_clicked_callback, button_hold_callback);


  static uint32_t lastCall = 0;

  if(not ledState and millis() - lastCall > 100)
  {
    ledState = true;
    digitalWrite(LED_BUILTIN,HIGH);
    lastCall = millis();
  }
  if(ledState and millis() - lastCall > 2000)
  {
    ledState = false;
    digitalWrite(LED_BUILTIN, LOW);
    lastCall = millis();
  }

  handlePowerToggle();

  if (isActivated)
  {
#ifdef USE_BLUETOOTH
    bluetooth::parse_messages();
#endif
    color_mode_update();
  }
}
