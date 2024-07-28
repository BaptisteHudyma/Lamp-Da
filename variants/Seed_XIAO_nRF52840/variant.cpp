#include "variant.h"

#include "nrf.h"
#include "wiring_constants.h"
#include "wiring_digital.h"

const uint32_t g_ADigitalPinMap[] = {
    28,  // AD0 is P0.28/AIN4
    29,  // AD1 is P0.29/AIN5
    3,   // AD2 is P0.03/AIN1
    2,   // AD3 is P0.02/AIN0
    45,  // D4 is P1.13
    46,  // D5 is P1.14
    42,  // D6 is P1.10
    44,  // D7 is P1.12
    43,  // D8 is P1.11

    31,  // D9 is P0.31/AIN7: strip brightness

    13,  // D10 is P0.13 (SCL: Data of the main I2C line)
    15,  // D11 is P0.15 (SDA: Clock of the main I2C line)

    14,  // D12 is P0.14 USB 3.3V pwr

    30,  // D13 is P0.30/AIN6 : BAT21: battery level

    12,  // D14 is P0.12, CH_OK
    11,  // D15 is P0.11, EN_OTG
    41,  // D16 is P1.09, PROC_HOT

    17,  // D17 is P0.17, CH_INT

    // MIC
    20,  // D18 is P0.20 (MIC_PWR)
    32,  // D19 is P1.00 (PDM_CLK)
    16,  // D20 is P0.16 (PDM_DATA)

    // LSM6DS3TR
    40,  // D21 is P1.08 (IMU_PWR)
    27,  // D22 is P0.27 (IMU_SDA)
    7,   // D23 is P0.07 (IMU_SCL)
};

void initVariant() {
  pinMode(PIN_QSPI_CS, OUTPUT);
  digitalWrite(PIN_QSPI_CS, HIGH);

  pinMode(USB_33V_PWR, OUTPUT);
  digitalWrite(USB_33V_PWR, HIGH);

  pinMode(OUT_BRIGHTNESS, OUTPUT);
  digitalWrite(OUT_BRIGHTNESS, LOW);

  pinMode(BAT21, INPUT);

  pinMode(CHARGE_OK, INPUT_PULLUP_SENSE);
  pinMode(ENABLE_OTG, OUTPUT);
  pinMode(CHARGE_PROC_HOT, INPUT_PULLUP);

  digitalWrite(ENABLE_OTG, LOW);

  pinMode(CHARGE_INT, INPUT);
}
