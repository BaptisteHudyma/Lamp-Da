#ifndef PLATFORM_PRINT_H
#define PLATFORM_PRINT_H

#include <string>
#include <vector>

/**
 * \brief call once at program start
 */
extern void init_prints();

/**
 * \brief Print a screen to the external world
 * To use with caution, this process can be slow
 */
extern void lampda_print(const std::string& str);

/**
 * \breif read external inputs (may take some time)
 */
extern std::vector<std::string> read_inputs();

#endif