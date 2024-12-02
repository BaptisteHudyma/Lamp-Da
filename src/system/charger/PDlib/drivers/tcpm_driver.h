/*
 * tcpm_driver.h
 *
 * Created: 11/11/2017 18:42:39
 *  Author: jason
 */

#ifndef TCPM_DRIVER_H_
#define TCPM_DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../platform/i2c.h"

#include <stdint.h>

// USB-C Stuff
#include "../tcpm/tcpm.h"
#include "FUSB302.h"
#define CONFIG_USB_PD_PORT_COUNT 1

#ifdef __cplusplus
}
#endif

#endif /* TCPM_DRIVER_H_ */