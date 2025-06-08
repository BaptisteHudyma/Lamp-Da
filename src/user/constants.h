#ifndef USER_CONSTANTS_H
#define USER_CONSTANTS_H

#include <cstdint>

//
// common to all lamp type
//

// does button usermode defaults to "lockdown" mode?
static constexpr bool usermodeDefaultsToLockdown = false;

//
// simple lamp type
//

#ifdef LMBD_LAMP_TYPE__SIMPLE

// Update when the soft changes version
#define USER_SOFTWARE_VERSION_MAJOR 0
#define USER_SOFTWARE_VERSION_MINOR 1

// parameters of the led strip used
constexpr float consWattByMeter = 12;            // power consumption (in Watt/meters)
constexpr float inputVoltage_V = 12;             // voltage (volts)
constexpr float ledStripLenght_mm = 91.0 * 25.0; // 91 sections of 25 mm

// define position of led 0 to the circuit center
constexpr float ledZeroToCircuitRotationX_degrees = 0;
constexpr float ledZeroToCircuitRotationY_degrees = 0;
constexpr float ledZeroToCircuitRotationZ_degrees = 22;

#endif // LMBD_LAMP_TYPE__SIMPLE

//
// cct lamp type
//

#ifdef LMBD_LAMP_TYPE__CCT

// Update when the soft changes version
#define USER_SOFTWARE_VERSION_MAJOR 0
#define USER_SOFTWARE_VERSION_MINOR 1

// parameters of the led strip used
constexpr float consWattByMeter = 10;            // power consumption (in Watt/meters)
constexpr float inputVoltage_V = 12;             // voltage (volts)
constexpr float ledStripLenght_mm = 67.0 * 27.0; // 67 sections of 27 mm

// define position of led 0 to the circuit center
constexpr float ledZeroToCircuitRotationX_degrees = 0;
constexpr float ledZeroToCircuitRotationY_degrees = 0;
constexpr float ledZeroToCircuitRotationZ_degrees = 20;

#endif // LMBD_LAMP_TYPE__CCT

//
// indexable lamp type
//

#ifdef LMBD_LAMP_TYPE__INDEXABLE

// Update when the soft changes version
#define USER_SOFTWARE_VERSION_MAJOR 0
#define USER_SOFTWARE_VERSION_MINOR 1

constexpr float lampBodyRadius_mm = 25; // external radius of the lamp body

// parameters of the led strip used
constexpr uint16_t LED_COUNT = 582;     // How many indexable leds are attached to the controler
constexpr float consWattByMeter = 5;    // power consumption (in Watt/meters)
constexpr float inputVoltage_V = 12;    // voltage (volts)
constexpr uint16_t ledByMeter = 160.0;  // the indexable led by meters
constexpr float ledStripWidth_mm = 5.2; // width of the led strip

// physical parameters computations
constexpr float ledSize_mm = 1.0 / ledByMeter * 1000.0; // size of the individual led
constexpr float lampBodyCircumpherence_mm = 2.0 * 3.14159265 * lampBodyRadius_mm;
constexpr float ledStripLenght_mm = LED_COUNT * ledSize_mm;

constexpr float stripXCoordinates = lampBodyCircumpherence_mm / ledSize_mm + 0.35;
constexpr float stripYCoordinates = ledStripLenght_mm / lampBodyCircumpherence_mm;
constexpr float lampBodyHeight_mm = stripYCoordinates * ledStripWidth_mm;

// define position of led 0 to the circuit center
constexpr float ledZeroToCircuitRotationX_degrees = 0;
constexpr float ledZeroToCircuitRotationY_degrees = 0;
constexpr float ledZeroToCircuitRotationZ_degrees = 22;

#endif // LMBD_LAMP_TYPE__INDEXABLE

//
// add your own lamp type below :)
//

#endif
