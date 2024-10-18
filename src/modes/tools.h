#ifndef USER_MODE_TOOLS_H
#define USER_MODE_TOOLS_H

#include <cstdint>
#include <utility>

// always_inline macro
#define LMBD_INLINE __attribute__((always_inline))

namespace modes {

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

  /// \private Return True if all elements returns True through \p QueryStruct
  template <template<class> class QueryStruct>
  static constexpr bool any() {
    bool acc = false;
    unroll<TupleSz>([&](auto Idx) {
      acc |= QueryStruct<std::tuple_element_t<Idx, TupleTy>>::value;
    });
    return acc;
  }

};

/// \private Defined boolean is True if any \p TupleTy item has boolean True
template <typename TupleTy>
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
  static constexpr bool hasBrightCallback = forEach<TupleTy>::template any<QHasBright>();
  static constexpr bool hasCustomRamp = forEach<TupleTy>::template any<QCustomRamp>();
  static constexpr bool hasButtonCustomUI = forEach<TupleTy>::template any<QButtonUI>();
  static constexpr bool hasSystemCallbacks = forEach<TupleTy>::template any<QHasSystem>();
  static constexpr bool requireUserThread = forEach<TupleTy>::template any<QUserThread>();
};

} // namespace details

} // namespace modes

#endif
