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

class Alert
{
public:
    void raise_alert(const Alerts alert)
    {
        if (alert == Alerts::NONE)
            return;
        _current = alert;
    }

    void clear_alert(const Alerts alert)
    {
        if(_current != alert)
            return;
        _current = NONE;
    }

    Alerts current() const
    {
        return _current;
    }

private:
    volatile Alerts _current = Alerts::NONE;
};

static Alert AlertManager;

#endif