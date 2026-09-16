/*! \file bsp/filesystem.h
    \brief High level actions on the file system
*/

#pragma once

#include <cstdint>

namespace lampda {
namespace bsp {
/// File system high level actions.
namespace filesystem {

/// call once on program stop.
void shutdown();

/// hard clean of the whole filesystem, you will loose all stored data.
void clear_internal_fs();

/// internal system file space.
namespace system {

/// Return true if the given key exists in the filesystem
bool doKeyExists(const uint32_t key);
/**
 * \brief Check and return a value stored in a filesystem
 * \param[in] key The key to get the value from
 * \param[out] value The associated value, if the function returns true
 * \return true if the key exists and has a value
 */
bool get_value(const uint32_t key, uint32_t& value);
/**
 * \brief Store a key/value in the filesystem
 * \param[in] key The key to store at
 * \param[in] value The associated value
 */
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

/// clear the stored values in the currently loaded file system.
void clear_cached();

/**
 * \brief Write the system parameters to a file
 * \warning A failure will erase the system memory
 */
void write_to_file();

/**
 * \brief Load system values from memory
 * \return true if succesfull
 */
bool load_from_file();

/// Delete the system file (Should not be used)
bool format();

} // namespace system

/// user file space.
namespace user {

/// Return true if the given key exists in the filesystem
bool doKeyExists(const uint32_t key);
/**
 * \brief Check and return a value stored in a filesystem
 * \param[in] key The key to get the value from
 * \param[out] value The associated value, if the function returns true
 * \return true if the key exists and has a value
 */
bool get_value(const uint32_t key, uint32_t& value);
/**
 * \brief Store a key/value in the filesystem
 * \param[in] key The key to store at
 * \param[in] value The associated value
 */
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

/// clear the stored values in the currently loaded file system.
void clear_cached();

/**
 * \brief Write the user parameters to a file
 * \warning A failure will erase the system memory
 */
void write_to_file();

/**
 * \brief Load user values from memory
 * \return true if the process suceeded
 */
bool load_from_file();

/// Delete the user file
bool format();

} // namespace user

} // namespace filesystem
} // namespace bsp
} // namespace lampda
