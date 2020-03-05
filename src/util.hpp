#pragma once

#include <functional>


namespace util {
template<class Thunk>
struct finally {
  Thunk thunk;
  finally(Thunk thunk) : thunk{std::move(thunk)} {}
  ~finally() { thunk(); }
};

#define FN(...)                                                                \
  (auto _) { return __VA_ARGS__; }

template<class F, class G>
constexpr auto comp(F f, G g) {
  return
      [&](auto&&... arg) { return f(g(std::forward<decltype(arg)>(arg)...)); };
}

template<class X, class Y, class Less = std::less<>>
constexpr decltype(auto) stable_max(X&& x, Y&& y, Less less = {}) {
  auto greater_or_eq = comp([] FN(!_), less);
  return std::min(x, y, greater_or_eq);
}

}  // namespace util
