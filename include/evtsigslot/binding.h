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

#ifndef EVTSIGSLOT_CONNECTION
#define EVTSIGSLOT_CONNECTION

#include <evtsigslot/slot_state.h>

#include <cstdio>
#include <memory>

namespace evtsigslot {

class BindingBlocker {
 public:
  BindingBlocker() = default;
  ~BindingBlocker() noexcept { Release(); }

  BindingBlocker(const BindingBlocker &) = delete;
  BindingBlocker &operator=(const BindingBlocker &) = delete;

  BindingBlocker(BindingBlocker &&o) noexcept : state_(std::move(o.state_)) {}
  BindingBlocker &operator=(BindingBlocker &&o) noexcept {
    Release();
    state_.swap(o.state_);
    return *this;
  }

 protected:
  friend class Binding;
  explicit BindingBlocker(std::weak_ptr<detail::SlotState> s) noexcept
      : state_(std::move(s)) {
    if (auto s = state_.lock()) s->Block();
  }

  void Release() noexcept {
    if (auto s = state_.lock()) s->Unblock();
  }

 private:
  std::weak_ptr<detail::SlotState> state_;
};

class Binding {
 public:
  Binding() = default;
  virtual ~Binding() = default;

  Binding(const Binding &) noexcept = default;
  Binding &operator=(const Binding &) noexcept = default;
  Binding(Binding &&) noexcept = default;
  Binding &operator=(Binding &&) noexcept = default;

  bool Valid() const noexcept { return !state_.expired(); }

  bool IsBinded() const noexcept {
    const auto s = state_.lock();
    return s && s->IsBinded();
  }

  bool Unbind() noexcept {
    auto s = state_.lock();
    return s && s->Unbind();
  }

  bool IsBlocked() const noexcept {
    const auto s = state_.lock();
    return s && s->IsBlocked();
  }

  void Block() noexcept {
    if (auto s = state_.lock()) s->Block();
  }

  void Unblock() noexcept {
    if (auto s = state_.lock()) s->Unblock();
  }

  BindingBlocker Blocker() noexcept { return BindingBlocker(state_); }

 protected:
  template <typename>
  friend class Signal;
  explicit Binding(std::weak_ptr<detail::SlotState> s) noexcept
      : state_(std::move(s)) {}

  std::weak_ptr<detail::SlotState> state_;
};

class ScopedBinding : public Binding {
 public:
  ScopedBinding() = default;
  ~ScopedBinding() { Unbind(); }

  ScopedBinding(const Binding &c) noexcept : Binding(c) {}
  ScopedBinding(Binding &&c) noexcept : Binding(c) {}

  ScopedBinding(const ScopedBinding &c) noexcept = delete;
  ScopedBinding &operator=(const ScopedBinding &c) noexcept = delete;

  ScopedBinding(ScopedBinding &&m) noexcept : Binding(std::move(m.state_)) {}
  ScopedBinding &operator=(ScopedBinding &&m) noexcept {
    Unbind();
    state_.swap(m.state_);
    return *this;
  }

 private:
  template <typename>
  friend class Signal;

  explicit ScopedBinding(std::weak_ptr<detail::SlotState> s) noexcept
      : Binding(std::move(s)) {}
};

}  // namespace evtsigslot

#endif /* end of include guard: EVTSIGSLOT_CONNECTION */
