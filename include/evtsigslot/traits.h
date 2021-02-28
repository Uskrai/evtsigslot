/*
 *  MIT License
 *
 *  Copyright (c) 2017 Pierre-Antoine Lacaze
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

#include <type_traits>

namespace evtsigslot {

namespace detail {

struct observer_type {};

}  // namespace detail

namespace trait {

/// represent a list of types
template <typename...>
struct typelist {};

namespace detail {

template <typename...>
struct voider {
  using type = void;
};

// void_t from c++17
template <typename... T>
using void_t = typename detail::voider<T...>::type;

template <typename, typename = void>
struct has_call_operator : std::false_type {};

template <typename F>
struct has_call_operator<
    F, void_t<decltype(&std::remove_reference<F>::type::operator())>>
    : std::true_type {};

template <typename, typename, typename = void, typename = void>
struct is_callable : std::false_type {};

template <typename F, typename P, typename... T>
struct is_callable<F, P, typelist<T...>,
                   void_t<decltype(((*std::declval<P>()).*std::declval<F>())(
                       std::declval<T>()...))>> : std::true_type {};

template <typename F, typename... T>
struct is_callable<F, typelist<T...>,
                   void_t<decltype(std::declval<F>()(std::declval<T>()...))>>
    : std::true_type {};

template <typename T, typename = void>
struct is_weak_ptr : std::false_type {};

template <typename T>
struct is_weak_ptr<T, void_t<decltype(std::declval<T>().expired()),
                             decltype(std::declval<T>().lock()),
                             decltype(std::declval<T>().reset())>>
    : std::true_type {};

template <typename T, typename = void>
struct is_weak_ptr_compatible : std::false_type {};

template <typename T>
struct is_weak_ptr_compatible<T, void_t<decltype(to_weak(std::declval<T>()))>>
    : is_weak_ptr<decltype(to_weak(std::declval<T>()))> {};

}  // namespace detail

static constexpr bool with_rtti =
#ifdef SIGSLOT_RTTI_ENABLED
    true;
#else
    false;
#endif

/// determine if a pointer is convertible into a "weak" pointer
template <typename P>
constexpr bool is_weak_ptr_compatible_v =
    detail::is_weak_ptr_compatible<std::decay_t<P>>::value;

/// determine if a type T (Callable or Pmf) is callable with supplied arguments
template <typename L, typename... T>
constexpr bool is_callable_v = detail::is_callable<T..., L>::value;

template <typename T>
constexpr bool is_weak_ptr_v = detail::is_weak_ptr<T>::value;

template <typename T>
constexpr bool has_call_operator_v = detail::has_call_operator<T>::value;

template <typename T>
constexpr bool is_pointer_v = std::is_pointer<T>::value;

template <typename T>
constexpr bool is_func_v = std::is_function<T>::value;

template <typename T>
constexpr bool is_pmf_v = std::is_member_function_pointer<T>::value;

template <typename T>
constexpr bool is_observer_v =
    std::is_base_of<::evtsigslot::detail::observer_type,
                    std::remove_pointer_t<T>>::value;

}  // namespace trait

}  // namespace evtsigslot
