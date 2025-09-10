#ifndef USER_MODE_TOOLS_H
#define USER_MODE_TOOLS_H

#include <cstdint>
#include <cstring>
#include <utility>
#include <optional>
#include <tuple>

#include "src/modes/include/compile.hpp"
#include "src/modes/include/mode_type.hpp"

#ifndef LMBD_CPP17
#error "File requires -DLMBD_CPP17 to explicitly enables C++17 features"
#endif

namespace modes {

/// \private True iff Mode is a BasicMode
template<typename Mode> static constexpr bool is_mode = std::is_base_of_v<BasicMode, Mode>;

/**
 * \brief Compute a linear scale interpolation between bounds. approximation is unconstrained, and if the input is not
 * between bounds, the outputs will not respect them eiher
 *
 * \param[in] x Value to interpolate
 * \param[in] in_min Min bound of value to interpolate
 * \param[in] in_max Max bound of value to interpolate
 * \param[in] out_min Min bound of result
 * \param[in] out_max Max bound of result
 *
 */
template<typename T, typename U>
static constexpr U LMBD_INLINE linear_scale(T x, T in_min, T in_max, U out_min, U out_max)
{
  return (out_max - out_min) * ((x - in_min) / float(in_max - in_min)) + out_min;
}

//
// StateTyOf
//  - returns StateTy if defined
//  - or else return EmptyState
//

/// \private Placeholder type for when no state is available
struct NoState
{
};

/// \private
template<typename Mode,
         typename StateTy = typename Mode::StateTy,
         bool isDefault = std::is_same_v<StateTy, BasicMode::StateTy> || (sizeof(StateTy) == 0),
         typename RetTy = std::conditional_t<isDefault, NoState, StateTy>>
static constexpr auto stateTyOfImpl(int) -> RetTy;

/// \private
template<typename> static constexpr auto stateTyOfImpl(...) -> NoState;

/// \private Get StateTy if explicitly defined, or else an empty NoState
template<typename Mode> using StateTyOf = decltype(stateTyOfImpl<Mode>(0));

//
// Store
//

/// \private Tag meaning that no store has been enabled here
struct NoStoreId
{
};

/// \private Placeholder Enum for when no store is available
enum class NoStoreHere : uint16_t
{
  noKeyDefined // placeholder (should not be used)
};

/// \private
template<typename Mode,
         typename EnumTy = typename Mode::Store,
         bool isDefault = (Mode::storeId == BasicMode::storeId),
         typename RetTy = std::conditional_t<isDefault, NoStoreHere, EnumTy>>
static constexpr auto storeEnumOfImpl(int) -> RetTy;

/// \private
template<typename> static constexpr auto storeEnumOfImpl(...) -> NoStoreHere;

/// \private Get \p Store type if explicitly defined, or else \p NoStoreHere
template<typename Mode> using StoreEnumOf = decltype(storeEnumOfImpl<Mode>(0));

/// \private
namespace details {

//
// unroll
//

/// \private Implements unroll<N>(callback)
template<class CbTy, uint8_t... Indexes>
static constexpr void LMBD_INLINE unroll_impl(CbTy&& cb, std::integer_sequence<uint8_t, Indexes...>)
{
  (cb(std::integral_constant<uint8_t, Indexes> {}), ...);
}

/// \private Calls \p cb with integral constants from 0 to N
template<uint8_t N, class CbTy> static constexpr void LMBD_INLINE unroll(CbTy&& cb)
{
  constexpr auto Indexes = std::make_integer_sequence<uint8_t, N>();
  unroll_impl(
          [&](auto I) LMBD_INLINE {
            return cb(std::integral_constant<uint8_t, decltype(I)::value> {});
          },
          Indexes);
}

//
// forEach & anyOf & allOf
//

/// \private Helper to perform queries on elements of \p TupleTy
template<typename TupleTy, uint8_t TupleSz = std::tuple_size_v<TupleTy>> struct forEach
{
  /// \private Return True if any elements returns True through \p QueryStruct
  template<template<class> class QueryStruct, bool hasError = false> static constexpr bool any()
  {
    bool acc = false;
    if constexpr (!hasError)
    {
      unroll<TupleSz>([&](auto Idx) {
        acc |= QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value;
      });
    }
    return acc;
  }

  /// \private Return True if all elements returns True through \p QueryStruct
  template<template<class> class QueryStruct, bool hasError = false> static constexpr bool all()
  {
    bool acc = true;
    if constexpr (!hasError)
    {
      unroll<TupleSz>([&](auto Idx) {
        acc &= QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value;
      });
    }
    else
    {
      return false;
    }
    return acc;
  }

  /// \private Return a std::array of all bool given as input via \p QueryStruct
  template<template<class> class QueryStruct, bool hasError = false> static constexpr auto asTable()
  {
    // collect all values as table
    std::array<bool, TupleSz> acc {};
    if constexpr (!hasError)
    {
      unroll<TupleSz>([&](auto Idx) {
        constexpr size_t I = decltype(Idx)::value;
        acc[I] = QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value;
      });
    }
    return acc;
  }

  /// \private Return the maximal size of all std::array queried via \p QueryStruct
  template<template<class> class QueryStruct, bool hasError = false> static constexpr size_t getMaxTableSize()
  {
    size_t acc = 0;
    if constexpr (!hasError)
    {
      unroll<TupleSz>([&](auto Idx) {
        size_t sz = QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value.size();
        if (sz > acc)
          acc = sz;
      });
    }
    return acc;
  }

  /// \private Return a std::array<std::array> of the table of all table via \p QueryStruct
  template<template<class> class QueryStruct, size_t MaxSz, bool hasError = false> static constexpr auto asTable2D()
  {
    // collect all values as table
    std::array<std::array<bool, MaxSz>, TupleSz> acc {};
    if constexpr (!hasError)
    {
      unroll<TupleSz>([&](auto Idx) {
        constexpr size_t I = decltype(Idx)::value;

        auto& remote = QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value;
        for (size_t J = 0; J < remote.size() && J < MaxSz; ++J)
        {
          acc[I][J] = remote[J];
        }
      });
    }
    return acc;
  }
};

/// \private Defined boolean is True if any \p TupleTy item has boolean True
template<typename TupleTy, bool hasError = false> struct anyOf
{
  template<typename Ty> struct QHasBright
  {
    static constexpr bool value = Ty::hasBrightCallback;
  };

  template<typename Ty> struct QCustomRamp
  {
    static constexpr bool value = Ty::hasCustomRamp;
  };

  template<typename Ty> struct QButtonUI
  {
    static constexpr bool value = Ty::hasButtonCustomUI;
  };

  template<typename Ty> struct QHasSystem
  {
    static constexpr bool value = Ty::hasSystemCallbacks;
  };

  template<typename Ty> struct QUserThread
  {
    static constexpr bool value = Ty::requireUserThread;
  };

  // booleans we need
  static constexpr bool hasBrightCallback = forEach<TupleTy>::template any<QHasBright, hasError>();
  static constexpr bool hasCustomRamp = forEach<TupleTy>::template any<QCustomRamp, hasError>();
  static constexpr bool hasButtonCustomUI = forEach<TupleTy>::template any<QButtonUI, hasError>();
  static constexpr bool hasSystemCallbacks = forEach<TupleTy>::template any<QHasSystem, hasError>();
  static constexpr bool requireUserThread = forEach<TupleTy>::template any<QUserThread, hasError>();

  // as table
  static constexpr auto everyBrightCallback = forEach<TupleTy>::template asTable<QHasBright, hasError>();
  static constexpr auto everyCustomRamp = forEach<TupleTy>::template asTable<QCustomRamp, hasError>();
  static constexpr auto everyButtonCustomUI = forEach<TupleTy>::template asTable<QButtonUI, hasError>();
  static constexpr auto everySystemCallbacks = forEach<TupleTy>::template asTable<QHasSystem, hasError>();
  static constexpr auto everyRequireUserThread = forEach<TupleTy>::template asTable<QUserThread, hasError>();
};

/// \private As 2D table for all important booleans we need
template<typename TupleTy, bool hasError = false> struct asTableFor
{
  template<typename Ty> struct QHasBright
  {
    static constexpr auto value = Ty::everyBrightCallback;
  };

  template<typename Ty> struct QCustomRamp
  {
    static constexpr auto value = Ty::everyCustomRamp;
  };

  template<typename Ty> struct QButtonUI
  {
    static constexpr auto value = Ty::everyButtonCustomUI;
  };

  template<typename Ty> struct QHasSystem
  {
    static constexpr auto value = Ty::everySystemCallbacks;
  };

  template<typename Ty> struct QUserThread
  {
    static constexpr auto value = Ty::everyRequireUserThread;
  };

  static constexpr size_t maxTableSz = forEach<TupleTy>::template getMaxTableSize<QHasBright, hasError>();

  static constexpr auto everyBrightCallback = forEach<TupleTy>::template asTable2D<QHasBright, maxTableSz, hasError>();
  static constexpr auto everyCustomRamp = forEach<TupleTy>::template asTable2D<QCustomRamp, maxTableSz, hasError>();
  static constexpr auto everyButtonCustomUI = forEach<TupleTy>::template asTable2D<QButtonUI, maxTableSz, hasError>();
  static constexpr auto everySystemCallbacks = forEach<TupleTy>::template asTable2D<QHasSystem, maxTableSz, hasError>();
  static constexpr auto everyRequireUserThread =
          forEach<TupleTy>::template asTable2D<QUserThread, maxTableSz, hasError>();
};

/// \private Defined boolean is True if all \p TupleTy item has boolean True
template<typename TupleTy, bool hasError = false> struct allOf
{
  template<typename Ty> struct QIsMode
  {
    static constexpr bool value = is_mode<Ty>;
  };

  template<typename Ty> struct QStateOk
  {
    static constexpr bool value = std::is_default_constructible_v<StateTyOf<Ty>>;
  };

  static constexpr bool inheritsFromBasicMode = forEach<TupleTy>::template all<QIsMode, hasError>();
  static constexpr bool stateDefaultConstructible = forEach<TupleTy>::template all<QStateOk, hasError>();
};

//
// StateTyFor & StateTyFrom
//

/// \private Get std::tuple<Modes::StateTy...> from Modes...
template<typename... Modes> using StateTyFor = std::tuple<std::optional<StateTyOf<Modes>>...>;

template<typename... Modes> static constexpr auto stateTyFromImpl(std::tuple<Modes...>*) -> StateTyFor<Modes...>;

/// \private Get std::tuple<Mode::StateTy...> from std::tuple<Mode...>
template<typename AsTuple> using StateTyFrom = decltype(stateTyFromImpl((AsTuple*)0));

//
// ModeBelongsTo & GroupBelongsTo
//

template<typename Item, typename... Items> static constexpr bool belongsToImpl(std::tuple<Items...>*)
{
  return (std::is_same_v<Item, Items> || ...);
};

/// \private Checks if \p Mode is member of \p AllModes as tuple
template<typename Mode, typename AllModes> static constexpr bool ModeBelongsTo = belongsToImpl<Mode>((AllModes*)0);

/// \private Checks if \p Group is member of \p AllGroups as tuple
template<typename Group, typename AllGroups> static constexpr bool GroupBelongsTo = belongsToImpl<Group>((AllGroups*)0);

//
// GroupIdFrom & ModeIdFrom
//

template<typename Mode, typename AllGroups> static constexpr int groupIdFromImpl()
{
  int acc = -1;
  constexpr uint8_t N = std::tuple_size_v<AllGroups>;
  unroll<N>([&](auto Idx) {
    constexpr uint8_t groupId = decltype(Idx)::value;
    using GroupHere = std::tuple_element_t<groupId, AllGroups>;
    using AllModesHere = typename GroupHere::AllModesTy;
    if (acc != -1)
      return;

    // make GroupIdFrom<Group, AllGroups> return the proper groupId
    if constexpr (std::is_same_v<Mode, GroupHere>)
    {
      acc = groupId;

      // save groupId if Mode was found in Group
    }
    else if constexpr (ModeBelongsTo<Mode, AllModesHere>)
    {
      acc = groupId;
    }
  });
  return acc;
}

template<typename Mode, typename AllGroups, int groupId> static constexpr int modeIdFromImpl()
{
  if constexpr (groupId < 0)
  {
    return -1;
  }
  else
  {
    using GroupHere = std::tuple_element_t<groupId, AllGroups>;
    using AllModesHere = typename GroupHere::AllModesTy;

    // early exit for calls to ModeIdFrom<Group, AllGroups, groupId>
    if (std::is_same_v<Mode, GroupHere>)
    {
      return -1;
    }

    int acc = -1;
    constexpr uint8_t N = std::tuple_size_v<AllModesHere>;
    unroll<N>([&](auto Idx) {
      constexpr uint8_t modeId = decltype(Idx)::value;
      using ModeHere = std::tuple_element_t<modeId, AllModesHere>;
      if (acc != -1)
        return;

      // save modeId if Mode found at that position in Group
      if constexpr (std::is_same_v<Mode, ModeHere>)
      {
        acc = modeId;
      }
    });

    return acc;
  }
}

/// \private Return group number where \p Mode was found (or -1 if not found)
template<typename Mode, typename AllGroups> static constexpr int GroupIdFrom = groupIdFromImpl<Mode, AllGroups>();

/// \private Return position of \p Mode within its group (or -1 if not found)
template<typename Mode, typename AllGroups, int GroupId = GroupIdFrom<Mode, AllGroups>>
static constexpr int ModeIdFrom = modeIdFromImpl<Mode, AllGroups, GroupId>();

//
// ModeExists
//

template<typename Mode, typename AllGroups> static constexpr bool testIfModeExistsImpl()
{
  constexpr uint8_t NbGroups {std::tuple_size_v<AllGroups>};

  bool acc = false;
  unroll<NbGroups>([&](auto Idx) {
    using GroupHere = std::tuple_element_t<Idx, AllGroups>;
    using AllModes = typename GroupHere::AllModesTy;
    acc |= ModeBelongsTo<Mode, AllModes>;
  });

  return acc;
}

/// \private Checks if \p Mode exists in the set of \p AllGroups
template<typename Mode, typename AllGroups> static constexpr bool ModeExists = testIfModeExistsImpl<Mode, AllGroups>();

/// \private Reliably lookup what is LocalStore alias in Ctx
template<typename Ctx> using LocalStoreOf = typename std::remove_cv_t<std::remove_reference_t<Ctx>>::LocalStore;

/** \private Relaxed implementation of C++20 std::bit_cast
 *
 * Adapted from https://en.cppreference.com/w/cpp/numeric/bit_cast
 */
template<size_t NBytes, class To, class From>
static inline std::enable_if_t<NBytes <= sizeof(To) && NBytes <= sizeof(From) && std::is_trivially_copyable_v<To> &&
                                       std::is_trivially_copyable_v<From>,
                               void>
        LMBD_INLINE bit_cast(To& dst, const From& src) noexcept
{
  std::memcpy(&dst, &src, NBytes);
}

} // namespace details

} // namespace modes

#endif
