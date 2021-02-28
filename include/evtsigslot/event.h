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

#ifndef FMR_EVTSIGSLOT_EVENT
#define FMR_EVTSIGSLOT_EVENT

#include <utility>

namespace evtsigslot {

template <typename Emitted>
class Event {
  Emitted val_;
  bool is_skipped_ = false, vetoed_ = false;

 public:
  template <typename... U>
  explicit Event(U&&... val) : val_(std::forward<U>(val)...) {}

  Emitted& Get() { return val_; }
  const Emitted& Get() const { return val_; }

  operator Emitted&() { return val_; }
  operator const Emitted&() const { return val_; }

  void Skip(bool skip = true) { is_skipped_ = skip; }
  bool IsSkipped() const { return is_skipped_; }

  void Veto() { vetoed_ = true; }
  bool IsAllowed() const { return vetoed_; }
};

}  // namespace evtsigslot

#endif /* end of include guard: FMR_EVTSIGSLOT_EVENT */
