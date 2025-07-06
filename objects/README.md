# Objects

The parts to print for the lampe are :
- `Lampda-Poles.stl` x3 : no supports needed
- `Lampda-LowerPart.stl` x1 : print with the logo facing down, no supports needed
- `Lampda-Top.stl` x1: print face down, with support for the first layer only

The lower part can be used without the logo, or with you own, using the `Lampda-LowerPart_no_logo.stl`

Dependind on you printer settings, ou may need to remake the holes using a 3.5mm drill.

# Bill of Material
BOM:
- cylindrical tube, external diameter 50mm, internal diameter 46mm, 12cm lenght
- 3x21700 Li-ion batteries, in a 3S configuration
- 15cm clear heat shrink, 60mm diameter
- 16mm pushbutton
- 6 wire USB-C, panel mounted
- 15cm of rope, 2mm diameter
- 2 cylindrical pins (35mmx3mm)
- 2 cylindrical pins (50mmx3mm)
- the PCB

![USBC 6 pins](/Medias/6pin_usb_c.png) ![RGB push button](/Medias/rgb_push_button.png) ![Clear shrink wrap](/Medias/clear_shrink_wrap.png)

## Led Strips

The led strip lenght varies with strip width:
- a bit less than 4m for 5mm width
- around 2.5m for 8mm width

The led strips used in my case are :
- 2000K 12V, 8mm width, 480led/m COB strip : candle like color, used for the simple model
- indexable rgb 12V, 5mm width, 160led/m COB strip : indexable rgb strip, used for RGB model

# Assembly (not a tutorial)

The tube holes can be made accurately using the `hole_guides.stl`, and a 3mm drill.

The led strip can be wrapped using the `wrap_guide`, using the guide that matches the width of the led strip.

After the led strip is wrapped, shrink the shrink tube around it, and cut the excess.

Place the lower_part, and push the 50mm cylindrical through the tube and the lower_part.

Place the battery in the tube, and the 3 poles around it, with 2 vertical 35mm cylindrical pins. A drop of glue should be added at the 3 pole junction.

Place the PCB and close the assembly using the top part, and a final 50mm cylindrical pin. Do not forget to place the rope in the top part before pushing the cylindrical pin.


![Printed assembly](/Medias/printed_assembly.png) ![Full assembly](/Medias/full_assembly.png)
