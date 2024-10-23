#ifndef USER_MODE_TOOLS_H
#define USER_MODE_TOOLS_H

#include <cstdint>
#include <utility>

#include "mode_type.h"

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

  // usual suspects
  static constexpr bool simpleMode = false;
  static constexpr bool hasBrightCallback = forEach<TupleTy>::template any<QHasBright, hasError>();
  static constexpr bool hasCustomRamp = forEach<TupleTy>::template any<QCustomRamp, hasError>();
  static constexpr bool hasButtonCustomUI = forEach<TupleTy>::template any<QButtonUI, hasError>();
  static constexpr bool hasSystemCallbacks = forEach<TupleTy>::template any<QHasSystem, hasError>();
  static constexpr bool requireUserThread = forEach<TupleTy>::template any<QUserThread, hasError>();
};

/// \private Defined boolean is True if all \p TupleTy item has boolean True
template <typename TupleTy, bool hasError = false>
struct allOf {

  template <typename Ty>
  struct QIsMode { static constexpr bool value = isMode<Ty>; };

  static constexpr bool inheritsFromBasicMode = forEach<TupleTy>::template all<QIsMode, hasError>();
};


template <typename Mode,
         typename StateTy = typename Mode::StateTy,
         bool isDefault = std::is_same_v<StateTy, BasicMode::StateTy>,
         typename RetTy = std::conditional_t<isDefault, NoState, StateTy>>
static constexpr auto stateTyOfImpl(int) -> RetTy;

template <typename>
static constexpr auto stateTyOfImpl(...) -> NoState;

/// \private Get StateTy if explicitly defined, or else EmptyState
template <typename Mode>
using StateTyOf = decltype(stateTyOfImpl<Mode>(0));

/// \private Get std::tuple<Modes::StateTy...> from Modes...
template <typename... Modes>
using StateTyFor = std::tuple<StateTyOf<Modes>...>;

template <typename... Modes>
static constexpr auto stateTyFromImpl(std::tuple<Modes...>*) -> StateTyFor<Modes...>;

/// \private Get std::tuple<Mode::StateTy...> from std::tuple<Mode...>
template <typename AsTuple>
using StateTyFrom = decltype(stateTyFromImpl((AsTuple*) 0));

} // namespace details

} // namespace modes

#endif
