#ifndef C_LOGGER_H
#define C_LOGGER_H

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

/**
 * This file should be used by .c files to access print functions
 */

EXTERNC void lampda_print(const char* format, ...);

#undef EXTERNC

#endif
