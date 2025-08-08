#ifndef USER_CONSTANTS_H
#define USER_CONSTANTS_H

#include <cstdint>
#include <cmath>

//
// common to all lamp type
//

// does button usermode defaults to "lockdown" mode?
static constexpr bool usermodeDefaultsToLockdown = false;

static constexpr float c_PI = 3.1415926535897;
static constexpr float c_TWO_PI = 6.2831853071795;
static constexpr float c_HALF_PI = 1.5707963267948;

static constexpr float c_degreesToRadians = c_PI / 180.0f;

//
// simple lamp type
//

#ifdef LMBD_LAMP_TYPE__SIMPLE

// Update when the soft changes version
#define USER_SOFTWARE_VERSION_MAJOR 1
#define USER_SOFTWARE_VERSION_MINOR 0

// parameters of the led strip used
static constexpr float consWattByMeter = 12;            // power consumption (in Watt/meters)
static constexpr float inputVoltage_V = 12;             // voltage (volts)
static constexpr float ledStripLenght_mm = 91.0 * 25.0; // 91 sections of 25 mm

// define position of led 0 to the circuit center
static constexpr float circuitToLedZeroRotationX_degrees = 0;
static constexpr float circuitToLedZeroRotationY_degrees = 0;
static constexpr float circuitToLedZeroRotationZ_degrees = 88;

#endif // LMBD_LAMP_TYPE__SIMPLE

//
// cct lamp type
//

#ifdef LMBD_LAMP_TYPE__CCT

// Update when the soft changes version
#define USER_SOFTWARE_VERSION_MAJOR 0
#define USER_SOFTWARE_VERSION_MINOR 1

// parameters of the led strip used
static constexpr float consWattByMeter = 10;            // power consumption (in Watt/meters)
static constexpr float inputVoltage_V = 12;             // voltage (volts)
static constexpr float ledStripLenght_mm = 67.0 * 27.0; // 67 sections of 27 mm

// define position of led 0 to the circuit center
static constexpr float circuitToLedZeroRotationX_degrees = 0;
static constexpr float circuitToLedZeroRotationY_degrees = 0;
static constexpr float circuitToLedZeroRotationZ_degrees = 88;

#endif // LMBD_LAMP_TYPE__CCT

//
// indexable lamp type
//

#ifdef LMBD_LAMP_TYPE__INDEXABLE

// Update when the soft changes version
#define USER_SOFTWARE_VERSION_MAJOR 1
#define USER_SOFTWARE_VERSION_MINOR 1

static constexpr float lampBodyRadius_mm = 25; // external radius of the lamp body

// parameters of the led strip used
static constexpr uint16_t LED_COUNT = 570;     // How many indexable leds are attached to the controler
static constexpr float consWattByMeter = 5;    // power consumption (in Watt/meters)
static constexpr float inputVoltage_V = 12;    // voltage (volts)
static constexpr float ledByMeter = 162.6;     // the REAL indexable led by meters (for a 160Led/m)
static constexpr float ledStripWidth_mm = 5.2; // width of the led strip
static constexpr float ledStripHeigh_mm = 0.7; // heigh of the led strip (calibrated for this strip)

// physical parameters computations
static constexpr float ledSize_mm = 1.0 / ledByMeter * 1000.0;                   // size of the individual led
static constexpr float lampBodyCircumpherence_mm = c_TWO_PI * lampBodyRadius_mm; // external circumpherence
static constexpr float ledStripLenght_mm = LED_COUNT * ledSize_mm;

// led per tube circumpherence
static constexpr float ledPerTurn = lampBodyCircumpherence_mm / ledSize_mm;
// estimate of the lamp height
static constexpr float lampHeight = ledStripWidth_mm * LED_COUNT / ledPerTurn;

static constexpr float stripXCoordinates = lampBodyCircumpherence_mm / ledSize_mm;
static constexpr float stripYCoordinates = ledStripLenght_mm / lampBodyCircumpherence_mm;
static constexpr float lampBodyHeight_mm = stripYCoordinates * ledStripWidth_mm;

constexpr uint16_t stripMatrixWidth = ceil(stripXCoordinates);
constexpr uint16_t stripMatrixHeight = ceil(stripYCoordinates);

// define position of led 0 to the circuit center
static constexpr float circuitToLedZeroRotationX_degrees = 0;
static constexpr float circuitToLedZeroRotationY_degrees = 0;
static constexpr float circuitToLedZeroRotationZ_degrees = 88;

#endif // LMBD_LAMP_TYPE__INDEXABLE

//
// add your own lamp type below :)
//

#endif
