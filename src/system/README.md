# File details
- behavior.h: controls the lamp behaviors: what button actions does what, battery level, charger start and stops, ...
- alert.h: Handle the diffferent alerts raised by the program
- charger: charger related operations
    - charger.h: main high level logic to use the charger, as well as power switches
    - power_source.h: handle pd negociation, cable detection, ...
    - BQ25703A: Folder to handler the charging processes with the target ic
    - PDlib: folder that contains the library to talk to the PD negocation ic. Adapted to this architecture
- ext: external libraries
- physical: stuf related to the physical components: button, bluetooth, IMU, ...
    - LSM6DS3: library to talk to the IMU. Adapted to this architecture
    - battery.h: handle the battery readings, for battery level
    - bluetooth.h: Control the bluetooth associated behavior
    - button.h: control the button. Takes callbacks for actions on multiple button pushes. Used to display stuf on the button if needed
    - fft.h: implementation of the fft and assocated filtering
    - fileSystem.h: handle the reading and writting of variables to memory
    - IMU.h: the imu related operations
    - led_power.h: interface of the led constant current driver
    - Microphone.h: control the microphone behavior. Make available some functions to get the sound level and beat. Gives some animations as well
- utils: General functions and constants that everybody needs
    - colorspace.h: contain color space transition classes. Execution of those can be quite heavy for a microcontroler, beware !
    - constants.h: global constants used all around the program
    - serial.h: handle serial communication. Location of the CLI capabilities
    - utils.h: useful functions to make colors