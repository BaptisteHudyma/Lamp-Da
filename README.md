# Lamp-Da

A compact lantern project.

This repository contains the software and PCB files to create a compact lantern, composed of a led strip wrapped around a 50mm cylinder.

The code is not designed to be the most readable but to be robust, so it can be quite tricky to understand, sorry !

## Behavior

base behavior:
- The lamp starts and stops with one click.
- The luminosity can be raised by clicking than holding, and diminished by double-clicking than holding.

The battery level is displayed as a color gradient from green (high) to red (low).

![battery levels](/Medias/lamp_top.jpg)

Error and alerts are displayed as blinking animations:
- low battery: slow red blinks
- critical battery: fast red blinks, emergency shutdown
- charging: breeze animation, changing color to reflect battery level
- incoherent battery readings: fast green blinks
- main loop too slow: fast fushia blinks
- temperature too high: fast orange blinks
- temperature extreme: emergency shutdown
- bluetooth advertising: blue breeze animation
- unhandled: fast white blinks

### User defined behaviors
The user MUST define the target behavior for the target uses.

Some functions are defined in user_functions.h, and must be implemented in user_functions.cpp
They allow the user to program the exact desired behavior.

Some branches are available for base models:
- cct_led_strip: Program designed for a constant color temperature led strip.
- constant_current_control_base: Program designed for a constant color led strip
- indexable_strip_base: program designed for an indexable led strip wrapped around the lamp body

![The different lamps](/Medias/lamp_types.jpg)
Above: constant colors, cct, indexable

## File details
- Lamp-Da.ino: main class of the program, containing the setup and loop functions
    - user_constants.h: constants defined by the user for the program. they must be updated to match the lamp characteristics
    - user_functions.h; function called by the program, that the user must update for a specific application

## Physical build and architecture :

The electrical circuit and build files can be found in the **electrical** folder.

![electrical circuit](/Medias/circuit.jpg)

The PCB can be ordered directly assembled from JLC PCB, for a total cost of around 270$ for the minimal 5 pieces command (price drops with a more commands, until around 11$/circuit).

The circuit is 4 cells li-ion USB C charger, that is also programable via the same USB port.
It features a constant current strip that can be as high as continuous 2.3A, controled by PWM.

The circuit features:
- USB-C 4S li-ion charger, based on BQ25703A ic.
- USB short circuit and EC protection, based on TPD8S300 ic.
- USB-C power negocitation, base on FUSB302 ic.
- Constant current led driver, that can maintain stable up to 2.3A, based on LM3409HV ic.
- 9 programable IO pins (4 of which can be analog inputs, 4 can be pwm outputs). Based on nRF52840 ic.
- MEMS Microphone, placed away from parasitic ringing components.
- LSM6DS3TR IMU, placed on the exact center so the axis alignement is easy.
- Bluetooth 5.1 low power, with correct 5/10m range.

- Multiple protection features (ESD spikes protection, USB voltage snubber, USB voltage limitation, ...)

Be careful:
- **NO REVERSE VOLTAGE PROTECTION FOR BATTERY**: it will blow the circuit right up
- IO are 3.3V max, any voltage greater will destroy the system


### How to program
Out of factory, the system will miss a bootloader.
You can flash a bootloader using the **IO | CL** pads on the board, with a JTAG probe, or an open source solution described below:

Once the bootloader is written, the microcontroler can be programed via USB, using the Arduino IDE or any flashing program.

#### Bootloader flash
- Use a raspberry of any other linux system with gpios
- Use the open source program **openocd** as described [in this post](https://forum.seeedstudio.com/t/xiao-ble-sense-bootloader-bricked-how-to-restore-it/263091/5)


# Program flash

Install the Adafruit nRF52 board support (version 1.6.1) [as described here](https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino)

Do not forget to replace the content of the version folder in .arduino15/packages/adafruit/hardware/nrf52 by the content of the [above repository](https://github.com/BaptisteHudyma/LampDa_nRF52_Arduino).

After restarting the Arduino IDE, you can select "LampDa nRF52840" in Tools > Board > Adafruit nRF52 Board.

After that, use the Arduino IDE as always, the program will compile for the LampDa board.
