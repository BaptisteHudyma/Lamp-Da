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
#include "../../../platform/time.h"

#include <stdint.h>

// USB-C Stuff
#include "../tcpm/tcpm.h"
#include "FUSB302.h"

#include "../config.h"

#ifdef __cplusplus
}
#endif

#endif /* TCPM_DRIVER_H_ */
