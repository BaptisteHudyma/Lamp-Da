# Memory usage

# Flash usage

Statically checked after compilation, displayed as a percentage of the total available flash.

## Stack and RAM usage

Compile the project, than run :
`python tools/ramAnalyser.py --elf _build/objs/LampColorControler.ino.elf --sudir ./_build/objs/`

This program will fail on a stack budget overflow.
It shows if the program uses dynamic memory allocations, and the path of most stack use.
