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
#include <evtsigslot/slot_traits.h>

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
  using event_type = Event<Emitted>;
  using event_ptr = std::unique_ptr<event_type>;
  using arg_list = event_type&;
  cow_type<std::list<group_type>, Lockable> slot_list_;

  std::queue<event_ptr> queue_event_;
  Lockable slot_mutex_, queue_mutex_;
  std::atomic_bool block_;

  inline static size_t default_handler_limit_ = 1;
  size_t handler_limit_ = default_handler_limit_;
  std::atomic_size_t handler_;

  using locker_type = std::scoped_lock<std::mutex>;

  static constexpr bool is_emit_void = std::is_same_v<Emitted, void>;

 public:
  Signal() : block_(false), handler_(0) {}
  Signal(const Signal&) = delete;
  Signal& operator=(const Signal&) = delete;

  Signal(Signal&& m) : block_(m.block_.load()) {
    locker_type lock(m.slot_mutex_);
    handler_.exchange(m.handler_.load());
    std::swap(slot_list_, m.slot_list_);
  }

  ~Signal() { UnbindAll(); }

  Signal& operator=(Signal&& m) {
    locker_type lock(slot_mutex_, m.slot_mutex_);

    handler_.exchange(m.handler_.load());
    swap(slot_list_, m.slot_list_);
    block_.store(m.block_.exchange(block_.load()));
  }

  /**
   * @brief: Alias for Emit compatible argument
   * @param: Callable and object
   */
  template <typename... T>
  using emit_void_return =
      std::enable_if_t<std::is_constructible_v<Emitted, T...> ||
                           std::is_same_v<Emitted, void>,
                       void>;

  template <typename... T>
  emit_void_return<T...> Queue(T&&... val) {
    if (block_) return;

    {
      locker_type queue_locker(queue_mutex_);
      queue_event_.emplace(
          std::make_unique<event_type>(std::forward<T>(val)...));
    }

    ProcessEvent();
  }

  template <typename... T>
  emit_void_return<T...> PostEvent(T&&... t) {
    PostEvent(std::forward<T>(t)...);
  }

  void PostEvent(event_type& event) {
    cow_copy_type<list_type, Lockable> ref = SlotReference();

    for (const auto& group : detail::CowRead(ref)) {
      for (const auto& slot : group.list) {
        event.Skip(false);
        slot->operator()(event);
        if (!event.IsSkipped()) break;
      }
    }
  }

  template <typename... T>
  emit_void_return<T...> operator()(T&&... val) {
    Queue(std::forward<T>(val)...);
  }

  void ProcessEvent(bool force = false) {
    {
      locker_type locker(queue_mutex_);
      if (!force && handler_.load() < handler_limit_)
        handler_.store(handler_ + 1);
      else
        return;
    }

    struct handler_decrement {
      std::atomic_size_t& atom_;
      bool decrement_;
      handler_decrement(std::atomic_size_t& atom, bool should_decrement)
          : atom_(atom), decrement_(should_decrement) {}
      ~handler_decrement() { atom_.store(atom_.load() - 1); }
    };

    handler_decrement decrement(handler_, !force);

    while (true) {
      std::unique_ptr<Event<Emitted>> event;
      {
        locker_type queue_locker(queue_mutex_);
        if (queue_event_.empty()) break;
        event = std::move(queue_event_.front());
        queue_event_.pop();
      }
      PostEvent(*event);
    }
  }
  template <typename... Caller>
  using slot_traits_def = slot_traits<trait::typelist<Emitted>, Caller...>;

  template <typename... Caller>
  static constexpr bool is_callable_v = slot_traits_def<Caller...>::value;

  template <typename... Caller>
  using slot_caller_type = typename slot_traits_def<Caller...>::type;

  template <typename Callable, typename Class>
  std::enable_if_t<is_callable_v<Callable, Class>, Binding> Bind(
      Callable&& callable, Class&& class_ptr) {
    auto slot = std::make_shared<slot_caller_type<Callable, Class>>(
        static_cast<Cleanable&>(*this), std::forward<Callable>(callable),
        std::forward<Class>(class_ptr));
    Binding bind(slot);
    AddSlot(std::move(slot));
    return bind;
  }

  template <typename Callable>
  std::enable_if_t<is_callable_v<Callable>, Binding> Bind(Callable&& callable) {
    auto slot = std::make_shared<slot_caller_type<Callable>>(
        static_cast<Cleanable&>(*this), std::forward<Callable>(callable));
    Binding bind(slot);
    AddSlot(std::move(slot));
    return bind;
  }

  // template <typename Callable, typename Class>
  // std::enable_if_t<is_callable_v<Callable, Class> &&
  // !trait::is_observer_v<Class> &&
  // !trait::is_weak_ptr_compatible_v<Class>,
  // Binding>
  // Bind(Callable&& callable, Class&& class_ptr) {
  // auto slot = MakeSlot<Callable, Class, Emitted>(
  // *this, std::forward<Callable>(callable),
  // std::forward<Class>(class_ptr));
  // Binding bind(slot);
  // AddSlot(std::move(slot));
  // return bind;
  // }
  //
  // template <typename Callable>
  // std::enable_if_t<is_callable_v<Callable>, Binding> Bind(Callable&&
  // callable) { auto slot = MakeSlot<Callable, Emitted>(*this,
  // std::forward<Callable>(callable));
  // Binding bind(slot);
  // AddSlot(std::move(slot));
  // return bind;
  // }

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
  std::enable_if_t<(is_callable_v<Callable> || trait::is_pmf_v<Callable>),
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
  std::enable_if_t<is_callable_v<Callable, Object> || trait::is_pmf_v<Callable>,
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
  std::enable_if_t<!is_callable_v<Class> && trait::is_pointer_v<Class> &&
                       !trait::is_pmf_v<Class>,
                   size_t>
  Unbind(const Class& class_ptr) {
    auto ret =
        DoUnbindIf([&](const auto& it) { return it->HasObject(class_ptr); });
    return ret;
  }

  void UnbindAll() {
    locker_type locker(slot_mutex_);
    detail::CowWrite(slot_list_).clear();
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

  size_t CountQueue() noexcept { return queue_event_.size(); }

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

    it->list.push_front(std::move(slot));
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
  }
};

}  // namespace evtsigslot

#endif /* end of include guard: FMR_EVTSIGSLOT_SIGNAL */
