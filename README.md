# LampColorControler
An embedded program to control an indexable color strip wrapped around a cylinder. Based on Seed nRF52840 Sense module.

The code is not designed to be the most readable but to be robust, so it can be quite tricky to understand, sorry !

## Behavior
There are 5 modes :
- Constant: constant colors, that can be set by triple clicks than hold.
- Calm: slowly varying color schemes
- Party: Fast varying color schemes
- Sound: Animations using the microphone (vu meter, music react, ..)
- Flicking: For now, only the Police light mode.


The lamp starts with one click, by default in constant mode.
Use double click to switch between states inside modes, and triple click to go to the next mode.
You can also use four clicks to go back to the previous mode.


The luminosity can be raised by clicking than holding, and diminished by double-clicking than holding.
The battery level can be accessed by pressing 5 time the button.



Depends:
- python-nrfutil


## File details
- LampColorControler.ino: main class of the program, containing the setup and loop functions
- src
    - behavior.h: controls the lamp behaviors: what button actions does what. It's here that the animations are called
    - colors : Contain all the color/animations related stuff
        - animations.h: define some generic animations, that can be used as is with no fiddling
        - colors.h: Define the color class, that is the entrance point for all animations. You can define your own in it (SolidColor, DynamicColor, IndexedColor, ...)
        - palettes.h: handles color palettes. More intuitive than the color class above, but less customization possible
        - wipes.h: wipe animations, dot chase etc. Could be merged with animations.h in the future
    - ext: external libraries
    - physical: stuf related to the physical components: button, bluetooth, IMU, ...
        - bluetooth.h: Control the bluetooth associated behavior (foe now, causes segfaults, so not used)
        - button.h: control the button. Takes callbacks for actions on multiple button pushes. The implementation is intented to work with a tactile button (can be anything metallic really...)
        - Microphone.h: control the microphone behavior. Make available some functions to get the sound level and beat. Gives some animations as well
    - utils: General functions and constants that everybody needs
        - colorspace.h: contain color space transition classes. Execution of those can be quite heavy for a microcontroler, beware !
        - contants.h: global constants used all around the program
        - strip.h: overload of the Adafruit_NeoPixel class, to get cleaner color with getPixelColor
        - utils.h: useful functions to make colors

## Physical build and architecture :

TODO
