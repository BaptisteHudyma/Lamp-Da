# LampColorControler
An embedded program to control an indexable color strip wrapped around a cylinder. Based on Seed nRF52840 Sense module

The code is not designed to be the most readable but to be robust, so it can be quite tricky to understand, sorry !


depends:
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
        - bluetooth.h: Control the bluetooth associated behavior
        - button.h: control the button. Takes callbacks for actions on multiple button pushes. The implementation is intented to work with a tactile button (can be anything metallic really...)
        - Microphone.h: control the microphone behavior. Make available some functions to get the sound level and beat. Gives some animations as well
    - utils: General functions and constants that everybody needs
        - contants.h: global constants used all around the program
        - strip.h: overload of the Adafruit_Strip class, to get cleaner color with getPixelColor
        - utils.h: useful functions to control colors, color spaces and more !

## Physical build and architecture :

TODO