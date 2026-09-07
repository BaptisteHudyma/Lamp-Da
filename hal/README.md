# HAL (Hardware Abstraction Layer)

## Overview

This directory contains the Hardware Abstraction Layer (HAL) for the embedded system. The HAL provides a unified interface to hardware and simulation environments, allowing the same application code to run on both physical and simulated platforms.

## Structure

### `adafruit_nrf/`

Hardware implementation for the **NRF52840 microcontroller** using the Adafruit NRF core as the backend. This folder contains all device drivers and low-level interfaces specific to the NRF52840 platform.

### `simulator/`

Linux-based simulator implementation. Provides a virtual environment for development and testing using standard Linux interfaces and custom simulation variables.

## Platform Selection

At compile time, one of these two folders is linked into the build depending on the target platform:

- **Physical hardware**: links `adafruit_nrf/`
- **Simulation**: links `simulator/`

The application code remains unchanged regardless of the selected platform.

Any modification in the HAL must be done here, not in the generated files under generated/hal.
