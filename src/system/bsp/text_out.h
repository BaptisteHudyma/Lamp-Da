/*! \file text_out.h
    \brief Convertion header to use the text out functions from C code.
*/

#ifndef BSP_TEXT_OUT_H
#define BSP_TEXT_OUT_H

#ifdef __cplusplus
namespace lampda {
namespace bsp {

#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/**
 * This file should be used by .c files to access print functions
 */

EXTERNC void lampda_print_init();

EXTERNC void lampda_print(const char* format, ...);

/// raw print, no system additional logs
EXTERNC void lampda_print_raw(const char* format, ...);

#undef EXTERNC

#ifdef __cplusplus
} // namespace bsp
} // namespace lampda
#endif

#endif
