#include <array>
#include <complex>
#include <tuple>

template<class N>
struct vec2 {
 private:
  std::complex<N> data_;

 public:
  vec2() = default;
  vec2(std::complex<N> z) : data_{z} {}
  vec2(N x, N y = N{0}) : data_{x, y} {}
  vec2(std::array<N, 2> v) : vec2{v[0], v[1]} {}
  vec2(std::tuple<N, N> t) : vec2{std::get<0>(t), std::get<1>(t)} {}

#define CONST_AND_NOT(const_)                                                  \
  const_ auto& asVector() const_ {                                             \
    return reinterpret_cast<N const_(&)[2]>(data_);                            \
  }                                                                            \
  const_ auto& asComplex() const_ { return data_; }                            \
  const_ auto& asTuple() const_ { return std::tie(x(), y()); }                 \
  const_ auto& x() const_ { return asVector()[0]; }                            \
  const_ auto& y() const_ { return asVector()[1]; }                            \
  const_ auto& real() const_ { return x(); }                                   \
  const_ auto& imag() const_ { return y(); }

  CONST_AND_NOT()
  CONST_AND_NOT(const)
#undef CONST_AND_NOT

  auto operator-() const { return vec2(-x(), -y()); }

#define FOR_ALL_BINOPS(macro) macro(+) macro(-) macro(*) macro(/)

#define LIFT_OPEQ(op)                                                          \
  auto operator op##=(vec2 const z) { data_ op## = z; }
  FOR_ALL_BINOPS(LIFT_OPEQ)

 private:  // hidden friend
#define LIFT_BIN_OP(op)                                                        \
  friend auto operator op(vec2 const x, vec2 const y) {                        \
    return vec2{x.data_ op y.data_};                                           \
  }
  FOR_ALL_BINOPS(LIFT_BIN_OP)
#undef FOR_ALL_BINOPS
#undef LIFT_OPEQ
#undef LIFT_BIN_OP
};
