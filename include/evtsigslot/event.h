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
#include <variant>

namespace evtsigslot {

namespace internal {
class EmptyEvent {
  bool is_skipped_, vetoed_;

 public:
  EmptyEvent() : is_skipped_(false), vetoed_(false){};

  void Skip(bool skip = true) { is_skipped_ = skip; }
  bool IsSkipped() const { return is_skipped_; }

  void Veto() { vetoed_ = true; }
  bool IsAllowed() const { return vetoed_; }
};
//
}  // namespace internal

template <typename Emitted>
class Event : public internal::EmptyEvent {
  Emitted val_;

 public:
  template <typename... U>
  explicit Event(U&&... val) : val_(std::forward<U>(val)...) {}

  Emitted& dGet() { return val_; }
  const Emitted& Get() const { return val_; }

  operator Emitted&() { return val_; }
  operator const Emitted&() const { return val_; }
};

template <>
class Event<void> : public internal::EmptyEvent {};

}  // namespace evtsigslot

namespace std {

// template <std::size_t I, class... Types>
// constexpr std::variant_alternative_t<I, std::variant<Types...>>& get(
// evtsigslot::Event<Types...>& v) {
// return std::get<I>(std::forward<Types...>(v.Get()));
// };
//
// template <std::size_t I, class... Types>
// constexpr std::variant_alternative_t<I, std::variant<Types...>>&& get(
// evtsigslot::Event<Types...>&& v) {
// return std::get<I>(std::tuple<Types...>(std::forward<Types...>(v.Get())));
// }

}  // namespace std

#endif /* end of include guard: FMR_EVTSIGSLOT_EVENT */
