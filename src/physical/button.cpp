#include "button.h"

#include "../utils/constants.h"

#define RELEASE_TIMING_MS 200

bool check_button_state()
{
  //empty capacitor
  pinMode(BUTTON_PIN, OUTPUT);
  digitalWrite(BUTTON_PIN, LOW);
  delayMicroseconds(1000);

  pinMode(BUTTON_PIN, INPUT);
  uint8_t cycles;
  for(cycles = 0; cycles < 255; cycles++) {
      if(digitalRead(BUTTON_PIN) == HIGH)
          break;
  }
  // 150/255 is a good threshold for noise
  return cycles > 50;
}

void treat_button_pressed(const bool isButtonPressDetected, std::function<void(uint8_t)> clickSerieCallback, std::function<void(uint8_t, uint32_t)> clickHoldSerieCallback)
{
  static uint8_t clickedEvents = 0;    // multiple clicks
  static uint32_t buttonHoldStart= 0;  // start of the hold event in millis, valid if clicked event is > 0

  static uint32_t lastButtonPressedTime = 0;  // last time the pressed event was detected
  static bool isButtonPressed = false;
  
  const uint32_t currentMillis = millis();
  const uint32_t sinceLastCall = currentMillis - lastButtonPressedTime;
  const uint32_t buttonPressedDuration = currentMillis - buttonHoldStart;

  // currently in long press status
  const bool isInLongPress = isButtonPressed and buttonPressedDuration > HOLD_BUTTON_MIN_MS;

  // remove button clicked if last call was too long ago
  if(sinceLastCall > RELEASE_TIMING_MS or (isInLongPress && sinceLastCall > RELEASE_TIMING_MS/2))
  {
    // end of button press, change event
    if (isInLongPress)
      // signal end of click and hold
      clickHoldSerieCallback(clickedEvents, 0);
    else
      clickSerieCallback(clickedEvents);

    // reset
    clickedEvents = 0;
    isButtonPressed = false;
    buttonHoldStart = currentMillis;
  }

  // set button high
  if (isButtonPressDetected)
  {
    // small delay since button press
    if (sinceLastCall > 50)
    {
      if (!isInLongPress)
      {
        clickedEvents += 1;
        buttonHoldStart = currentMillis;
      }
    }
    
    lastButtonPressedTime = currentMillis;
    isButtonPressed = true;
  }

  
  if(isInLongPress)
  {
    clickHoldSerieCallback(clickedEvents, buttonPressedDuration);
  }
}

void handle_button_events(std::function<void(uint8_t)> clickSerieCallback, std::function<void(uint8_t, uint32_t)> clickHoldSerieCallback)
{
  // check the button pressed status
  const bool isButtonPressed = check_button_state();
  treat_button_pressed(isButtonPressed, clickSerieCallback, clickHoldSerieCallback);
}


float get_battery_level()
{
   constexpr float maxVoltage = 16.5;
   constexpr float lowVoltage = 13.0;

   static float lastValue = 0;

   // map the input ADC out to voltage reading.
   constexpr float maxInValue = 870;
   const float pinMeasureVoltage = analogRead(BATTERY_CHARGE_PIN) / maxInValue * maxVoltage;
   const float batteryVoltage = (1.0 - (maxVoltage - pinMeasureVoltage) / (maxVoltage - lowVoltage)) * 100.0;

   lastValue = batteryVoltage * 0.01 + lastValue * 0.99;
   return batteryVoltage;
}