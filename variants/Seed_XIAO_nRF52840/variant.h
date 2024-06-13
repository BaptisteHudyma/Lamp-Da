#ifndef _SEEED_XIAO_NRF52840_SENSE_H_
#define _SEEED_XIAO_NRF52840_SENSE_H_

#define TARGET_SEEED_XIAO_NRF52840_SENSE

/** Master clock frequency */
#define VARIANT_MCK (64000000ul)

#define USE_LFXO  // Board uses 32khz crystal for LF
// #define USE_LFRC    // Board uses RC for LF

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define PINS_COUNT (33)
#define NUM_DIGITAL_PINS (33)
#define NUM_ANALOG_INPUTS (8)
#define NUM_ANALOG_OUTPUTS (0)

// LEDs
#define LED_BLUE (PINS_COUNT)  // No connection
#define PIN_LED (PINS_COUNT)
#define LED_PWR (PINS_COUNT)
#define PIN_NEOPIXEL (PINS_COUNT)
#define NEOPIXEL_NUM (0)

#define LED_STATE_ON (1)  // State when LED is litted

// Buttons
#define PIN_BUTTON1 (PINS_COUNT)

// Analog Digital PINs
static const uint8_t AD0 = 0;
static const uint8_t AD1 = 1;
static const uint8_t AD2 = 2;
static const uint8_t D4 = 4;
static const uint8_t D5 = 5;
static const uint8_t D6 = 6;
static const uint8_t D7 = 7;
static const uint8_t D8 = 8;

// IO
#define OUT_BRIGHTNESS (9)
#define BAT21 (13)
#define USB_33V_PWR (12)
#define OUTPUT_VOLTAGE (3)

// Charger
#define CHARGE_OK (14)
#define ENABLE_OTG (15)
#define CHARGE_PROC_HOT (16)

#define CHARGE_INT (17)

#define ADC_RESOLUTION (12)

// master sda/scl (charger and usb negocitation)
#define SDA_MASTER 11
#define SCL_MASTER 10

// SPI Interfaces
#define SPI_INTERFACES_COUNT (0)

// Wire Interfaces
#define WIRE_INTERFACES_COUNT (2)

#define PIN_WIRE_SDA SDA_MASTER
#define PIN_WIRE_SCL SCL_MASTER

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

static const uint8_t SS = 7;

#define PIN_WIRE1_SDA (22)
#define PIN_WIRE1_SCL (23)
#define PIN_LSM6DS3TR_C_POWER (21)

// PDM Interfaces
#define PIN_PDM_PWR (18)
#define PIN_PDM_CLK (19)
#define PIN_PDM_DIN (20)

// QSPI Pins
#define PIN_QSPI_SCK (24)
#define PIN_QSPI_CS (25)
#define PIN_QSPI_IO0 (26)
#define PIN_QSPI_IO1 (27)
#define PIN_QSPI_IO2 (28)
#define PIN_QSPI_IO3 (29)

// On-board QSPI Flash
#define EXTERNAL_FLASH_DEVICES (P25Q16H)
#define EXTERNAL_FLASH_USE_QSPI

// Default address for device. Note, it is without read/write bit. When read
// with analyser, this will appear 1 bit shifted to the left
#define BQ25703ADevaddr 0x6B

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
