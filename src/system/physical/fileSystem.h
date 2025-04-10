#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <cstdint>
#include <string>

namespace fileSystem {

void setup();

/**
 * \brief Will initialize the state and brighness values if they exist
 */
void load_initial_values();

// clear the stored values in the currently loaded file system
void clear();

// hard clean of the whole filesystem, you will loose all stored data
void clear_internal_fs();

/**
 * \brief Will write the state and brighness values to the config file
 */
void write_state();

bool doKeyExists(const uint32_t key);
bool get_value(const uint32_t key, uint32_t& value);
void set_value(const uint32_t key, const uint32_t value);

/** \brief Drop all keys using the given bit prefix
 *
 * All keys such as `bitMatch == (bitSelect & key)` will be removed
 *
 * \param[in] bitMatch keys matching this pattern will be dropped
 * \param[in] bitSelect select which key bits to match with
 * \returns count of elements removed from storage
 */
uint32_t dropMatchingKeys(const uint32_t bitMatch, const uint32_t bitSelect = 0xffffffff);

} // namespace fileSystem

#endif
