# Updating the system

The system is packed with a DFU compatible bootloader, accessible by USB.
The DFU update is accessible by puttin the system in update mode, using the following procedure :
- plug the system to a USB master device (eg: computer)
- Open a serial port at 115200 bauds
- test the connection by sending "v" to get system version, and "t" to get system type (simple, indexable, ...)
- type "DFU" to enter device firmware update
- The system reboots and appears as a mass storage device
- Put the .utf2 file corresponding to you system type into the mass storage device
- touch nothing !
- The system reboots with the updated firmware

Note : DFU stands for Device Firware Update

## Update using a terminal

In a first terminal :
Listen to output on the serial port `cat -v < /dev/ttyACM0`


On another terminal :
Open the serial port `sudo chmod o+rw /dev/ttyACM0`
Send the "t" command : `echo -e 't\0' > /dev/ttyACM0`

You should see the type of the lamp on the first terminal.

Then send the "DFU" command : `echo -e 'DFU\0' > /dev/ttyACM0` to make the system enter the DFU mode.
