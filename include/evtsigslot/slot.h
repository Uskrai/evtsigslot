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

#ifndef FMR_EVTSIGSLOT_SLOT
#define FMR_EVTSIGSLOT_SLOT

#include <evtsigslot/event.h>
#include <evtsigslot/func_ptr.h>
#include <evtsigslot/slot_state.h>

#include <memory>

namespace evtsigslot {

struct Cleanable {
  virtual ~Cleanable() = default;
  virtual void Clean(detail::SlotState*) = 0;
};

template <typename Emitted>
class Slot : public detail::SlotState {
 public:
  Slot(Cleanable& c) : cleaner_(c) {}

  void operator()(Event<Emitted>& val) {
    bool is_blocked = IsBlocked();
    if (IsBinded() && !is_blocked) {
      DoCall(val);
    } else
      val.Skip();
  };

  using event_type = Event<Emitted>;
  using value_type = Emitted;

  virtual func_ptr GetCallable() = 0;

  virtual bool HasObject(const void* obj) = 0;

  template <typename T>
  bool HasCallable(T&& t) {
    auto func_ptr = get_function_ptr(t);
    auto this_ptr = GetCallable();

    return func_ptr && this_ptr && func_ptr == this_ptr;
  }

 protected:
  virtual void OnDisconnect() override { cleaner_.Clean(this); }

  virtual void DoCall(event_type& event) = 0;

 private:
  Cleanable& cleaner_;
};

template <typename Callable, typename Class, typename Emitted>
class SlotClass : public Slot<Emitted> {
  Class class_ptr_;
  Callable callable_;

 public:
  SlotClass(Cleanable& c, Callable&& callable, Class class_ptr)
      : Slot<Emitted>(c),
        callable_(std::forward<Callable>(callable)),
        class_ptr_(class_ptr) {}

  using typename Slot<Emitted>::event_type;
  using typename Slot<Emitted>::value_type;

  virtual Class GetClassPtr() { return class_ptr_; }
  virtual func_ptr GetCallable() { return get_function_ptr(callable_); }

  static constexpr bool is_emit_void = std::is_same_v<Emitted, void>;
  static constexpr bool is_callable_without_event =
      trait::is_callable_v<trait::typelist<Emitted>, Callable, Class>;

  virtual void DoCall(event_type& val) override {
    if constexpr (!is_callable_without_event && !is_emit_void) {
      (GetClassPtr()->*callable_)(val);
    } else {
      if constexpr (!is_emit_void) {
        (GetClassPtr()->*callable_)(val.Get());
      } else {
        (GetClassPtr()->*callable_)(val);
      }
      val.Skip();
    }
  }

  virtual bool HasObject(const void* obj) override { return obj == class_ptr_; }
};

template <typename Callable, typename Emitted>
class SlotFunc : public Slot<Emitted> {
  Callable callable_;

 public:
  SlotFunc(Cleanable& c, Callable&& callable)
      : Slot<Emitted>(c), callable_{std::forward<Callable>(callable)} {}

  using typename Slot<Emitted>::event_type;
  using typename Slot<Emitted>::value_type;

  static constexpr bool is_emit_void = std::is_same_v<Emitted, void>;
  static constexpr bool is_callable_without_arg =
      trait::is_callable_v<trait::typelist<>, Callable>;
  static constexpr bool is_callable_without_event =
      trait::is_callable_v<trait::typelist<Emitted>, Callable>;

  virtual void DoCall(event_type& val) override {
    if constexpr (!is_callable_without_event && !is_callable_without_arg) {
      callable_(val);
    } else {
      if constexpr (!is_emit_void) {
        callable_(val.Get());
      } else {
        callable_();
      }
      val.Skip();
    }
  }

  virtual func_ptr GetCallable() override {
    return get_function_ptr(callable_);
  }

  virtual bool HasObject(const void* obj) override { return false; }
};

template <typename Callable, typename Class, typename... Emitted>
constexpr std::shared_ptr<Slot<Emitted...>> MakeSlot(Cleanable& cleanable,
                                                     Callable&& callable,
                                                     Class class_ptr) {
  using SlotType = SlotClass<Callable, Class, Emitted...>;
  return std::make_shared<SlotType>(cleanable, std::forward<Callable>(callable),
                                    class_ptr);
}

/**
 * Make shared pointer Slot for Function or lambda
 *
 * @param: cleanable an Cleanable object
 *
 * @return: Slot<Emitted> shared pointer
 */
template <typename Callable, typename... Emitted>
constexpr std::shared_ptr<Slot<Emitted...>> MakeSlot(Cleanable& cleanable,
                                                     Callable&& callable) {
  using SlotType = SlotFunc<Callable, Emitted...>;
  return std::make_shared<SlotType>(cleanable,
                                    std::forward<Callable>(callable));
}

}  // namespace evtsigslot

#endif /* end of include guard: FMR_EVTSIGSLOT_SLOT */
