#ifndef MODES_HARDWARD_KEYSTORE_HPP
#define MODES_HARDWARD_KEYSTORE_HPP

#include "src/modes/include/compile.hpp"
#include "src/modes/include/tools.hpp"

#include "src/system/physical/fileSystem.h"
#include "src/system/utils/utils.h"

#include <cstring>
#include <array>

/// Mode key store interface, see ContextTy::KeyProxy
namespace modes::store {

/** \private Store identifier (change to force cleanup of all values)
 *
 * Implementation should call migrateIfNeeded() which implements:
 *
 *    If not hasValue(storeUid):
 *        setValue(storeUid, storeUid);
 *    If not getValue(storeUid) == storeUid:
 *        forceEraseAllData();
 *        setValue(storeUid, storeUid);
 *
 * This enable to force cleanup of an old database after a migration, by
 * changing the value picked as store unique identifier.
 */
static constexpr uint32_t storeUid = utils::hash("StoreRev:v01");

/// \private Map a 32-bit hash value to a 31-bit value
static constexpr uint32_t map32to31hash(uint32_t a)
{
  uint32_t lsb = a & 0b1; // mix the lost LSB info into the 13th bit
  uint32_t val = (a ^ storeUid ^ (lsb << 14));
  return val >> 1;
}

/// \private Same as hash, but called when hash("string") to verify length
template<int16_t N> static constexpr uint32_t LMBD_INLINE hash(const char (&s)[N])
{
  static_assert(N - 1 <= 14, "Please use keys shorter than 14 bytes!");
  return map32to31hash(utils::hash(s, N));
}

/** \brief Set value for named \p key and read in in \p out
 *
 * \param[in] key Key string to be hashed as 31 bit integer
 * \param[in] value Value to be saved for \p key corresponding hash
 */
template<int16_t N> static inline void LMBD_INLINE setValue(const char (&key)[N], uint32_t value)
{
  static_assert(N - 1 <= 14, "Please use keys shorter than 14 bytes!");
  fileSystem::user::set_value(hash(key), value);
}

/** \brief Get value for named \p key and write it in \p out
 *
 * \param[in] key Key string to be hashed as 31 bit integer
 * \param[out] out Value where result will be written if found
 * \return Returns True if key is found and \p out has been written
 */
template<int16_t N> static inline bool LMBD_INLINE getValue(const char (&key)[N], uint32_t& out)
{
  static_assert(N - 1 <= 14, "Please use keys shorter than 14 bytes!");
  return fileSystem::user::get_value(hash(key), out);
}

/** \brief Check if value for \p key exists
 *
 * \param[in] key Key string to be hashed as 31 bit integer
 * \return Returns True if key exists
 */
template<int16_t N> static inline bool LMBD_INLINE hasValue(const char (&key)[N], uint32_t& out)
{
  static_assert(N - 1 <= 14, "Please use keys shorter than 14 bytes!");
  return fileSystem::user::doKeyExists(hash(key), out);
}

/// Force clear the stored parameters
static inline void clear_stored() { fileSystem::clear(); }

/**
 * \brief  Check for migration and erase all values if needed
 * IT WILL ERASE THE WHOLE MEMORY, EVENT STATISTICS
 */

static inline void LMBD_INLINE migrateIfNeeded()
{
  if (not fileSystem::user::doKeyExists(storeUid))
  {
    fileSystem::user::set_value(storeUid, storeUid);
  }

  uint32_t out = storeUid ^ 0xff;
  if (not fileSystem::user::get_value(storeUid, out))
  {
    fileSystem::user::set_value(storeUid, storeUid);
    fileSystem::user::get_value(storeUid, out);
  }

  if (out != storeUid)
  {
    fileSystem::clear_internal_fs();
    fileSystem::clear();
    fileSystem::user::set_value(storeUid, storeUid);
    fileSystem::user::write_to_file();
    fileSystem::system::write_to_file();

    fileSystem::system::load_from_file();
    fileSystem::user::load_from_file();
  }
}

/** \private Flag indexed / enumeration-based key space
 *
 * There are two ways of storing and retrieving persistent key store values:
 *  - by named key, a C-string hashed on the 31 least-significant bytes
 *  - by indexed keys, using fixed values, all starting by a 1 on their MSB
 *
 * This flag (0x8000000) reflects this and is or-ed to each PrefixValues
 */
static constexpr uint32_t indexKeyFlag = (1 << 31);

/** \private Possible prefixes to differentiate indexed keys use cases
 *
 * The bit format for all indexed keys is the following:
 *
 * ```
 *    indexed key format, first 16 most-significant bits:
 *
 * 0b1                        31  1       (always present)
 *    ..                      29  0-3     prefix to differentiate use-cases
 *      ....                  25  0-14    group identifier
 *          .....             20  0-30    mode identifier
 *               ....         16  0-15    data offset for large values
 * ```
 *
 * Where the 16 least-significant bits are used by user to enumerate keys.
 */
enum class PrefixValues : uint32_t
{
  generalKeys = indexKeyFlag | (0x0 << 29), ///< General keys (user)
  managerKeys = indexKeyFlag | (0x1 << 29), ///< Manager keys (per-mode)
  privateKeys = indexKeyFlag | (0x2 << 29), ///< Private keys (system-only)
  invalidKeys = indexKeyFlag | (0x3 << 29), ///< (reserved) for later use
};

/// \private Used for no group index (global to all groups and all modes)
static constexpr uint32_t noGroupIndex = 15;

/// \private Used for no mode index (local to group, global to its modes)
static constexpr uint32_t noModeIndex = 31;

/// \private Return indexed key given its parameters
static constexpr uint32_t LMBD_INLINE
indexKeyFor(PrefixValues prefix, uint32_t groupId, uint32_t modeId, uint32_t offset, uint32_t index)
{
  uint32_t rawPrefix = (uint32_t)prefix;

  assert((rawPrefix >> 31) == 0b1); // prefix on 2 of the MSBs +indexKeyFlag
  assert((groupId >> 4) == 0);      // groupId on 4 bits (0-14)
  assert((modeId >> 5) == 0);       // modeId on 5 bits (0-30)
  assert((offset >> 4) == 0);       // offset on 4 bits (0-15)
  assert((index >> 16) == 0);       // index on 16 bits (0-65535)

  return rawPrefix | (groupId << 25) | (modeId << 20) | (offset << 16) | index;
}

/// \private Orchestrate interactions with the indexed key store
template<typename _EnumTy, int _groupId, int _modeId, PrefixValues _prefix = PrefixValues::generalKeys> struct KeyStore
{
  using EnumTy = _EnumTy;
  static constexpr uint32_t groupId = _groupId < 0 ? noGroupIndex : _groupId;
  static constexpr uint32_t modeId = _modeId < 0 ? noModeIndex : _modeId;
  static constexpr PrefixValues prefix = _prefix;

  // it's magic!
  static constexpr uint16_t storeIdKey = 0xafff;

  static_assert(sizeof(EnumTy) == sizeof(uint16_t), "uint16_t enum only");
  static_assert((groupId >> 4) == 0, "Group index must fit on 4 bits!");
  static_assert((modeId >> 5) == 0, "Mode index must fit on 5 bits!");

  // \private maps to ContextTy::KeyProxy::setValue()
  template<EnumTy key, typename T> static inline void LMBD_INLINE setValue(T value)
  {
    using valueTy = std::remove_cv_t<std::remove_reference_t<T>>;
    constexpr uint16_t rawKey = (uint16_t)key;

    static_assert(storeIdKey != rawKey, "error: key can not match storeIdKey (0xafff)");

    // simple case: value is uint32_t, store it as-is
    if constexpr (std::is_same_v<valueTy, uint32_t>)
    {
      constexpr auto idx = indexKeyFor(prefix, groupId, modeId, 0, rawKey);
      fileSystem::user::set_value(idx, value);

      // small case: value is smaller than uint32_t, store it on a uint32_t
    }
    else if constexpr (sizeof(valueTy) <= sizeof(uint32_t))
    {
      uint32_t storage = 0;
      details::bit_cast<sizeof(value)>(storage, value);
      setValue<key>(storage);

      // large case: value is stored on up to 16 uint32_t
    }
    else if constexpr (sizeof(valueTy) <= 16 * sizeof(uint32_t))
    {
      constexpr uint8_t N = ((sizeof(valueTy) - 1) / sizeof(uint32_t)) + 1;
      std::array<uint32_t, N> storage {};
      details::bit_cast<sizeof(value)>(storage, value);
      for (uint8_t I = 0; I < N; ++I)
      {
        uint32_t off = indexKeyFor(prefix, groupId, modeId, I, rawKey);
        fileSystem::user::set_value(off, storage[I]);
      }
    }
    else
    {
      static_assert(sizeof(valueTy) <= 64, "KeyStore values limited to 64 bytes!");
    }
  }

  // \private maps to ContextTy::KeyProxy::getValue()
  template<EnumTy key, typename T> static inline bool LMBD_INLINE getValue(T& output)
  {
    constexpr uint16_t rawKey = (uint16_t)key;

    static_assert(storeIdKey != rawKey, "error: key can not match storeIdKey (0xafff)");

    // simple case: value is uint32_t, get it as-is
    if constexpr (std::is_same_v<T, uint32_t>)
    {
      constexpr auto idx = indexKeyFor(prefix, groupId, modeId, 0, rawKey);
      return fileSystem::user::get_value(idx, output);

      // small case: value is smaller than uint32_t, get it on a uint32_t
    }
    else if constexpr (sizeof(T) <= sizeof(uint32_t))
    {
      uint32_t storage = 0;
      if (not getValue<key, uint32_t>(storage))
      {
        return false;
      }

      details::bit_cast<sizeof(output)>(output, storage);
      return true;

      // large case: value is stored on up to 16 uint32_t
    }
    else if constexpr (sizeof(T) <= 16 * sizeof(uint32_t))
    {
      constexpr uint8_t N = ((sizeof(T) - 1) / sizeof(uint32_t)) + 1;
      std::array<uint32_t, N> storage {};
      for (uint8_t I = 0; I < N; ++I)
      {
        uint32_t off = indexKeyFor(prefix, groupId, modeId, I, rawKey);
        if (not fileSystem::user::get_value(off, storage[I]))
        {
          return false;
        }
      }

      details::bit_cast<sizeof(output)>(output, storage);
      return true;
    }
    else
    {
      static_assert(sizeof(T) <= 64, "KeyStore values limited to 64 bytes!");
    }
    return false;
  }

  // \private maps to ContextTy::KeyProxy::getValue(defaultValue)
  template<EnumTy key, typename T> static inline void LMBD_INLINE getValue(T& output, T defValue)
  {
    if (not getValue<key, T>(output))
    {
      output = defValue;
    }
  }

  // \private maps to ContextTy::KeyProxy::hasValue
  template<EnumTy key, typename T = uint32_t> static inline bool LMBD_INLINE hasValue()
  {
    T output;
    return getValue<key, T>(output);
  }

  // \private empty store if \p storeId mismatch with value at \p storeIdKey
  template<uint32_t storeId> static void migrateStoreIfNeeded()
  {
    constexpr auto idx = indexKeyFor(prefix, groupId, modeId, 0, storeIdKey);

    // retrieve storeId from storage
    uint32_t otherId = 0;
    if (not fileSystem::user::get_value(idx, otherId))
    {
      otherId = storeId;
      fileSystem::user::set_value(idx, storeId);
    }

    // if it don't match our storeId, clean storage from our old keys
    if (otherId != storeId || otherId == 0)
    {
      constexpr uint32_t select = 0xfff00000; // all but offset
      constexpr uint32_t masked = idx & select;
      fileSystem::user::dropMatchingKeys(masked, select);
      fileSystem::user::set_value(idx, storeId);
    }
  }
};

/// \private
template<uint32_t baseId, typename AllModes> static constexpr uint32_t derivateStoreIdImpl()
{
  constexpr uint32_t MixCstA = 0x8af8901d, MixCstB = 0x2f9c7c90;
  constexpr uint8_t NbModes = std::tuple_size_v<AllModes>;

  uint32_t storeId = baseId;
  details::unroll<NbModes>([&](auto Idx) {
    constexpr uint8_t I = decltype(Idx)::value;
    using ModeHere = std::tuple_element_t<I, AllModes>;
    using StoreHere = StoreEnumOf<ModeHere>;
    constexpr bool hasStore = not std::is_same_v<StoreHere, NoStoreHere>;

    uint32_t storeIdHere = 0;
    if constexpr (hasStore)
    {
      storeIdHere = ModeHere::storeId;
    }

    // straightforward mix function
    storeId += MixCstA;
    storeId = (storeId << 7) ^ (storeId >> (32 - 7));
    storeId ^= MixCstB;

    storeId += storeIdHere;
  });

  return storeId;
}

/// \private Returns a new storeId, sensitive to all storeId of AllModes
template<uint32_t baseId, typename AllModes> static constexpr uint32_t derivateStoreId =
        derivateStoreIdImpl<baseId, AllModes>();

} // namespace modes::store

#endif
