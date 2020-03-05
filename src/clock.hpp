#pragma once

#include <chrono>
#include "guile_utils.hpp"

using Clock = std::chrono::steady_clock;

void def_scm_clock() {
  using namespace std::chrono_literals;
  using std::chrono::milliseconds;
  using namespace guile_utils;

  def_guile_wrapper_type<milliseconds>("milliseconds");

  define_subr(
      "milliseconds",
      +[](SCM ms) { return pack(milliseconds{unpack<int>(ms)}); });

  def_guile_wrapper_type<Clock::duration>("duration");

  define_subr(
      "duration",
      +[](SCM dt) { return pack(Clock::duration{unpack<int>(dt)}); });

  define_subr(
      "milliseconds->duration",
      +[](SCM ms) { return pack<Clock::duration>(unpack<milliseconds>(ms)); });

  define_subr(
      "milliseconds-count",
      +[](SCM ms) { return pack(unpack<milliseconds>(ms).count()); });
  define_subr(
      "duration-count",
      +[](SCM dt) { return pack(unpack<Clock::duration>(dt).count()); });

  define_subr(
      "milliseconds+",
      +[](restargs args) {
        auto adapted_args = list_adaptor(args.arglist);
        return pack(std::accumulate(begin(adapted_args),
                                    end(adapted_args),
                                    0ms,
                                    [](milliseconds const x, SCM const y) {
                                      return x + unpack<milliseconds>(y);
                                    }));
      });

  define_subr(
      "duration->milliseconds",
      +[](SCM duration) {
        return pack(
            duration_cast<milliseconds>(unpack<Clock::duration>(duration)));
      });

  def_guile_wrapper_type<Clock::time_point>("time-point");

  define_subr(
      "time-point",
      +[](SCM duration) {
        return pack(Clock::time_point(unpack<Clock::duration>(duration)));
      });

  define_subr(
      "now",
      +[]() { return pack(Clock::now()); });

  define_subr(
      "time-point-diff",
      +[](SCM x, SCM y) {
        return pack(unpack<Clock::time_point>(x)
                    - unpack<Clock::time_point>(y));
      });
}
