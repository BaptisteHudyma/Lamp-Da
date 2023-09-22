#include "button.h"

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
  return cycles > 150;
}

void treat_button_pressed(const bool isButtonPressDetected, std::function<void(uint8_t, uint32_t)> callback)
{
  static uint8_t clickedEvents = 0;    // multiple clicks
  static uint32_t buttonHoldStart= 0;  // start of the hold event in millis, valid if clicked event is > 0

  static uint32_t lastButtonPressedTime = 0;  // last time the pressed event was detected
  static bool isButtonPressed = false;
  
  const uint32_t currentMillis = millis();
  const uint32_t sinceLastCall = currentMillis - lastButtonPressedTime;
  // remove button clicked if last call was too long ago
  if(sinceLastCall > 250)
  {
    // end of button press, change event
    callback(clickedEvents, currentMillis - buttonHoldStart);

    // reset
    clickedEvents = 0;
    isButtonPressed = false;
    buttonHoldStart = 0;
  }

  // set button high
  if (isButtonPressDetected)
  {
    // small delay since button press
    if (sinceLastCall > 50)
    {
        clickedEvents += 1;
        buttonHoldStart = currentMillis;
    }
    
    lastButtonPressedTime = currentMillis;
    isButtonPressed = true;
  }
}

void handle_button_events(std::function<void(uint8_t, uint32_t)> callback)
{
  // check the button pressed status
  const bool isButtonPressed = check_button_state();
  treat_button_pressed(isButtonPressed, callback);
}
