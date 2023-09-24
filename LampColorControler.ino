#include "behavior.h"
#include "MicroPhone.h"
#include "button.h"


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

  sound::init_microphone(16000);
}

void loop() {
  handle_button_events(button_clicked_callback, button_hold_callback);

  if (!isActivated)
  {
    strip.clear();
    strip.show();  //  Update strip to match

    // deactivate strip power
  }
  else
    color_mode_update();
}
