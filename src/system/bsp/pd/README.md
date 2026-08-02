USB pd is complicated.

This whole lib is a mess to understand, as much as it was to make work, so have some pity for yourself and do not step in.

The board can only be used as a data URP (USB slave, if you dont speak USB "nomenclature"), but can act as a power source (provider) or sink (consummer).
By default it should be used as a sink, but do whatever.

This lib handles power profile selection (as SINK) or power profile selection handling (as source), and power role swap.
The power role swap allows the system to go from sink to source, if requested by the plugged in device.

Here is an indicative map if you really really need to get going, or maybe just curious:
- the **drivers** folder contains the drivers for certain PD negociation chips. This project uses the FUSB302B for now, but this could change in the future.
- All files that starts with **tcpm** are the abstraction layer to the hardware. They link the drivers to the usb-PD algorithms.
- All the rest is a complex mess of the USB-pd algorithms, defined in the norm... It as been adapted to remove the unused functions (makes it lighter to compile/run), and prevent the reader from experiencing heavy nausea.


Some definitions to help in your quest:
- DFP: Downward facing port: USB host
- UFP: Upfront facing port: USB slave
- DRP: Dural role port: Can be host or slave, depending on context ! (an negociations)
- tcpm: Type C port management: interface between the hardware and software specific code
- tcpc: Type C port controler: generic usb type c controls
- PPS: Programmable Power Supply (from 3.3 to 20V in steps of 20mV)
- SPR: Standard power range (5V, 9V, 15V, 20V, PPS, to maximum 3/5A)
- EPR: Extended power range (up to 45V)
- GoodCRC: acknowledge message

- VCONN: special USB c line, used to power emarker chips. Not used in this program

USB pd message types
- DO: data object, prefix
- BDO: BIST DO, physical layer compliance testing
- PDO: Power DO, source power capabilities, or sink power requirements
- RDO: Request DO, used by a sink to require a target source PDO
- VDO: vendor DO, vendor specific informations
- BSDO: Battery Status DO, battery status informations
- ADO, Alert DO, to detail incidents that happened on the line


A standard cable should support 20V 3A.
To get more, you need to program something to speak to a marker chip in the cable.
We do not need it in this project, so you wont find code for it.