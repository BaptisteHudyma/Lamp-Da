#ifndef USER_MODE_TOOLS_H
#define USER_MODE_TOOLS_H

#include <cstdint>
#include <utility>
#include <optional>
#include <tuple>

#include "src/modes/mode_type.hpp"

#ifndef LMBD_CPP17
#error "File requires -DLMBD_CPP17 to explicitly enables C++17 features"
#endif

// always_inline macro
#define LMBD_INLINE __attribute__((always_inline))

// maybe_unused macro
#define LMBD_USED [[maybe_unused]]

namespace modes {

struct NoState { };

template <typename Mode>
static constexpr bool isMode = std::is_base_of_v<BasicMode, Mode>;

namespace details {

/// \private Implements unroll<N>(callback)
template <class CbTy, uint8_t... Indexes>
static constexpr void LMBD_INLINE unroll_impl(
        CbTy&& cb,
        std::integer_sequence<uint8_t, Indexes...>) {
  (cb(std::integral_constant<uint8_t, Indexes>{}), ...);
}

/// \private Calls \p cb with integral constants from 0 to N
template <uint8_t N, class CbTy>
static constexpr void LMBD_INLINE unroll(CbTy &&cb) {
  constexpr auto Indexes = std::make_integer_sequence<uint8_t, N>();
  unroll_impl([&](auto I) LMBD_INLINE {
    return cb(std::integral_constant<uint8_t, decltype(I)::value>{});
  }, Indexes);
}

/// \private Helper to perform queries on elements of \p TupleTy
template <typename TupleTy, uint8_t TupleSz = std::tuple_size_v<TupleTy>>
struct forEach {

  /// \private Return True if any elements returns True through \p QueryStruct
  template <template<class> class QueryStruct, bool hasError = false>
  static constexpr bool any() {
    bool acc = false;
    if constexpr (!hasError) {
      unroll<TupleSz>([&](auto Idx) {
        acc |= QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value;
      });
    }
    return acc;
  }

  /// \private Return True if all elements returns True through \p QueryStruct
  template <template<class> class QueryStruct, bool hasError = false>
  static constexpr bool all() {
    bool acc = true;
    if constexpr (!hasError) {
      unroll<TupleSz>([&](auto Idx) {
        acc &= QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value;
      });
    } else {
     return false;
    }
    return acc;
  }

};

/// \private Defined boolean is True if any \p TupleTy item has boolean True
template <typename TupleTy, bool hasError = false>
struct anyOf {

  template <typename Ty>
  struct QHasBright { static constexpr bool value = Ty::hasBrightCallback; };

  template <typename Ty>
  struct QCustomRamp { static constexpr bool value = Ty::hasCustomRamp; };

  template <typename Ty>
  struct QButtonUI { static constexpr bool value = Ty::hasButtonCustomUI; };

  template <typename Ty>
  struct QHasSystem { static constexpr bool value = Ty::hasSystemCallbacks; };

  template <typename Ty>
  struct QUserThread { static constexpr bool value = Ty::requireUserThread; };

  // booleans we need
  static constexpr bool simpleMode = false;
  static constexpr bool hasBrightCallback = forEach<TupleTy>::template any<QHasBright, hasError>();
  static constexpr bool hasCustomRamp = forEach<TupleTy>::template any<QCustomRamp, hasError>();
  static constexpr bool hasButtonCustomUI = forEach<TupleTy>::template any<QButtonUI, hasError>();
  static constexpr bool hasSystemCallbacks = forEach<TupleTy>::template any<QHasSystem, hasError>();
  static constexpr bool requireUserThread = forEach<TupleTy>::template any<QUserThread, hasError>();
};

//
// StateTyOf definition:
//  - returns StateTy if defined
//  - or else return EmptyState
//

template <typename Mode,
         typename StateTy = typename Mode::StateTy,
         bool isDefault = std::is_same_v<StateTy, BasicMode::StateTy> || (sizeof(StateTy) == 0),
         typename RetTy = std::conditional_t<isDefault, NoState, StateTy>>
static constexpr auto stateTyOfImpl(int) -> RetTy;

template <typename>
static constexpr auto stateTyOfImpl(...) -> NoState;

/// \private Get StateTy if explicitly defined, or else EmptyState
template <typename Mode>
using StateTyOf = decltype(stateTyOfImpl<Mode>(0));

/// \private Defined boolean is True if all \p TupleTy item has boolean True
template <typename TupleTy, bool hasError = false>
struct allOf {

  template <typename Ty>
  struct QIsMode { static constexpr bool value = isMode<Ty>; };

  template <typename Ty>
  struct QStateOk { static constexpr bool value = std::is_default_constructible_v<StateTyOf<Ty>>; };

  static constexpr bool inheritsFromBasicMode = forEach<TupleTy>::template all<QIsMode, hasError>();
  static constexpr bool stateDefaultConstructible = forEach<TupleTy>::template all<QStateOk, hasError>();
};

/// \private Get std::tuple<Modes::StateTy...> from Modes...
template <typename... Modes>
using StateTyFor = std::tuple<std::optional<StateTyOf<Modes>>...>;

template <typename... Modes>
static constexpr auto stateTyFromImpl(std::tuple<Modes...>*) -> StateTyFor<Modes...>;

/// \private Get std::tuple<Mode::StateTy...> from std::tuple<Mode...>
template <typename AsTuple>
using StateTyFrom = decltype(stateTyFromImpl((AsTuple*) 0));

template <typename Item, typename... Items>
static constexpr bool belongsToImpl(std::tuple<Items...>*) {
  return (std::is_same_v<Item, Items> || ...);
};

/// \private Checks if \p Mode is member of \p AllModes as tuple
template <typename Mode, typename AllModes>
static constexpr bool ModeBelongsTo = belongsToImpl<Mode>((AllModes*) 0);

/// \private Checks if \p Group is member of \p AllGroups as tuple
template <typename Group, typename AllGroups>
static constexpr bool GroupBelongsTo = belongsToImpl<Group>((AllGroups*) 0);

template <typename Mode, typename AllGroups>
static constexpr bool testIfModeExistsImpl() {
  constexpr uint8_t NbGroups{std::tuple_size_v<AllGroups>};

  bool acc = false;
  unroll<NbGroups>([&](auto Idx) {
    using GroupHere = std::tuple_element_t<Idx, AllGroups>;
    using AllModes = typename GroupHere::AllModesTy;
    acc |= ModeBelongsTo<Mode, AllModes>;
  });

  return acc;
}

/// \private Checks if \p Mode exists in the set of \p AllGroups
template <typename Mode, typename AllGroups>
static constexpr bool ModeExists = testIfModeExistsImpl<Mode, AllGroups>();

} // namespace details

/// \private see details::StateTyOf
template <typename Mode>
using StateTyOf = details::StateTyOf<Mode>;

} // namespace modes

#endif
