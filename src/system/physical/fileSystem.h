#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>

namespace fileSystem {

void setup();

/**
 * \brief Will initialize the state and brighness values if they exist
 */
void load_initial_values();

void clear();

/**
 * \brief Will write the state and brighness values to the config file
 */
void write_state();

bool get_value(const std::string& key, uint32_t& value);
void set_value(const std::string& key, const uint32_t value);

}  // namespace fileSystem

#endif