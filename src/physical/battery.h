#ifndef BATTERY_H
#define BATTERY_H

#include "../utils/constants.h"
#include "../utils/utils.h"

// return a number between 0 and 100
inline uint8_t get_battery_level(const bool resetRead)
{
   constexpr float maxVoltage = 16.6;
   constexpr float lowVoltage = 13.0;

   static float lastValue = 0;

   // map the input ADC out to voltage reading.
   constexpr float minInValue = 472.0;
   constexpr float maxInValue = 600.0;
   const float batteryVoltage = utils::map(analogRead(BATTERY_CHARGE_PIN), minInValue, maxInValue, lowVoltage, maxVoltage);

   // init or reset
   if(lastValue == 0 or resetRead)
   {
     lastValue = batteryVoltage;
   }

   lastValue = batteryVoltage * 0.01 + lastValue * 0.99;
   return utils::map(lastValue, lowVoltage, maxVoltage, 0, 100);
}


#endif