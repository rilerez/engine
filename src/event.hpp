#pragma once

#include <tuple>
#include "guile_utils.hpp"
#include <chrono>
#include <vector>
#include <memory_resource>
#include <queue>

#include "clock.hpp"

struct Event {
  guile_utils::Symbol type;
  SCM data;
  Clock::time_point time;
  int priority;

  bool ready(Clock::time_point now) const { return time <= now; }

  Event() = default;
  Event(guile_utils::Symbol type,
        SCM data,
        Clock::time_point time,
        int priority)
      : type{type}, data{data}, time{time}, priority{priority} {}

 private:
#define DEFOP(op)                                                              \
  friend auto operator op(Event const x, Event const y) {                      \
    return std::tuple{x.time, x.priority} op std::tuple{y.time, y.priority};   \
  }
  DEFOP(<)
  DEFOP(<=)
  DEFOP(>)
  DEFOP(>=)
#undef DEFOP
};

using EventQueue = std::priority_queue<Event, std::pmr::vector<Event>>;
