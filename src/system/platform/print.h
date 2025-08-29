#ifndef PLATFORM_PRINT_H
#define PLATFORM_PRINT_H

#include <string>
#include <vector>
#include <stdarg.h>

#include "src/system/utils/print.h"

/**
 * \brief call once at program start
 */
extern void init_prints();

/**
 * \breif read external inputs (may take some time)
 */
extern std::vector<std::string> read_inputs();

#endif
