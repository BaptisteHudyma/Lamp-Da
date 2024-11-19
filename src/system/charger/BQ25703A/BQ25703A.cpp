/**************************************************************************/
/*!
  @file     Arduino_BQ25703A.cpp
  @author   Lorro

  Update by Baptiste Hudyma: 2024

  Library for basic interfacing with BQ25703A battery management IC from TI
*/
/**************************************************************************/
#include "BQ25703A.h"

#include <Wire.h>

#include "Arduino.h"

namespace bq2573a {

// Prints a binary number with leading zeros (Automatic Handling)
#define PRINTBIN(Num)                                             \
  for (uint32_t t = (1UL << ((sizeof(Num) * 8) - 1)); t; t >>= 1) \
    Serial.write(Num& t ? '1' : '0');

BQ25703A::BQ25703A() {}

// I2C functions below here
//------------------------------------------------------------------------

boolean BQ25703A::readDataReg(const byte regAddress, byte* dataVal, const uint8_t arrLen)
{
  Wire.beginTransmission(BQ25703Aaddr);
  if (!Wire.write(regAddress))
    return false;

  byte ack = Wire.endTransmission();
  if (ack == 0)
  {
    Wire.requestFrom((int)BQ25703Aaddr, (int)(arrLen + 1));
    if (Wire.available() > 0)
    {
      for (uint8_t i = 0; i < arrLen; i++)
      {
        dataVal[i] = Wire.read();
      }
    }
    return true;
  }
  else
  {
    return false; // if I2C comm fails
  }
}

boolean BQ25703A::writeDataReg(const byte regAddress, byte dataVal0, byte dataVal1)
{
  Wire.beginTransmission(BQ25703Aaddr);
  if (!Wire.write(regAddress))
    return false;
  if (!Wire.write(dataVal0))
    return false;
  if (!Wire.write(dataVal1))
    return false;
  // succes ?
  return Wire.endTransmission() == 0;
}

// boolean BQ25703A::read2ByteReg( byte regAddress, byte *val0, byte *val1 ){
//
//   Wire.beginTransmission( BQ25703Aaddr );
//   Wire.write( regAddress );
//   byte ack = Wire.endTransmission();
//   if( ack == 0 ){
//     Wire.requestFrom( ( int )BQ25703Aaddr , 2 );
//     if( Wire.available() > 0 ){
//       val0 = Wire.receive();
//       val0 = Wire.receive();
//     }
//     return true;
//   }else{
//     return false; //if I2C comm fails
//   }
// }

} // namespace bq2573a