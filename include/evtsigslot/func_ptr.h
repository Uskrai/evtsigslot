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

#include <evtsigslot/traits.h>

#include <algorithm>
#include <iterator>
#include <memory>

#if defined __clang__ || (__GNUC__ > 5)
#define SIGSLOT_MAY_ALIAS __attribute__((__may_alias__))
#else
#define SIGSLOT_MAY_ALIAS
#endif

namespace evtsigslot {

/**
 * The following function_traits and object_pointer series of templates are
 * used to circumvent the type-erasing that takes place in the slot_base
 * implementations. They are used to compare the stored functions and objects
 * with another one for disconnection purpose.
 */

/*
 * Function pointers and member function pointers size differ from compiler to
 * compiler, and for virtual members compared to non virtual members. On some
 * compilers, multiple inheritance has an impact too. Hence, we form an union
 * big enough to store any kind of function pointer.
 */

namespace mock {

struct a {
  virtual ~a() = default;
  void f();
  virtual void g();
};
struct b {
  virtual ~b() = default;
  virtual void h();
};
struct c : a, b {
  void g() override;
};

union fun_types {
  decltype(&c::g) m;
  decltype(&a::g) v;
  decltype(&a::f) d;
  void (*f)();
  void* o;
};

}  // namespace mock

/*
 * This union is used to compare function pointers
 * Generic callables cannot be compared. Here we compare pointers but there is
 * no guarantee that this always works.
 */

union SIGSLOT_MAY_ALIAS func_ptr {
  void* value() { return &data[0]; }

  const void* value() const { return &data[0]; }

  template <typename T>
  T& value() {
    return *static_cast<T*>(value());
  }

  template <typename T>
  const T& value() const {
    return *static_cast<const T*>(value());
  }

  inline explicit operator bool() const { return value() != nullptr; }

  inline bool operator==(const func_ptr& o) const {
    return std::equal(std::begin(data), std::end(data), std::begin(o.data));
  }

  mock::fun_types _;
  char data[sizeof(mock::fun_types)];
};

template <typename T, typename = void>
struct function_traits {
  static void ptr(const T& /*t*/, func_ptr& d) {
    d.value<std::nullptr_t>() = nullptr;
  }

  static constexpr bool is_disconnectable = false;
  static constexpr bool must_check_object = true;
};

template <typename T>
struct function_traits<T, std::enable_if_t<trait::is_func_v<T>>> {
  static void ptr(T& t, func_ptr& d) { d.value<T*>() = &t; }

  static constexpr bool is_disconnectable = true;
  static constexpr bool must_check_object = false;
};

template <typename T>
struct function_traits<T*, std::enable_if_t<trait::is_func_v<T>>> {
  static void ptr(T* t, func_ptr& d) { d.value<T*>() = t; }

  static constexpr bool is_disconnectable = true;
  static constexpr bool must_check_object = false;
};

template <typename T>
struct function_traits<T, std::enable_if_t<trait::is_pmf_v<T>>> {
  static void ptr(const T& t, func_ptr& d) { d.value<T>() = t; }

  static constexpr bool is_disconnectable = trait::with_rtti;
  static constexpr bool must_check_object = true;
};

// for function objects, the assumption is that we are looking for the call
// operator
template <typename T>
struct function_traits<T, std::enable_if_t<trait::has_call_operator_v<T>>> {
  using call_type = decltype(&std::remove_reference<T>::type::operator());

  static void ptr(const T& /*t*/, func_ptr& d) {
    function_traits<call_type>::ptr(&T::operator(), d);
  }

  static constexpr bool is_disconnectable =
      function_traits<call_type>::is_disconnectable;
  static constexpr bool must_check_object =
      function_traits<call_type>::must_check_object;
};

template <typename T>
func_ptr get_function_ptr(const T& t) {
  func_ptr d;
  std::uninitialized_fill(std::begin(d.data), std::end(d.data), '\0');
  function_traits<std::decay_t<T>>::ptr(t, d);
  return d;
}

/*
 * obj_ptr is used to store a pointer to an object.
 * The object_pointer traits are needed to handle trackable objects correctly,
 * as they are likely to not be pointers.
 */
using obj_ptr = const void*;

template <typename T>
obj_ptr get_object_ptr(const T& t);

template <typename T, typename = void>
struct object_pointer {
  static obj_ptr get(const T&) { return nullptr; }
};

}  // namespace evtsigslot
