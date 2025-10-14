#ifndef USER_CONSTANTS_H
#define USER_CONSTANTS_H

#include <cstdint>
#include <cmath>

//
// common to all lamp type
//

// does button usermode defaults to "lockdown" mode?
static constexpr bool usermodeDefaultsToLockdown = false;

static constexpr float c_PI = 3.1415926535897f;
static constexpr float c_TWO_PI = 6.2831853071795f;
static constexpr float c_HALF_PI = 1.5707963267948f;

static constexpr float c_degreesToRadians = c_PI / 180.0f;

// Update when the soft changes version
static constexpr uint8_t USER_SOFTWARE_VERSION_MAJOR = 1;
static constexpr uint8_t USER_SOFTWARE_VERSION_MINOR = 6;

//
// simple lamp type
//

#ifdef LMBD_LAMP_TYPE__SIMPLE

// compute the expected average loop runtime (in ms)
// defined as milliseconds / FPS
static constexpr uint32_t MAIN_LOOP_UPDATE_PERIOD_MS = static_cast<uint32_t>(1000 / 80.0f);

// parameters of the led strip used
static constexpr float consWattByMeter = 12;              // power consumption (in Watt/meters)
static constexpr uint16_t stripInputVoltage_mV = 12000;   // voltage
static constexpr float ledStripLenght_mm = 91.0f * 25.0f; // 91 sections of 25 mm

// define position of led 0 to the circuit center
static constexpr float circuitToLedZeroRotationX_degrees = 0.0f;
static constexpr float circuitToLedZeroRotationY_degrees = 0.0f;
static constexpr float circuitToLedZeroRotationZ_degrees = 88.0f;

#endif // LMBD_LAMP_TYPE__SIMPLE

//
// cct lamp type
//

#ifdef LMBD_LAMP_TYPE__CCT

// compute the expected average loop runtime (in ms)
// defined as milliseconds / FPS
static constexpr uint32_t MAIN_LOOP_UPDATE_PERIOD_MS = static_cast<uint32_t>(1000 / 80.0f);

// parameters of the led strip used
static constexpr float consWattByMeter = 10;              // power consumption (in Watt/meters)
static constexpr uint16_t stripInputVoltage_mV = 12000;   // voltage
static constexpr float ledStripLenght_mm = 67.0f * 27.0f; // 67 sections of 27 mm

// define position of led 0 to the circuit center
static constexpr float circuitToLedZeroRotationX_degrees = 0.0f;
static constexpr float circuitToLedZeroRotationY_degrees = 0.0f;
static constexpr float circuitToLedZeroRotationZ_degrees = 88.0f;

#endif // LMBD_LAMP_TYPE__CCT

//
// indexable lamp type
//

#ifdef LMBD_LAMP_TYPE__INDEXABLE
static constexpr float lampBodyRadius_mm = 25; // external radius of the lamp body

// parameters of the led strip used
#ifdef LMBD_LAMP_TYPE__INDEXABLE_IS_HD
// hd 240L/m strip
static constexpr uint16_t LED_COUNT = 870;              // How many indexable leds are attached to the controler
static constexpr float consWattByMeter = 5;             // power consumption (in Watt/meters)
static constexpr uint16_t stripInputVoltage_mV = 12000; // voltage
static constexpr float ledByMeter = 244;                // the REAL indexable led by meters (for a 240Led/m)
static constexpr float ledStripWidth_mm = 5.2f;         // width of the led strip
static constexpr float ledStripHeigh_mm = 0.7f;         // heigh of the led strip (calibrated for this strip)

// compute the expected average loop runtime (in ms)
// defined as milliseconds / FPS
static constexpr uint32_t MAIN_LOOP_UPDATE_PERIOD_MS = static_cast<uint32_t>(1000 / 40.0f);
#else
// standard 160L/m strip
static constexpr uint16_t LED_COUNT = 580;              // How many indexable leds are attached to the controler
static constexpr float consWattByMeter = 5;             // power consumption (in Watt/meters)
static constexpr uint16_t stripInputVoltage_mV = 12000; // voltage
static constexpr float ledByMeter = 162.6f;             // the REAL indexable led by meters (for a 160Led/m)
static constexpr float ledStripWidth_mm = 5.2f;         // width of the led strip
static constexpr float ledStripHeigh_mm = 0.7f;         // heigh of the led strip (calibrated for this strip)

// compute the expected average loop runtime (in ms)
// defined as milliseconds / FPS
static constexpr uint32_t MAIN_LOOP_UPDATE_PERIOD_MS = static_cast<uint32_t>(1000 / 80.0f);
#endif

// physical parameters computations
static constexpr float ledSize_mm = 1000.0f / ledByMeter;                        // size of the individual led
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
static constexpr float circuitToLedZeroRotationX_degrees = 0.0f;
static constexpr float circuitToLedZeroRotationY_degrees = 0.0f;
static constexpr float circuitToLedZeroRotationZ_degrees = 88.0f;

#endif // LMBD_LAMP_TYPE__INDEXABLE

//
// add your own lamp type below :)
//

#endif
