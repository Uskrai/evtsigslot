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

using trait::detail::void_t;

enum SlotFlags {
  kIsCallableWithEvent,
  kIsCallableWithoutEvent,
  kIsCallableWithoutArgs
};

/**
 * Base for SlotTraits
 * @param: EmittedList should be type with trait::typelist
 * @param: Second and/or third should be the Caller
 */
template <typename EmittedList, typename, typename = void, typename = void>
struct slot_traits : std::false_type {};

/**
 * @brief: SlotTraits Helper class for defining Conditional Value Slot Type
 */
template <SlotFlags flags, typename SlotType, typename = void, typename = void>
struct slot_traits_helper {
  static constexpr bool is_callable_with_event = flags == kIsCallableWithEvent;
  static constexpr bool is_callable_without_event =
      flags == kIsCallableWithoutEvent || flags == kIsCallableWithoutArgs;
  static constexpr bool is_callable_without_args =
      flags == kIsCallableWithoutArgs;

  static constexpr bool value = true;
  using type = SlotHelper<SlotType, slot_traits_helper<flags, SlotType>>;
};

/**
 * @brief: SlotTraitCaller Helper for defining New Caller ( Function, Member
 * Function, Member function of smart pointer )
 *
 * @param: EmittedList should be type with trait::typelist
 * @param: Second and Third should be the Caller
 */
template <typename EmittedList, typename, typename = void, typename = void>
struct slot_traits_caller_helper : std::false_type {};

/**
 * @brief: SlotTraitCaller Helper for Checking Event argument
 */
template <typename EmittedList, typename... CallerList>
constexpr bool is_slot_event_callable =
    trait::is_slot_callable_v<trait::typelist<Event<EmittedList>>,
                              CallerList...>;

template <typename EmittedList, typename SlotType, typename... CallerList>
struct slot_traits_caller_helper<
    trait::typelist<EmittedList>, SlotType, trait::typelist<CallerList...>,
    typename std::enable_if<is_slot_event_callable<EmittedList, CallerList...>,
                            void>::type>
    : slot_traits_helper<kIsCallableWithEvent, SlotType> {};

/**
 * @brief: SlotTraitCaller Helper for checking void Argument
 */
template <typename Emitted, typename SlotType, typename... CallerList>
struct slot_traits_caller_helper<
    Emitted, SlotType, trait::typelist<CallerList...>,
    typename std::enable_if<
        trait::is_slot_callable_v<trait::typelist<>, CallerList...>,
        void>::type> : slot_traits_helper<kIsCallableWithoutArgs, SlotType> {};

template <typename EmittedList, typename SlotType, typename... CallerList>
struct slot_traits_without_event_helper
    : slot_traits_helper<
          kIsCallableWithoutEvent,
          typename std::enable_if<trait::is_slot_callable_v<
              trait::typelist<EmittedList>, CallerList...>>::type> {};

/**
 * @brief: SlotTraitCaller Helper for checking EmittedList argument
 */
template <typename EmittedList, typename SlotType, typename... CallerList>
struct slot_traits_caller_helper<
    trait::typelist<EmittedList>, SlotType, trait::typelist<CallerList...>,
    typename std::enable_if_t<
        !is_slot_event_callable<EmittedList, CallerList...> &&
            trait::is_slot_callable_v<trait::typelist<EmittedList>,
                                      CallerList...>,
        void>> : slot_traits_helper<kIsCallableWithoutEvent, SlotType> {};

/**
 * @brief: SltoTrait Helper for Converting Emitted to typelist in
 * slot_traits_caller_helper
 */
template <typename EmittedList, typename SlotType, typename... Caller>
using slot_traits_caller_helper_lister =
    slot_traits_caller_helper<trait::typelist<EmittedList>, SlotType,
                              trait::typelist<Caller...>>;

/**
 * @brief: SlotTraits for Function
 */
template <typename Callable, typename EmittedList>
struct slot_traits<
    trait::typelist<EmittedList>, Callable,
    typename std::enable_if<
        slot_traits_caller_helper_lister<
            EmittedList, SlotFunc<Callable, EmittedList>, Callable>::value,
        void>::type>
    : slot_traits_caller_helper_lister<
          EmittedList, SlotFunc<Callable, EmittedList>, Callable> {};

/**
 * @brief: SlotTraits for Member Function
 */
template <typename Callable, typename Class, typename EmittedList>
struct slot_traits<trait::typelist<EmittedList>, Callable, Class,
                   typename std::enable_if<
                       slot_traits_caller_helper_lister<
                           EmittedList, SlotClass<Callable, Class, EmittedList>,
                           Callable, Class>::value,
                       void>::type>
    : slot_traits_caller_helper_lister<EmittedList,
                                       SlotClass<Callable, Class, EmittedList>,
                                       Callable, Class> {};

template <typename... U>
constexpr bool slot_traits_value = slot_traits<U...>::value;

template <typename... U>
using slot_traits_type = typename slot_traits<U...>::type;

}  // namespace evtsigslot

#endif /* end of include guard: EVTSIGSLOT_SLOT_TRAITS */
