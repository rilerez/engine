#include <algorithm>
#include <cassert>
#include <chrono>
#include <entt/entt.hpp>
#include <libguile.h>
#include <bitset>

#include "sdl2raii/emscripten_glue.hpp"
#include "sdl2raii/sdl.hpp"

#include "clock.hpp"
#include "vec2.hpp"
#include "util.hpp"
#include "guile_utils.hpp"
#include "event.hpp"

using namespace std::literals;
namespace chrono = std::chrono;

sdl::unique::Window window;
sdl::unique::Renderer renderer;
entt::registry registry;

using vec = vec2<float>;
using Size = vec2<int>;

struct Phase {
  vec position;
  vec velocity;
};

vec interpolate_pos(Phase const phase, chrono::duration<float> dt) {
  return phase.position + phase.velocity * dt.count();
}

struct Collider{
  std::bitset<32> layers;
};

struct Sprite {
  sdl::Rect src;
  sdl::unique::Texture tex;
};

struct Timeline {
  Clock::time_point then;
  Clock::time_point now;
  Timeline() = default;
  Timeline(Clock::time_point start) : then{start}, now{start} {}
  void update(Clock::time_point new_now) {
    then = now;
    now = new_now;
  }
  Clock::duration elapsed() { return now - then; }
};

template<>
SCM guile_utils::pack<vec>(vec const v) {
  return scm_make_rectangular(pack(v.x()), pack(v.y()));
}

template<>
vec guile_utils::unpack<vec>(SCM v) {
  assert(scm_is_complex(v));
  return vec{static_cast<float>(unpack<double>(scm_real_part(v))),
             static_cast<float>(unpack<double>(scm_imag_part(v)))};
}

EventQueue events;

void inner_main() {
  def_scm_clock();

  using namespace guile_utils;
  def_guile_wrapper_type<entt::registry>("registry");
  scm_c_define("registry", box_ptr(&registry));

  define_subr(
      "registry-create",
      +[](SCM reg) { return pack(unbox<entt::registry>(reg)->create()); });

  def_guile_wrapper_type<entt::entity>("entity");
  define_subr(
      "phase",
      +[](SCM reg, SCM ent) {
        auto [pos, vel] =
            unbox<entt::registry>(reg)->get<Phase>(unpack<entt::entity>(ent));
        return list();
      });

  sdl::Init(sdl::init::video);
  util::finally _ = [] { sdl::Quit(); };
  auto draw_group = registry.group<Phase const, Size const, Sprite const>();
  auto update_view = registry.view<Phase>();

  scm_c_primitive_load("/home/riley/projects/engine/lisp/init.scm");

  window = sdl::CreateWindow("thingy",
                             sdl::window::pos_undefined,
                             sdl::window::pos_undefined,
                             1000,
                             1000,
                             0);
  renderer = sdl::CreateRenderer(
      window.get(),
      -1,
      sdl::renderer::accelerated | sdl::renderer::presentvsync);

  auto ball = registry.create();
  registry.assign<Phase>(ball) =
      Phase{.position = vec{30.0, 30.0}, .velocity = vec{8, 4}};
  registry.assign<Size>(ball, 32, 32);
  registry.assign<Sprite>(
      ball,
      Sprite{
          .src = sdl::Rect{0, 0, 32, 32},
          .tex = sdl::CreateTextureFromSurface(
              renderer.get(),
              sdl::LoadBMP("/home/riley/projects/engine/assets/circle.bmp"))});

  auto time = Timeline(Clock::now());

  auto update_type = guile_utils::Symbol("update");
  auto push_update_event = [=](auto time) {
    events.emplace(update_type, guile_utils::list(), time, 5);
  };

  push_update_event(time.now);

  Clock::duration update_lag = 0ms;
  emscripten_glue::main_loop([&] {
    auto const elapsed = time.elapsed();
    update_lag += elapsed;
    while(auto const event = sdl::NextEvent()) {
      switch(event->type) {
        case SDL_QUIT:
          emscripten_glue::cancel_main_loop();
          break;
      }
    }

    auto const keep_polling_events = [&]() {
      return (not events.empty()) and events.top().ready(time.now);
    };
    while(keep_polling_events()) {
      auto ev = events.top();
      if(ev.type == update_type) {
        Clock::duration const update_step = 10ms;
        update_lag -= update_step;
        auto update = [](auto& update_view, Clock::duration dt) {
          update_view.each(
              [dt](Phase& p) { p.position = interpolate_pos(p, dt); });
        };
        update(update_view, update_step);
        events.pop();
        push_update_event(time.now + update_step);
      }
    }

    auto draw = [](sdl::Renderer* renderer, auto& group, Clock::duration dt) {
      sdl::RenderClear(renderer);
      group.each([renderer, dt](Phase const phase,
                                Size const size,
                                Sprite const& sprite) {
        auto const newpos = interpolate_pos(phase, dt);
        auto const intify = [] FN(static_cast<int>(_));
        sdl::RenderCopy(renderer,
                        sprite.tex.get(),
                        std::nullopt,
                        sdl::Rect{intify(newpos.x()),
                                  intify(newpos.y()),
                                  size.x(),
                                  size.y()});
      });
      sdl::RenderPresent(renderer);
    };
    draw(renderer.get(), draw_group, update_lag);

    time.update(Clock::now());
  });
}

int main(int argc, char** argv) { guile_utils::with_guile(inner_main); }
