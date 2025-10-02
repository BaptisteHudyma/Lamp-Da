#ifndef PARAMETER_PARSER_H
#define PARAMETER_PARSER_H

#include <fstream>
#include <sstream>
#include <iostream>

#include "simulator/include/hardware_influencer.h"

static const char* fileName = "./simulator/resources/simulation_parameters.txt";

static const char* batteryVoltageKey = "batt_V";
static const char* vbusVoltageKey = "vbus_V";
static const char* cpuTemperatureKey = "cpu_temp";
static const char* addedAlgoDelayKey = "algo_del";

inline void read_and_update_parameters()
{
  static bool fileNotDetectedCalled = false;

  std::ifstream file(fileName);
  if (file.is_open())
  {
    fileNotDetectedCalled = false;

    while (file)
    {
      std::string line;
      std::getline(file, line);

      // skip empty lines & comments
      if (line.empty() or line[0] == '#')
        continue;
      std::stringstream linestream(line);
      if (!linestream)
        continue;

      std::string key;
      float value;
      linestream >> key >> value;

      if (key == batteryVoltageKey)
      {
        mock_battery::voltage = value;
      }
      else if (key == vbusVoltageKey)
      {
        mock_electrical::inputVbusVoltage = value;
      }
      else if (key == cpuTemperatureKey)
      {
        mock_registers::cpuTemperature = value;
      }
      else if (key == addedAlgoDelayKey)
      {
        mock_registers::addedAlgoDelay = value;
      }
      // TODO: other parameters
    }
  }
  else
  {
    if (!fileNotDetectedCalled)
      std::cerr << "Parameter file " << fileName << " not detected" << std::endl;
    fileNotDetectedCalled = true;
  }

  file.close();
}

#endif
