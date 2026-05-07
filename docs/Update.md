# Updating the system

The system is packed with a DFU compatible bootloader, accessible by USB.
The DFU update is accessible by putting the system in update mode, using the following procedure :
- plug the system to a USB master device (eg: computer)
- Open a serial port at 115200 bauds
- test the connection by sending "v" to get system version, and "t" to get system type (simple, indexable, ...)
- type "DFU" to enter device firmware update
- The system reboots and appears has a mass storage device
- Put the .utf2 file corresponding to your system type into the mass storage device
- **touch nothing ! wait for the system to disconnect on its own**. it should take a minute at most.
- The system reboots with the updated firmware

Note : DFU stands for Device Firware Update

## Using a terminal

In a first terminal :
Listen to output on the serial port `cat -v < /dev/ttyACM0`

On another terminal :
Open the serial port `sudo chmod o+rw /dev/ttyACM0`
Send the "t" command : `echo -e 't\0' > /dev/ttyACM0`

You should see the type of the lamp on the first terminal.

Then send the "DFU" command : `echo -e 'DFU\0' > /dev/ttyACM0` to make the system enter the DFU mode.

## Using the Arduino IDE

In *Tools* tab, select the port where the system is connected. It will often be "/dev/ttyACM0" in linux.

Open the *Serial Monitor*, in the *Tools* tab.
Set the speed to 115200 bauds.

You can now send commands and receive answers from the system CLI.


On Linux, you have to check that the Arduino IDE can access the serial port.

## Using the repository

This repository has some built in tools to debug and update the system.
The command `make monitor` will open a serial monitor to the system on /dev/ttyACM0.

When using the `make upload-` commands, the upload of nexw code is automatic if the compilation succeeds.
