#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <cstdint>
#include <string>

namespace fileSystem {

void setup();

// clear the stored values in the currently loaded file system
void clear();

// hard clean of the whole filesystem, you will loose all stored data
void clear_internal_fs();

// internal system file space
namespace system {

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

/**
 * \brief Write the system parameters to a file
 * /!\ a failure will erase the system memory
 */
void write_to_file();

/**
 * \brief Load system values from memory
 */
bool load_from_file();

} // namespace system

// user file space
namespace user {

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

/**
 * \brief Write the user parameters to a file
 * /!\ a failure will erase the system memory
 */
void write_to_file();

/**
 * \brief Load user values from memory
 */
bool load_from_file();

} // namespace user

} // namespace fileSystem

#endif
