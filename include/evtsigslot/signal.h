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

#ifndef FMR_EVTSIGSLOT_SIGNAL
#define FMR_EVTSIGSLOT_SIGNAL

#include <evtsigslot/binding.h>
#include <evtsigslot/copy_on_write.h>
#include <evtsigslot/event.h>
#include <evtsigslot/group.h>

#include <atomic>
#include <mutex>
#include <queue>
#include <type_traits>

namespace evtsigslot {

template <typename Emitted>
class Signal : Cleanable {
 protected:
  Group<Emitted> group_;
  std::mutex mutex_;
  std::queue<Event<Emitted>> queue_event_;

  using locker_type = std::scoped_lock<std::mutex>;

  template <typename L>
  using is_thread_safe = std::integral_constant<bool, true>;

  template <typename U, typename L>
  using cow_type =
      std::conditional_t<is_thread_safe<L>::value, detail::CopyOnWrite<U>, U>;

  template <typename U, typename L>
  using cow_copy_type =
      std::conditional_t<is_thread_safe<L>::value, detail::CopyOnWrite<U>, U>;

  using list_type = typename Group<Emitted>::vector_type;
  using arg_list = trait::typelist<Event<Emitted>&>;

  using Lockable = std::mutex;

 public:
  Signal() : block_(false) {}
  Signal(const Signal&) = delete;
  Signal& operator=(const Signal&) = delete;

  Signal(Signal&& m) : block_(m.block_.load()) {
    locker_type lock(m.slot_mutex_);
    std::swap(group_, m.group_);
  }

  Signal& operator=(Signal&& m) {
    locker_type lock(mutex_, m.mutex_);

    swap(group_, m.group_);
    block_.store(m.block_.exchange(block_.load()));
  }

  template <typename Callable, typename Class>
  std::enable_if_t<trait::is_callable_v<arg_list, Callable, Class> &&
                       !trait::is_observer_v<Class> &&
                       !trait::is_weak_ptr_compatible_v<Class>,
                   Binding>
  Bind(Callable&& callable, Class&& class_ptr) {
    auto slot = MakeSlot<Callable, Class, Emitted>(
        *this, std::forward<Callable>(callable),
        std::forward<Class>(class_ptr));
    Binding bind(slot);
    group_.AddSlot(std::move(slot));
    return bind;
  }

  template <typename Callable>
  std::enable_if_t<trait::is_callable_v<arg_list, Callable>, Binding> Bind(
      Callable&& callable) {
    auto slot =
        MakeSlot<Callable, Emitted>(*this, std::forward<Callable>(callable));
    Binding bind(slot);
    group_.AddSlot(std::move(slot));
    return bind;
  }

  template <typename... Args>
  ScopedBinding BindScoped(Args&&... args) {
    return Bind(std::forward<Args>(args)...);
  }

  /**
   * Unbind all slot binded to Callable
   *
   * @param: callable a callable
   *
   * @return: Unbinded slot
   */
  template <typename Callable>
  std::enable_if_t<trait::is_callable_v<arg_list, Callable> ||
                       trait::is_pmf_v<Callable>,
                   size_t>
  Unbind(const Callable& callable) {
    return DoUnbindIf(
        [&](const auto& it) { return it->HasCallable(callable); });
  }

  /**
   * Unbind all slot binded to the callable and the object
   *
   * @param: callable a callable
   * @param: obj an object
   *
   * @return: Unbind slot
   */
  template <typename Callable, typename Object>
  std::enable_if_t<trait::is_callable_v<arg_list, Callable, Object> &&
                       trait::is_pmf_v<Callable> && std::is_pointer_v<Object>,
                   size_t>
  Unbind(const Callable& callable, const Object& obj) {
    return DoUnbindIf([&](const auto& it) {
      return it->HasObject(obj) && it->HasCallable(callable);
    });
  }

  /**
   * Unbind all slot binded to the object
   *
   * @param: class_ptr obj pointer
   *
   * @return: Unbinded slot
   */
  template <typename Class>
  std::enable_if_t<
      !trait::is_callable_v<arg_list, Class> && !trait::is_pmf_v<Class>, size_t>
  Unbind(const Class& class_ptr) {
    return DoUnbindIf([&](const auto& it) { return it->HasObject(class_ptr); });
  }

  void UnbindAll() {
    while (group_.Size() != 0) group_.GetVector().pop_back();
  }

  template <typename... T>
  using emit_void_return =
      std::enable_if_t<std::is_constructible_v<Emitted, T...>, void>;

  template <typename... T>
  emit_void_return<T...> Emit(T&&... val) {
    if (block_) return;

    Event<Emitted> event(std::forward<T>(val)...);

    for (auto it : group_.Get()) {
      it->operator()(event);
      if (!event.IsSkipped()) break;
      event.Skip(false);
    }
  }

  template <typename... T>
  emit_void_return<T...> operator()(T&&... val) {
    Emit(std::forward<T>(val)...);
  }

  void Block() noexcept { block_.store(true); }
  void Unblock() noexcept { block_.store(false); }

  size_t CountSlot() const { return group_.Size(); }

 private:
  inline cow_copy_type<list_type, Lockable> SlotReference() {
    locker_type locker(slot_mutex_);
    return group_;
  }

  template <typename Cond>
  size_t DoUnbindIf(Cond func) {
    locker_type locker(slot_mutex_);
    auto& vec = group_.GetVector();
    size_t idx = 0, count = 0;

    while (idx < vec.size()) {
      if (func(vec[idx])) {
        group_.RemoveSlot(vec[idx].get());
        ++count;
      } else
        ++idx;
    }

    return count;
  }

  void Clean(detail::SlotState* state) override {
    locker_type locker(slot_mutex_);
    group_.RemoveSlot(state);
  }

  Lockable slot_mutex_;
  std::atomic_bool block_;
};

}  // namespace evtsigslot

#endif /* end of include guard: FMR_EVTSIGSLOT_SIGNAL */
