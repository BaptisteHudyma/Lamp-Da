#ifndef SERIAL_SIMULATOR_H
#define SERIAL_SIMULATOR_H

#include <iostream>

struct SerialTy
{
  void println(auto input) { std::cerr << "serial: " << input << std::endl; }
};

SerialTy Serial {};

#endif
