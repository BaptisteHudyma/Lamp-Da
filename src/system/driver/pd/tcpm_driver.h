/*
 * tcpm_driver.h
 *
 * Created: 11/11/2017 18:42:39
 *  Author: jason
 */

#ifndef TCPM_DRIVER_H_
#define TCPM_DRIVER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../../src/system/platform/i2c.h"
#include "../../../../src/system/platform/time.h"

// USB-C Stuff
#include "../../../../src/system/bsp/pd/tcpm/tcpm.h"
#include "FUSB302.h"

#include "../../../../src/system/bsp/pd/config.h"

#ifdef __cplusplus
}
#endif

#endif /* TCPM_DRIVER_H_ */
