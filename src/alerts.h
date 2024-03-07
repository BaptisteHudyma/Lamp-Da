#ifndef ALERTS_H
#define ALERTS_H

enum Alerts {
    NONE,                   // system is sane and ready
    
    MAIN_LOOP_FREEZE,       // main loop does not respond
    UNKNOWN_COLOR_MODE,     // An incorrect color mode was reached 
    UNKNOWN_COLOR_STATE,    // An incorrect color state was reached
    BATTERY_LOW,            // battery is dangerously low
    BATTERY_CRITICAL,       // battery is too low, shutdown immediatly
    LONG_LOOP_UPDATE,       // the main loop is taking too long to execute (bugs when reading button inputs)
    BATTERY_READINGS_INCOHERENT  // the pin that reads the battery value is not coherent with it's givent min and max
};

// store the current alert, if any
static volatile Alerts currentAlert = Alerts::NONE;

#endif