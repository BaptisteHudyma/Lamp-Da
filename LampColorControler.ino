#include "src/behavior.h"
#include "src/physical/MicroPhone.h"
#include "src/physical/button.h"
#include "src/physical/bluetooth.h"
#include "src/physical/fileSystem.h"
#include "src/utils/utils.h"

#include <bluefruit.h>

//#define USE_BLUETOOTH

void setup()
{
  Serial.begin(115200);
  
  // Necessary for sleep mode for some reason ??
  Bluefruit.begin(1);
  
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();  // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();   // Turn OFF all pixels ASAP
  strip.setBrightness(50);

  // attach the button interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP_SENSE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), button_state_interrupt, CHANGE);
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_POWER_PIN, OUTPUT);
  pinMode(BATTERY_CHARGE_PIN, INPUT);

  pinMode(BUTTON_RED, OUTPUT);
  pinMode(BUTTON_GREEN, OUTPUT);
  pinMode(BUTTON_BLUE, OUTPUT);
  digitalWrite(BUTTON_RED, LOW);
  digitalWrite(BUTTON_GREEN, LOW);
  digitalWrite(BUTTON_BLUE, LOW);

  // power up the system
  digitalWrite(LED_POWER_PIN, HIGH);

  // activate microphone readings
  sound::enable_microphone(16000);
#ifdef USE_BLUETOOTH
  bluetooth::enable_bluetooth();
  bluetooth::startup_sequence();
#endif

  // start the file system
  fileSystem::setup();

  // initialize the battery level
  get_battery_level(true);

  // read from the stored config
  read_parameters();
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

void display_battery_level()
{
  static constexpr uint32_t refreshRate_ms = 2000;
  static uint32_t lastCall = 0;

  const uint32_t newCall = millis();
  if(newCall - lastCall > refreshRate_ms or lastCall == 0)
  {
    lastCall = newCall;
    const double percent = get_battery_level(false) / 100.0;
    //Serial.println(percent);
    // red to green
    // force green to be kind of low because red is not as powerfull
    set_button_color(utils::ColorSpace::RGB(utils::get_gradient(utils::ColorSpace::RGB(255, 0, 0).get_rgb().color, utils::ColorSpace::RGB(0,30,0).get_rgb().color, percent)));
  }
}

void loop() {
  uint32_t start = millis();
  display_battery_level();

  // loop is not ran in shutdown mode
  handle_button_events(button_clicked_callback, button_hold_callback);

#ifdef USE_BLUETOOTH
  bluetooth::parse_messages();
#endif
  color_mode_update();

  // fast link
  blink_led(500);

  strip.show(); // show at the end of the loop

  // wait for a delay if we are faster than the set refresh rate
  uint32_t stop = millis();
  const uint32_t loopDuration = (stop - start);
  if(loopDuration < LOOP_UPDATE_PERIOD)
  {
    delay(LOOP_UPDATE_PERIOD - loopDuration);
  }

  stop = millis();
  // debug the loop update period
  //Serial.println(stop - start);
}
