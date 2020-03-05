#pragma once

#include <memory_resource>
#include <boost/container/small_vector.hpp>

namespace ntl {
template<class T, std::size_t N, class Allocator>
using small_vector = boost::container::small_vector<T, N, Allocator>;
template<class T, class Allocator>
using small_vector_base = boost::container::small_vector_base<T, Allocator>;
namespace pmr {
template<class T, std::size_t N>
using small_vector = small_vector<T, N, std::pmr::polymorphic_allocator<T>>;
template<class T>
using small_vector_base =
    small_vector_base<T, std::pmr::polymorphic_allocator<T>>;
}  // namespace pmr
}  // namespace ntl
