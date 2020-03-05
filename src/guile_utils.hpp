#pragma once
#include <optional>
#include <functional>
#include <libguile.h>

namespace guile_utils {

struct Symbol {
  SCM obj;
  Symbol() = default;
  Symbol(SCM symbol) : obj{symbol} { assert(scm_is_symbol(symbol)); }
  Symbol(char const* const name) : obj{scm_from_utf8_symbol(name)} {}
  operator SCM() const { return obj; }
};

template<class T>
std::optional<SCM> guile_type = std::nullopt;

namespace impl {
template<class Thunk>
using return_t = decltype(std::declval<Thunk>()());

template<class Thunk>
void* thunk_calling_callback(void* erased_thunk) {
  Thunk& thunk = *static_cast<Thunk*>(erased_thunk);
  using result_t = return_t<Thunk>;
  if constexpr(std::is_same_v<result_t, void>) {
    thunk();
    return nullptr;
  } else {
    std::unique_ptr<result_t> exception_safe_result{thunk()};
    return static_cast<void*>(exception_safe_result.release();
  }
}
}  // namespace impl

template<class Callee, class Thunk>
inline auto call_thunk_with(Callee callee, Thunk thunk) {
  void* res =
      callee(impl::thunk_calling_callback<Thunk>, static_cast<void*>(&thunk));
  // if res is null?
  using result_t = impl::return_t<Thunk>;
  if constexpr(std::is_same_v<result_t, void>) {
    return;
  } else {
    auto typed_res =
        unique_ptr<result_t>{static_cast<result_t*>(res)};  // owning(?)
    static_assert(
        std::is_nothrow_move_constructible_v<result_t>,
        "Please do not throw in move constructors. I beg you do not throw in "
        "move constructors for types you're shuttling through C callbacks.");
    auto stack_copy_result = std::move(*typed_res);
    return stack_copy_result;
  }
}

#define WRAP_C_CALLBACK(cppname, cname)                                        \
  template<class Thunk>                                                        \
  inline auto cppname(Thunk thunk) {                                           \
    return call_thunk_with(cname, thunk);                                      \
  }

WRAP_C_CALLBACK(with_guile, scm_with_guile)
WRAP_C_CALLBACK(without_guile, scm_without_guile)
WRAP_C_CALLBACK(with_continuation_barrier, scm_c_with_continuation_barrier)
#undef WRAP_C_CALLBACK

#define DEF_GC_NEW(name, scm_gcnew_name)                                       \
  template<class Type, class... Args>                                          \
  inline Type* name(char const* const name, Args&&... args) {                  \
    constexpr auto size = sizeof(Type);                                        \
    Type* p = static_cast<Type*>(scm_gcnew_name(size, name));                  \
    new(p)(Type)(std::forward<Args>(args)...);                                 \
    return p;                                                                  \
  }
DEF_GC_NEW(gcnew, scm_gc_malloc)
DEF_GC_NEW(gcnew_pointerless, scm_gc_malloc_pointerless)
#undef DEF_GC_NEW

template<class... arg_t>
SCM list(arg_t&&... arg) {
  static_assert((std::is_convertible_v<arg_t, SCM> && ...));
  return scm_list_n(SCM{std::forward<arg_t>(arg)}..., SCM_UNDEFINED);
}

struct restargs {
  SCM arglist;
  restargs() = default;
  restargs(SCM x) : arglist{x} {}
  operator SCM() { return arglist; }
};

template<class... Args>
auto define_subr(char const* const name, SCM (*fn)(Args..., restargs)) {
  static_assert((std::is_convertible_v<Args, SCM> && ...));
  return scm_c_define_gsubr(name, sizeof...(Args), 0, 1, &fn);
}

template<class... Args>
auto define_subr(char const* const name, SCM (*fn)(Args...)) {
  static_assert((std::is_convertible_v<Args, SCM> && ...));
  return scm_c_define_gsubr(name, sizeof...(Args), 0, 0, &fn);
}

auto make_foreign_object_type(Symbol name,
                              SCM fields,
                              scm_t_struct_finalize finalizer = nullptr) {
  return scm_make_foreign_object_type(name, fields, finalizer);
}

template<class To, class From>
inline auto bit_cast(const From& src) noexcept {
  static_assert(sizeof(To) == sizeof(From));
  static_assert(std::is_trivially_copyable_v<From>);
  static_assert(std::is_trivial_v<To>);
  To dst;
  std::memcpy(&dst, &src, sizeof(To));
  return dst;
}

template<class T>
void def_guile_wrapper_type(char const* const name) {
  using namespace guile_utils;
  assert(guile_type<T> == std::nullopt);
  static SCM const fields = list(Symbol("data"));
  guile_type<T> = make_foreign_object_type(Symbol(name), fields);
}

// is it possible to pack and box the same type? if so should there be a
// different scheme type?
template<class T>
constexpr bool fits_in_SCM = (sizeof(scm_t_bits) >= sizeof(T));
template<class T>
SCM pack(T val) {
  assert(guile_type<T> != std::nullopt);
  static_assert(fits_in_SCM<T>);
  SCM the_box = scm_make_foreign_object_0(*guile_type<T>);
  scm_foreign_object_unsigned_set_x(the_box, 0, bit_cast<scm_t_bits>(val));
  return the_box;
}
template<class T>
SCM box_gc(T val) {
  assert(*guile_type<T> != std::nullopt);
  SCM the_box =
      scm_make_foreign_object_1(*guile_type<T>, gcnew<T>(std::move(val)));
  return the_box;
}
template<class T>
SCM box_ptr(T* const ptr) {
  assert(guile_type<T> != std::nullopt);
  SCM the_box =
      scm_make_foreign_object_1(*guile_type<T>, static_cast<void*>(ptr));
  return the_box;
}
template<class T>
T* unbox(SCM the_box) {
  scm_assert_foreign_object_type(*guile_type<T>, the_box);
  return static_cast<T*>(scm_foreign_object_ref(the_box, 0));
}
template<class T>
T unpack(SCM packed) {
  scm_assert_foreign_object_type(*guile_type<T>, packed);
  return bit_cast<T>(scm_foreign_object_unsigned_ref(packed, 0));
}
#define DEF_UNPACK_PRIM(name, type)                                            \
  template<>                                                                   \
  type unpack<type>(SCM packed) {                                              \
    return scm_to_##name(packed);                                              \
  }
#define DEF_PACK_PRIM(name, type)                                              \
  template<>                                                                   \
  SCM pack<type>(type x) {                                                     \
    return scm_from_##name(x);                                                 \
  }

#define DEF_PACK_UNPACK(name, type)                                            \
  DEF_UNPACK_PRIM(name, type) DEF_PACK_PRIM(name, type)
#define DEF_PACK_UNPACK_INT(name) DEF_PACK_UNPACK(name, scm_t_##name)
DEF_PACK_UNPACK_INT(int8)
DEF_PACK_UNPACK_INT(uint8)
DEF_PACK_UNPACK_INT(int16)
DEF_PACK_UNPACK_INT(uint16)
DEF_PACK_UNPACK_INT(int32)
DEF_PACK_UNPACK_INT(uint32)
DEF_PACK_UNPACK_INT(int64)
DEF_PACK_UNPACK_INT(uint64)

DEF_PACK_UNPACK(bool, bool)
DEF_PACK_UNPACK(double, double)

struct list_adaptor {
  SCM cons;
  list_adaptor() = default;
  explicit list_adaptor(SCM cons) : cons{cons} {}
  auto operator*() { return scm_car(cons); }
  auto operator++() {
    cons = scm_cdr(cons);
    return this;
  }
  auto operator++(int) {
    auto old = this;
    ++(*this);
    return old;
  }
  friend auto begin(list_adaptor cons) const { return cons; }
  friend auto end(list_adaptor) const { return list_adaptor{SCM_EOL}; }
  friend bool operator==(list_adaptor x, list_adaptor y) const {
    return x.cons == y.cons;
  }
  friend bool operator!=(list_adaptor x, list_adaptor y) const {
    return x.cons != y.cons;
  }
};

template<>
list_adaptor unpack<list_adaptor>(SCM x) {
  assert(scm_is_pair(x));
  return list_adaptor{x};
}
template<>
SCM pack<list_adaptor>(list_adaptor x) {
  return x.cons;
}

template<class T, class = std::enable_if_t<std::is_convertible_v<T, SCM>>>
struct gc_guard {
  T x;
  gc_guard(T x) : x{x} { scm_gc_protect_object(x); }
  ~gc_guard() { scm_gc_unprotect_object(x); }
};
}  // namespace guile_utils
