/*
 *  Copyright (c) 2021 Uskrai
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <evtsigslot/signal.h>

#include <cstdio>

#include "evtsigslot/event.h"

class Base {
 public:
  virtual void Testing(evtsigslot::Event<int>& event) {
    printf("event:%d\n", event.Get());
    event.Skip();
  }
};

class Derived : public Base {
 public:
  virtual void Testing(evtsigslot::Event<int>& event) {
    printf("Derived:%d\n", event.Get());
  }

  virtual void Testing2(evtsigslot::Event<int> event) { printf("Testing2\n"); }
};

void Testing(evtsigslot::Event<int>& event) {
  printf("In Func:%d\n", event.Get());
}

void Over(int& eve) { printf("Over\n"); }
void Overr(int eve) { printf("Overr\n"); }

int main() {
  evtsigslot::Signal<int> signal;
  Base m;
  Derived d;
  signal.Bind(&Base::Testing, &m);
  signal.Bind(&Base::Testing, &d);
  signal.Bind(&Testing);
  signal.Unbind(&Base::Testing);
  signal.Unbind(&Testing);
  signal.Emit(3);
  signal.Emit(5);

  signal.Bind(&Derived::Testing2, &d);
  signal.Bind(&Over);
  signal.Bind(&Overr);

  int res = 1;
  signal.Emit(res);
  return 0;
}
