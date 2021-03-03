/*
 *  MIT License
 *
 *  Copyright (c) 2021 Uskrai
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#ifndef EVTSIGSLOT_SLOT_TRAITS
#define EVTSIGSLOT_SLOT_TRAITS
#include <evtsigslot/slot.h>

namespace evtsigslot {

template <typename...>
struct slot_traits : std::false_type {};

template <typename Emitted, typename... Caller>
struct event_traits {
  static constexpr bool is_callable_without_event =
      trait::is_callable_v<std::add_lvalue_reference<Emitted>>;
};

template <typename Callable, typename Emitted>
struct slot_traits<Callable, Emitted> {
  static constexpr bool is_emit_void = std::is_same_v<void, Emitted>;

  static constexpr bool is_callable_without_args =
      trait::is_callable_v<trait::typelist<>, Callable>;

  static constexpr bool is_callable_without_event =
      trait::is_slot_callable_v<trait::typelist<Emitted>, Callable> ||
      is_callable_without_args;

  static constexpr bool is_callable_with_event =
      !is_callable_without_event &&
      trait::is_slot_callable_v<trait::typelist<Event<Emitted>>, Callable>;

  static constexpr bool value =
      !trait::is_pmf_v<Callable> &&
      (is_callable_with_event || is_callable_without_args ||
       is_callable_without_event);

  using type =
      SlotHelper<SlotFunc<Callable, Emitted>, slot_traits<Callable, Emitted>>;
};

template <typename Callable, typename Class, typename Emitted>
struct slot_traits<Callable, Class, Emitted> {
  using slot_raw_pointer = SlotClass<Callable, Class, Emitted>;
  using slot_weak_pointer = void;

  static constexpr bool is_raw_pointer = trait::is_pointer_v<Class>;

  static constexpr bool is_callable_without_args =
      trait::is_slot_callable_v<trait::typelist<>, Callable, Class>;

  static constexpr bool is_callable_without_event =
      is_callable_without_args ||
      trait::is_slot_callable_v<trait::typelist<Emitted>, Callable, Class>;

  static constexpr bool is_emit_void = std::is_same_v<Emitted, void>;

  static constexpr bool is_callable_with_event =
      !is_callable_without_event &&
      trait::is_slot_callable_v<trait::typelist<Event<Emitted>>, Callable,
                                Class>;

  static constexpr bool value =
      trait::is_pmf_v<Callable> &&
      (is_callable_with_event || is_callable_without_event ||
       is_callable_without_args);

  using type = SlotHelper<
      std::conditional_t<is_raw_pointer, slot_raw_pointer, slot_weak_pointer>,
      slot_traits<Callable, Class, Emitted>>;
};

template <typename... U>
constexpr bool slot_traits_value = slot_traits<U...>::value;

template <typename... U>
using slot_traits_type = typename slot_traits<U...>::type;

}  // namespace evtsigslot

#endif /* end of include guard: EVTSIGSLOT_SLOT_TRAITS */
