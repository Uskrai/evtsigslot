#include <evtsigslot/signal.h>

#include <cassert>
#include <cmath>
#include <sstream>
#include <string>

static int sum = 0;

void f1(int i) { sum += i; }
void f2(int i) noexcept { sum += 2 * i; }

struct s {
  static void s1(int i) { sum += i; }
  static void s2(int i) noexcept { sum += 2 * i; }

  void f1(int i) { sum += i; }
  void f2(int i) const { sum += i; }
  void f3(int i) volatile { sum += i; }
  void f4(int i) const volatile { sum += i; }
  void f5(int i) noexcept { sum += i; }
  void f6(int i) const noexcept { sum += i; }
  void f7(int i) volatile noexcept { sum += i; }
  void f8(int i) const volatile noexcept { sum += i; }
};

struct oo {
  void operator()(int i) { sum += i; }
  void operator()(double i) { sum += int(std::round(4 * i)); }
};

struct o1 {
  void operator()(int i) { sum += i; }
};
struct o2 {
  void operator()(int i) const { sum += i; }
};
struct o3 {
  void operator()(int i) volatile { sum += i; }
};
struct o4 {
  void operator()(int i) const volatile { sum += i; }
};
struct o5 {
  void operator()(int i) noexcept { sum += i; }
};
struct o6 {
  void operator()(int i) const noexcept { sum += i; }
};
struct o7 {
  void operator()(int i) volatile noexcept { sum += i; }
};
struct o8 {
  void operator()(int i) const volatile noexcept { sum += i; }
};

void test_slot_count() {
  evtsigslot::Signal<int> sig;
  s p;

  sig.Bind(&s::f1, &p);
  assert(sig.CountSlot() == 1);
  sig.Bind(&s::f2, &p);
  assert(sig.CountSlot() == 2);
  sig.Bind(&s::f3, &p);
  assert(sig.CountSlot() == 3);
  sig.Bind(&s::f4, &p);
  assert(sig.CountSlot() == 4);
  sig.Bind(&s::f5, &p);
  assert(sig.CountSlot() == 5);
  sig.Bind(&s::f6, &p);
  assert(sig.CountSlot() == 6);

  {
    evtsigslot::ScopedBinding conn = sig.Bind(&s::f7, &p);
    assert(sig.CountSlot() == 7);
  }
  assert(sig.CountSlot() == 6);

  auto conn = sig.Bind(&s::f8, &p);
  assert(sig.CountSlot() == 7);
  conn.Unbind();
  assert(sig.CountSlot() == 6);

  sig.UnbindAll();
  assert(sig.CountSlot() == 0);
}

void test_free_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  auto c1 = sig.Bind(f1);
  sig(1);
  assert(sum == 1);

  sig.Bind(f2);
  sig(1);
  assert(sum == 4);
}

void test_static_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  sig.Bind(&s::s1);
  sig(1);
  assert(sum == 1);

  sig.Bind(&s::s2);
  sig(1);
  assert(sum == 4);
}

void test_pmf_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;
  s p;

  sig.Bind(&s::f1, &p);
  sig.Bind(&s::f2, &p);
  sig.Bind(&s::f3, &p);
  sig.Bind(&s::f4, &p);
  sig.Bind(&s::f5, &p);
  sig.Bind(&s::f6, &p);
  sig.Bind(&s::f7, &p);
  sig.Bind(&s::f8, &p);

  sig(1);
  assert(sum == 8);
}

void test_const_pmf_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;
  const s p;

  sig.Bind(&s::f2, &p);
  sig.Bind(&s::f4, &p);
  sig.Bind(&s::f6, &p);
  sig.Bind(&s::f8, &p);

  sig(1);
  assert(sum == 4);
}

void test_function_object_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  sig.Bind(o1{});
  sig.Bind(o2{});
  sig.Bind(o3{});
  sig.Bind(o4{});
  sig.Bind(o5{});
  sig.Bind(o6{});
  sig.Bind(o7{});
  sig.Bind(o8{});

  sig(1);
  assert(sum == 8);
}

void test_overloaded_function_object_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;
  evtsigslot::Signal<double> sig1;

  sig.Bind(oo{});
  sig(1);
  assert(sum == 1);

  sig1.Bind(oo{});
  sig1(1);
  assert(sum == 5);
}

void test_lambda_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  sig.Bind([&](int i) { sum += i; });
  sig(1);
  assert(sum == 1);

  sig.Bind([&](int i) mutable { sum += 2 * i; });
  sig(1);
  assert(sum == 4);
}

void test_generic_lambda_connection() {
  // std::stringstream s;
  //
  // auto f = [&](auto &a, auto... args) {
  // using result_t = int[];
  // s << a.Get();
  // result_t r{
  // 1,
  // ((void)(s << args), 1)...,
  // };
  // (void)r;
  // };
  //
  // evtsigslot::Signal<int> sig1;
  // evtsigslot::Signal<std::string> sig2;
  // evtsigslot::Signal<double> sig3;
  //
  // sig1.Bind(f);
  // sig2.Bind(f);
  // sig3.Bind(f);
  // sig1(1);
  // sig2("foo");
  // sig3(4.1);
  //
  // assert(s.str() == "1foo4.1");
}

void test_lvalue_emission() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  auto c1 = sig.Bind(f1);
  int v = 1;
  sig(v);
  assert(sum == 1);

  sig.Bind(f2);
  sig(v);
  assert(sum == 4);
}

void test_mutation() {
  int res = 0;
  evtsigslot::Signal<int &> sig;

  sig.Bind([](int &r) { r += 1; });
  sig(res);
  assert(res == 1);

  sig.Bind([](int &r) mutable { r += 2; });
  sig(res);
  assert(res == 4);
}

void test_compatible_args() {
  long ll = 0;
  std::string ss;
  short ii = 0;

  struct LongStringShort {
    LongStringShort(long lv, const std::string &st, short it) {
      l = lv;
      s = st;
      i = it;
    }
    long l;
    std::string s;
    short i;
  };

  auto f = [&](evtsigslot::Event<LongStringShort> &evt) {
    ll = evt.Get().l;
    ss = evt.Get().s;
    ii = evt.Get().i;
  };

  evtsigslot::Signal<LongStringShort> sig;
  sig.Bind(f);
  sig('0', "foo", true);

  assert(ll == 48);
  assert(ss == "foo");
  assert(ii == 1);
}
//
void test_disconnection() {
  // test removing only connected
  {
    sum = 0;
    evtsigslot::Signal<int> sig;

    auto sc = sig.Bind(f1);
    sig(1);
    assert(sum == 1);

    sc.Unbind();
    sig(1);
    assert(sum == 1);
    assert(!sc.Valid());
  }

  // test removing first connected
  {
    sum = 0;
    evtsigslot::Signal<int> sig;

    auto sc = sig.Bind(f1);
    sig(1);
    assert(sum == 1);

    sig.Bind(f2);
    sig(1);
    assert(sum == 4);

    sc.Unbind();
    sig(1);
    assert(sum == 6);
    assert(!sc.Valid());
  }

  // test removing last connected
  {
    sum = 0;
    evtsigslot::Signal<int> sig;

    sig.Bind(f1);
    sig(1);
    assert(sum == 1);

    auto sc = sig.Bind(f2);
    sig(1);
    assert(sum == 4);

    sc.Unbind();
    sig(1);
    assert(sum == 5);
    assert(!sc.Valid());
  }
}

void test_disconnection_by_callable() {
  // disconnect a function pointer
  {
    sum = 0;
    evtsigslot::Signal<int> sig;

    sig.Bind(f1);
    sig.Bind(f2);
    sig.Bind(f2);
    sig(1);
    assert(sum == 5);
    auto c = sig.Unbind(&f2);
    assert(c == 2);
    sig(1);
    assert(sum == 6);
  }

  // disconnect a function
  {
    sum = 0;
    evtsigslot::Signal<int> sig;

    sig.Bind(f1);
    sig.Bind(f2);
    sig(1);
    assert(sum == 3);
    sig.Unbind(f1);
    sig(1);
    assert(sum == 5);
  }

#ifdef SIGSLOT_RTTI_ENABLED
  // disconnect by pmf
  {
    sum = 0;
    evtsigslot::Signal<int> sig;
    s p;

    sig.Bind(&s::f1, &p);
    sig.Bind(&s::f2, &p);
    sig(1);
    assert(sum == 2);
    sig.Unbind(&s::f1);
    sig(1);
    assert(sum == 3);
  }

  // disconnect by function object
  {
    sum = 0;
    evtsigslot::Signal<int> sig;

    sig.Bind(o1{});
    sig.Bind(o2{});
    sig(1);
    assert(sum == 2);
    sig.Unbind(o1{});
    sig(1);
    assert(sum == 3);
  }

  // disconnect by lambda
  {
    sum = 0;
    evtsigslot::Signal<int> sig;
    auto l1 = [&](int i) { sum += i; };
    auto l2 = [&](int i) { sum += 2 * i; };
    sig.Bind(l1);
    sig.Bind(l2);
    sig(1);
    assert(sum == 3);
    sig.Unbind(l1);
    sig(1);
    assert(sum == 5);
  }
#endif
}

void test_disconnection_by_object() {
  // disconnect by pointer
  {
    printf("Start\n");
    sum = 0;
    evtsigslot::Signal<int> sig;
    s p1, p2;

    sig.Bind(&s::f1, &p1);
    sig.Bind(&s::f2, &p2);
    sig(1);
    assert(sum == 2);
    sig.Unbind(&p1);
    sig(1);
    assert(sum == 3);
  }

  // disconnect by shared pointer
  // {
  // sum = 0;
  // evtsigslot::Signal<int> sig;
  // auto p1 = std::make_shared<s>();
  // s p2;
  //
  // sig.Bind(&s::f1, p1);
  // sig.Bind(&s::f2, &p2);
  // sig(1);
  // assert(sum == 2);
  // sig.Unbind(p1);
  // sig(1);
  // assert(sum == 3);
  // }
}

void test_disconnection_by_object_and_pmf() {
  // disconnect by pointer
  {
    sum = 0;
    evtsigslot::Signal<int> sig;
    s p1, p2;

    sig.Bind(&s::f1, &p1);
    sig.Bind(&s::f1, &p2);
    sig.Bind(&s::f2, &p1);
    sig.Bind(&s::f2, &p2);
    sig(1);
    assert(sum == 4);
    sig.Unbind(&s::f1, &p2);
    sig(1);
    assert(sum == 7);
  }

  // disconnect by shared pointer
  // {
  // sum = 0;
  // evtsigslot::Signal<int> sig;
  // auto p1 = std::make_shared<s>();
  // auto p2 = std::make_shared<s>();
  //
  // sig.Bind(&s::f1, p1);
  // sig.Bind(&s::f1, p2);
  // sig.Bind(&s::f2, p1);
  // sig.Bind(&s::f2, p2);
  // sig(1);
  // assert(sum == 4);
  // // sig.Unbind(&s::f1, p2);
  // sig(1);
  // assert(sum == 7);
  // }

  // disconnect by tracker
  // {
  // sum = 0;
  // evtsigslot::Signal<int> sig;
  //
  // auto t = std::make_shared<bool>();
  // sig.Bind(f1);
  // sig.Bind(f2);
  // sig.Bind(f1, t);
  // sig.Bind(f2, t);
  // sig(1);
  // assert(sum == 6);
  // sig.Unbind(f2, t);
  // sig(1);
  // assert(sum == 10);
  // }
}

void test_scoped_connection() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  {
    auto sc1 = sig.BindScoped(f1);
    sig(1);
    assert(sum == 1);

    auto sc2 = sig.BindScoped(f2);
    sig(1);
    assert(sum == 4);
  }

  sig(1);
  assert(sum == 4);

  sum = 0;

  {
    evtsigslot::ScopedBinding sc1 = sig.Bind(f1);
    sig(1);
    assert(sum == 1);

    auto sc2 = sig.BindScoped(f2);
    sig(1);
    assert(sum == 4);
  }

  sig(1);
  assert(sum == 4);
}

void test_connection_blocking() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  auto c1 = sig.Bind(f1);
  sig.Bind(f2);
  sig(1);
  assert(sum == 3);

  c1.Block();
  sig(1);
  assert(sum == 5);

  c1.Unblock();
  sig(1);
  assert(sum == 8);
}

void test_connection_blocker() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  auto c1 = sig.Bind(f1);
  sig.Bind(f2);
  sig(1);
  assert(sum == 3);

  {
    auto cb = c1.Blocker();
    sig(1);
    assert(sum == 5);
  }

  sig(1);
  assert(sum == 8);
}

void test_signal_blocking() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  sig.Bind(f1);
  sig.Bind(f2);
  sig(1);
  assert(sum == 3);

  sig.Block();
  sig(1);
  assert(sum == 3);

  sig.Unblock();
  sig(1);
  assert(sum == 6);
}

void test_all_disconnection() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  sig.Bind(f1);
  sig.Bind(f2);
  sig(1);
  assert(sum == 3);

  sig.UnbindAll();
  sig(1);
  assert(sum == 3);
}

void test_connection_copying_moving() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  auto sc1 = sig.Bind(f1);
  auto sc2 = sig.Bind(f2);

  auto sc3 = sc1;
  auto sc4{sc2};

  auto sc5 = std::move(sc3);
  auto sc6{std::move(sc4)};

  sig(1);
  assert(sum == 3);

  sc5.Block();
  sig(1);
  assert(sum == 5);

  sc1.Unblock();
  sig(1);
  assert(sum == 8);

  sc6.Unbind();
  sig(1);
  assert(sum == 9);
}

void test_scoped_connection_moving() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  {
    auto sc1 = sig.BindScoped(f1);
    sig(1);
    assert(sum == 1);

    auto sc2 = sig.BindScoped(f2);
    sig(1);
    assert(sum == 4);

    auto sc3 = std::move(sc1);
    sig(1);
    assert(sum == 7);

    auto sc4{std::move(sc2)};
    sig(1);
    assert(sum == 10);
  }

  sig(1);
  assert(sum == 10);
}

void test_signal_moving() {
  sum = 0;
  evtsigslot::Signal<int> sig;

  sig.Bind(f1);
  sig.Bind(f2);

  sig(1);
  assert(sum == 3);

  auto sig2 = std::move(sig);
  sig2(1);
  assert(sum == 6);

  auto sig3 = std::move(sig2);
  sig3(1);
  assert(sum == 9);
}

template <typename T>
struct object {
  object();
  object(T i) : v{i} {}

  const T &val() const { return v; }
  T &val() { return v; }
  void set_val(const T &i) {
    if (i != v) {
      v = i;
      s(i);
    }
  }

  evtsigslot::Signal<T> &sig() { return s; }

 private:
  T v;
  evtsigslot::Signal<T> s;
};

void test_loop() {
  object<int> i1(0);
  object<int> i2(3);

  i1.sig().Bind(&object<int>::set_val, &i2);
  i2.sig().Bind(&object<int>::set_val, &i1);

  i1.set_val(1);

  assert(i1.val() == 1);
  assert(i2.val() == 1);
}

int main() {
  test_free_connection();
  test_static_connection();
  test_pmf_connection();
  test_const_pmf_connection();
  test_function_object_connection();
  test_overloaded_function_object_connection();
  test_lambda_connection();
  test_generic_lambda_connection();
  test_lvalue_emission();
  test_compatible_args();
  test_mutation();
  test_disconnection();
  test_disconnection_by_callable();
  test_disconnection_by_object();
  test_disconnection_by_object_and_pmf();
  test_scoped_connection();
  test_connection_blocker();
  test_connection_blocking();
  test_signal_blocking();
  test_all_disconnection();
  test_connection_copying_moving();
  test_scoped_connection_moving();
  test_signal_moving();
  test_loop();
  test_slot_count();
  return 0;
}
