
/**
 * PD_UFP.h
 *
 *      Author: Ryan Ma
 *      Edited: Kai Liebich
 *
 * Minimalist USB PD Ardunio Library for PD Micro board
 * Only support UFP(device) sink only functionality
 * Requires FUSB302_UFP.h, PD_UFP_Protocol.h and Standard Arduino Library
 *
 * Support PD3.0 PPS
 *
 */

#ifndef PD_UFP_H
#define PD_UFP_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <stdint.h>

#include "FUSB302_UFP.h"
#include "PD_UFP_Protocol.h"

enum
{
  STATUS_POWER_NA = 0,
  STATUS_POWER_TYP,
  STATUS_POWER_PPS
};
typedef uint8_t status_power_t;

///////////////////////////////////////////////////////////////////////////////////////////////////
// PD_UFP_c
///////////////////////////////////////////////////////////////////////////////////////////////////
class PD_UFP_c
{
public:
  PD_UFP_c();
  // Init
  void init(uint8_t int_pin, enum PD_power_option_t power_option = PD_POWER_OPTION_MAX_5V);
  void init_PPS(uint8_t int_pin,
                uint16_t PPS_voltage,
                uint8_t PPS_current,
                enum PD_power_option_t power_option = PD_POWER_OPTION_MAX_5V);
  // Task
  void run(void);
  // Status
  bool is_power_ready(void) { return status_power == STATUS_POWER_TYP; }
  bool is_PPS_ready(void) { return status_power == STATUS_POWER_PPS; }
  bool is_ps_transition(void) { return send_request || wait_ps_rdy; }
  // Get
  uint16_t get_voltage(void) { return ready_voltage; } // Voltage in 50mV units, 20mV(PPS)
  uint16_t get_current(void) { return ready_current; } // Current in 10mA units, 50mA(PPS)
  status_power_t get_ps_status(void) { return status_power; }
  // Set
  bool set_PPS(uint16_t PPS_voltage, uint8_t PPS_current);
  void set_power_option(enum PD_power_option_t power_option);
  // Clock
  static void clock_prescale_set(uint8_t prescaler);

  bool is_vbus_ok();
  uint16_t get_vbus_voltage();
  // there is a PD compatible source, the power is negociated, or will be soon
  bool is_USB_PD_available() { return isPowerNegociated; }
  // there is a standard USB, but may have some available power
  // This is just an indication, the true available power may be much lower
  bool is_USB_power_available() { return isPowerDetectedCC; }

  void reset();

protected:
  static FUSB302_ret_t FUSB302_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint8_t count);
  static FUSB302_ret_t FUSB302_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t* data, uint8_t count);
  static FUSB302_ret_t FUSB302_delay_ms(uint32_t t);
  void handle_protocol_event(PD_protocol_event_t events);
  void handle_FUSB302_event(FUSB302_event_t events);
  bool timer(void);
  void set_default_power(void);
  // Device
  FUSB302_dev_t FUSB302;
  PD_protocol_t protocol;
  uint8_t int_pin;
  // Power ready power
  uint16_t ready_voltage;
  uint16_t ready_current;
  // PPS setup
  uint16_t PPS_voltage_next;
  uint8_t PPS_current_next;
  // Status
  virtual void status_power_ready(status_power_t status, uint16_t voltage, uint16_t current);
  uint8_t status_initialized;
  uint8_t status_src_cap_received;
  status_power_t status_power;
  // Timer and counter for PD Policy
  uint16_t time_polling;
  uint16_t time_wait_src_cap;
  uint16_t time_wait_ps_rdy;
  uint16_t time_PPS_request;
  uint8_t get_src_cap_retry_count;
  uint8_t wait_src_cap;
  uint8_t wait_ps_rdy;
  uint8_t send_request;
  static uint8_t clock_prescaler;
  bool isPowerNegociated;
  bool isPowerDetectedCC;
  // Time functions
  void delay_ms(uint16_t ms);
  uint16_t clock_ms(void);
};

#endif
