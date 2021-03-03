#include <evtsigslot/signal.h>

namespace {
#include "common.h"
}

#include <cassert>

#include "evtsigslot/event.h"

void test_skip_event() {
  // shouldn't be called because l2 is not skipped
  // and this is called last
  auto l1 = []() { assert(false); };
  auto l2 = [&](evtsigslot::Event<void> i) {};
  auto l3 = [] {};

  evtsigslot::Signal<> sig;

  sig.Bind(l1);
  sig.Bind(l2);
  sig.Bind(l3);
  sig();
}

void test_value_retain() {
  evtsigslot::Signal<int> sig;

  auto l1 = [](int &i) { assert(i == 2); };
  auto l2 = [](int &i) { i += 2; };

  sig.Bind(l1);
  sig.Bind(l2);

  sig(0);
}

int main() {
  // test_skip_event();
  test_value_retain();

  return 0;
}
