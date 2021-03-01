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

#ifndef EVTSIGSLOT_COPY_ON_WRITE
#define EVTSIGSLOT_COPY_ON_WRITE

#include <atomic>
#include <utility>

namespace evtsigslot {

namespace detail {

template <typename T>
class CopyOnWrite {
  struct Payload {
    Payload() = default;

    template <typename... Args>
    explicit Payload(Args&&... args) : value_(std::forward<Args>(args)...) {}

    std::atomic_size_t count_{1};
    T value_;
  };

  Payload* data_;

 public:
  using value_type = T;

  CopyOnWrite() : data_(new Payload){};

  template <typename U>
  explicit CopyOnWrite(
      U&& u,
      std::enable_if_t<!std::is_same<std::decay_t<U>, CopyOnWrite>::value>* =
          nullptr)
      : data_(new Payload(std::forward<U>(u))) {}

  CopyOnWrite(const CopyOnWrite& oth) noexcept : data_(oth.data_) {
    ++data_->count_;
  }

  CopyOnWrite(CopyOnWrite&& mv) noexcept : data_(mv.data_) {
    mv.data_ = nullptr;
  }

  ~CopyOnWrite() {
    if (data_ && --data_->count_ == 0) {
      delete data_;
    }
  }

  CopyOnWrite& operator=(const CopyOnWrite& oth) noexcept {
    if (&oth != this) {
      *this = CopyOnWrite(oth);
    }
    return *this;
  }

  CopyOnWrite& operator=(CopyOnWrite&& mv) noexcept {
    auto tmp = std::move(mv);
    swap(*this, tmp);
    return *this;
  }

  value_type& Write() {
    if (!IsUnique()) {
      *this = CopyOnWrite(Read());
    }
    return data_->value_;
  }

  const value_type& Read() { return data_->value_; }

  friend inline void swap(CopyOnWrite& x, CopyOnWrite& y) noexcept {
    std::swap(x.data_, y.data_);
  }

 private:
  bool IsUnique() const noexcept { return data_->count_ == 0; }
};

template <typename T>
const T& CowRead(const T& v) {
  return v;
}

template <typename T>
const T& CowRead(CopyOnWrite<T>& v) {
  return v.Read();
}

template <typename T>
T& CowWrite(T& v) {
  return v;
}

template <typename T>
T& CowWrite(CopyOnWrite<T>& v) {
  return v.Write();
}

}  // namespace detail

}  // namespace evtsigslot

#endif /* end of include guard: EVTSIGSLOT_COPY_ON_WRITE */
