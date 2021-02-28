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

#include <atomic>

#ifndef EVTSIGSLOT_SLOT_STATE
#define EVTSIGSLOT_SLOT_STATE

namespace evtsigslot {

namespace detail {

class SlotState {
 public:
  SlotState() : binded_(true), blocked_(false) {}

  auto Index() const { return index_; }
  auto& Index() { return index_; }

  virtual bool IsBinded() const noexcept { return binded_; }
  virtual bool IsBlocked() const noexcept { return blocked_; }

  bool Unbind() noexcept {
    bool ret = binded_.exchange(false);
    if (ret) {
      OnDisconnect();
    }
    return ret;
  }

  void Block() noexcept { blocked_.store(true); }
  void Unblock() noexcept { blocked_.store(false); }

 protected:
  virtual void OnDisconnect() {}

 private:
  std::size_t index_;
  std::atomic_bool binded_, blocked_;
};

}  // namespace detail

}  // namespace evtsigslot

#endif /* end of include guard: EVTSIGSLOT_SLOT_STATE */
