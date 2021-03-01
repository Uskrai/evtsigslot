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
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <type_traits>

namespace evtsigslot {

template <typename Emitted = void>
class Signal : Cleanable {
 protected:
  using Lockable = std::mutex;

  template <typename L>
  using is_thread_safe = std::integral_constant<bool, true>;

  template <typename U, typename L>
  using cow_type =
      std::conditional_t<is_thread_safe<L>::value, detail::CopyOnWrite<U>, U>;

  template <typename U, typename L>
  using cow_copy_type = std::conditional_t<is_thread_safe<L>::value,
                                           detail::CopyOnWrite<U>, const U&>;

  using slot_type = Slot<Emitted>;
  using slot_ptr = std::shared_ptr<slot_type>;
  using slot_container = std::list<slot_ptr>;

  struct group_type {
    slot_container list;
    int id = 0;
  };

  using list_type = std::list<group_type>;
  using arg_list = trait::typelist<Event<Emitted>&>;
  cow_type<std::list<group_type>, Lockable> slot_list_;

  std::queue<Event<Emitted>> queue_event_;
  Lockable slot_mutex_, queue_mutex_;
  std::atomic_bool block_, handling_;

  using locker_type = std::scoped_lock<std::mutex>;

  static constexpr bool is_emit_void = std::is_same_v<Emitted, void>;

 public:
  Signal() : block_(false), handling_(false) {}
  Signal(const Signal&) = delete;
  Signal& operator=(const Signal&) = delete;

  Signal(Signal&& m) : block_(m.block_.load()) {
    locker_type lock(m.slot_mutex_);
    handling_.store(m.handling_.load());
    std::swap(slot_list_, m.slot_list_);
  }

  ~Signal() { UnbindAll(); }

  Signal& operator=(Signal&& m) {
    locker_type lock1(slot_mutex_, m.slot_mutex_);

    swap(slot_list_, m.slot_list_);
    block_.store(m.block_.exchange(block_.load()));
  }

  template <typename... Caller>
  static constexpr bool is_callable_v =
      trait::is_callable_v<arg_list, Caller...> ||
      (is_emit_void && trait::is_callable_v<trait::typelist<>, Caller...>);

  template <typename Callable, typename Class>
  std::enable_if_t<is_callable_v<Callable, Class> &&
                       !trait::is_observer_v<Class> &&
                       !trait::is_weak_ptr_compatible_v<Class>,
                   Binding>
  Bind(Callable&& callable, Class&& class_ptr) {
    auto slot = MakeSlot<Callable, Class, Emitted>(
        *this, std::forward<Callable>(callable),
        std::forward<Class>(class_ptr));
    Binding bind(slot);
    AddSlot(std::move(slot));
    return bind;
  }

  template <typename Callable>
  std::enable_if_t<is_callable_v<Callable>, Binding> Bind(Callable&& callable) {
    auto slot =
        MakeSlot<Callable, Emitted>(*this, std::forward<Callable>(callable));
    Binding bind(slot);
    AddSlot(std::move(slot));
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
    locker_type locker(slot_mutex_);
    detail::CowWrite(slot_list_).clear();
  }

  template <typename... T>
  using emit_void_return =
      std::enable_if_t<std::is_constructible_v<Emitted, T...> ||
                           std::is_same_v<Emitted, void>,
                       void>;
  template <typename... T>
  emit_void_return<T...> Emit(T&&... val) {
    if (block_) return;

    struct atomic_setter {
      std::atomic_bool& atom_;
      bool val_;
      atomic_setter(std::atomic_bool& atom, bool val)
          : atom_(atom), val_(val) {}
      ~atomic_setter() { atom_.store(val_); }
    };

    {
      locker_type queue_locker(queue_mutex_);
      queue_event_.emplace(std::forward<T>(val)...);

      if (!handling_.load()) {
        handling_.store(true);
      } else
        return;
    }

    atomic_setter handling_setter(handling_, false);

    size_t idx = 0;
    while (true) {
      cow_copy_type<list_type, Lockable> ref = SlotReference();
      auto& read = detail::CowRead(ref);

      auto& event = queue_event_.front();
      for (const auto& group : read) {
        for (const auto& slot : group.list) {
          event.Skip(false);
          slot->operator()(event);
          if (!event.IsSkipped()) break;
        }
      }

      locker_type queue_locker(queue_mutex_);
      queue_event_.pop();

      if (queue_event_.empty()) {
        break;
      }
    }
  }

  template <typename... T>
  emit_void_return<T...> operator()(T&&... val) {
    Emit(std::forward<T>(val)...);
  }

  void Block() noexcept { block_.store(true); }
  void Unblock() noexcept { block_.store(false); }

  size_t CountSlot() noexcept {
    cow_copy_type<list_type, Lockable> ref = SlotReference();
    size_t count = 0;
    for (const auto& group : detail::CowRead(ref)) {
      count += group.list.size();
    }
    return count;
  }

 private:
  inline cow_copy_type<list_type, Lockable> SlotReference() {
    locker_type locker(slot_mutex_);
    return slot_list_;
  }

  void AddSlot(slot_ptr&& slot) {
    locker_type locker(slot_mutex_);

    auto& group_list = detail::CowWrite(slot_list_);

    typename list_type::iterator it =
        std::find_if(group_list.begin(), group_list.end(),
                     [&](auto& it) { return it.id == slot->group_id_; });

    if (it == group_list.end()) {
      group_type group;
      group.id = slot->group_id_;
      group_list.emplace_back(group);

      it = std::find_if(
          group_list.begin(), group_list.end(),
          [&](const group_type& it) { return it.id == slot->group_id_; });
    }

    it->list.push_back(std::move(slot));
  }

  template <typename Cond>
  size_t DoUnbindIf(Cond func) {
    locker_type locker(slot_mutex_);
    auto& ref = detail::CowWrite(slot_list_);
    size_t idx = 0, count = 0;

    for (auto group = detail::CowWrite(slot_list_).begin();
         group != detail::CowWrite(slot_list_).end(); ++group) {
      auto it = group->list.begin();
      while (it != group->list.end())
        if (func(*it)) {
          it = group->list.erase(it);
          ++count;
        } else
          ++it;
    }

    return count;
  }

  void Clean(detail::SlotState* state) override {
    locker_type locker(slot_mutex_);

    static int i = 0;
    auto& write = detail::CowWrite(slot_list_);
    for (auto group = write.begin(); group != write.end(); ++group) {
      for (auto it = group->list.begin(); it != group->list.end(); ++it) {
        if (it->get() == state) {
          group->list.erase(it);
          return;
        }
      }
    }

    // for (auto it = write.Get().begin(); it != write.Get().end(); ++it) {
    // if ((*it).get() == state) {
    // write.Get().erase(it);
    // return;
    // }
    // }
  }
};

}  // namespace evtsigslot

#endif /* end of include guard: FMR_EVTSIGSLOT_SIGNAL */
