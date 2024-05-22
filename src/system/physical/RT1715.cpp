#include "RT1715.h"

#include <Wire.h>

#include "Arduino.h"

namespace rt1715 {

// Prints a binary number with leading zeros (Automatic Handling)
#define PRINTBIN(Num)                                             \
  for (uint32_t t = (1UL << ((sizeof(Num) * 8) - 1)); t; t >>= 1) \
    Serial.write(Num &t ? '1' : '0');

RT1715::RT1715() {}

// I2C functions below here
//------------------------------------------------------------------------

bool RT1715::readDataReg(const byte regAddress, byte *dataVal,
                         const uint8_t arrLen) {
  Wire.beginTransmission(RT1715addr);
  if (!Wire.write(regAddress)) return false;

  byte ack = Wire.endTransmission();
  if (ack == 0) {
    Wire.requestFrom((int)RT1715addr, (int)(arrLen + 1));
    if (Wire.available() > 0) {
      for (uint8_t i = 0; i < arrLen; i++) {
        dataVal[i] = Wire.read();
      }
    }
    return true;
  } else {
    return false;  // if I2C comm fails
  }
}

bool RT1715::writeDataReg(const byte regAddress, byte dataVal0, byte dataVal1) {
  Wire.beginTransmission(RT1715addr);
  if (!Wire.write(regAddress)) return false;
  if (!Wire.write(dataVal0)) return false;
  if (!Wire.write(dataVal1)) return false;
  byte ack = Wire.endTransmission();
  if (ack == 0) {
    return true;
  } else {
    return false;  // if I2C comm fails
  }
}

}  // namespace rt1715