#include <SDL2/SDL.h>
#include <SDL_error.h>
#include <SDL_events.h>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_surface.h>
#include <SDL_timer.h>
#include <SDL_video.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define BYTES_PER_PIXEL 4

void err_exit(const std::string error_text) {
  std::perror(error_text.c_str());
  std::exit(1);
}

constexpr uint32_t pack_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0) {
  uint32_t color = 0;
  color += ((uint32_t)r << 0);
  color += ((uint32_t)g << 8);
  color += ((uint32_t)b << 16);
  color += ((uint32_t)a << 24);
  return color;
}

constexpr void unpack_color(const uint32_t& color,
                            uint8_t& r,
                            uint8_t& g,
                            uint8_t& b,
                            uint8_t& a) {
  r = (color >> 0) & 255;
  g = (color >> 8) & 255;
  b = (color >> 16) & 255;
  a = (color >> 24) & 255;
}

void draw_rectangle(SDL_Surface* img,
                    const size_t img_w,
                    const size_t img_h,
                    const size_t x,
                    const size_t y,
                    const size_t w,
                    const size_t h,
                    const uint32_t color) {
#if 0
  assert(img.size() == img_w * img_h);
  for (size_t i = 0; i < w; i++) {
    for (size_t j = 0; j < h; j++) {
      size_t cx = x + i;
      size_t cy = y + j;
      if (cx >= img_w || cy >= img_h)
        continue;
      img[cx + cy * img_w] = color;
    }
  }
#else
  SDL_Rect rect;
  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;
  SDL_FillRect(img, &rect, color);
#endif
}

void blit(SDL_Surface* source, SDL_Window* window) {
  SDL_Surface* target = SDL_GetWindowSurface(window);
  SDL_BlitSurface(source, nullptr, target, nullptr);
  SDL_UpdateWindowSurface(window);
}

struct Map {
  const size_t w = 16;
  const size_t h = 16;
  std::vector<char> m;  // our game map
  const char& operator[](std::size_t idx) const { return m[idx]; }
  Map();
  Map(const std::vector<char> &m) : m(m) {};
  Map(const char (&m)[]);
  const char at(const size_t idx) const { return m.at(idx); };
};
Map::Map() {
  const char mapdata[] =
      "0000222222220000"
      "1              0"
      "1      11111   0"
      "1     0        0"
      "0     0  1110000"
      "0     3        0"
      "0   10000      0"
      "0   0   11100  0"
      "0   0   0      0"
      "0   0   1  00000"
      "0       1      0"
      "2       1      0"
      "0       0      0"
      "0 0000000      0"
      "0              0"
      "0002222222200000";
  m = std::vector<char>(std::begin(mapdata), std::end(mapdata));
}
Map::Map(const char (&map)[]) {
  const size_t length = strlen(map);
  m = std::vector<char>(map, map+length);
}

const float fov = M_PI / 3;

struct State {
  float x = 3.456;
  float y = 2.345;
  float a = 1.523;
  const size_t win_w = 1024;  // image width
  const size_t win_h = 512;   // image height
  const Map map = Map();
  State() {};
  State(const Map &map) : map(map) {};
};

#ifdef __EMSCRIPTEN__
struct iterargs {
  State *state;
  SDL_Window *window;
  SDL_Surface *surface;
};
void iter(iterargs *args) {
  State &state = *args->state;
  SDL_Window *window = args->window;
  SDL_Surface *surface = args->surface;
#else
void iter(State &state, SDL_Window *window, SDL_Surface* surface) {
#endif
  SDL_Event evt;
  while (SDL_PollEvent(&evt)) {
    switch (evt.type) {
      case SDL_MOUSEMOTION: {
        const SDL_MouseMotionEvent* mouse =
            reinterpret_cast<SDL_MouseMotionEvent*>(&evt);
        float movement = mouse->xrel * (M_PI / 180);
        state.a += movement;
        break;
      }
      case SDL_QUIT:
        SDL_FreeSurface(surface);
        SDL_DestroyWindow(window);
        std::exit(0);
        break;
    }
  }
  const uint8_t* ks = SDL_GetKeyboardState(nullptr);
  if (ks[SDL_SCANCODE_ESCAPE]) {
    SDL_QuitEvent evt = {SDL_QUIT, SDL_GetTicks()};
    SDL_PushEvent(reinterpret_cast<SDL_Event*>(&evt));
  }
  if (ks[SDL_SCANCODE_UP] || ks[SDL_SCANCODE_W])
    state.y -= 0.05;
  if (ks[SDL_SCANCODE_DOWN] || ks[SDL_SCANCODE_S])
    state.y += 0.05;
  if (ks[SDL_SCANCODE_RIGHT] || ks[SDL_SCANCODE_D])
    state.x += 0.05;
  if (ks[SDL_SCANCODE_LEFT] || ks[SDL_SCANCODE_A])
    state.x -= 0.05;

  SDL_FillRect(surface, nullptr, pack_color(255, 255, 255));

  const Map &map = state.map;

  const size_t rect_w = state.win_w / (map.w * 2);
  const size_t rect_h = state.win_h / map.h;
  for (size_t j = 0; j < map.h; j++) {
    for (size_t i = 0; i < map.w; i++) {
      if (map[i + j * map.w] == ' ')
        continue;
      const size_t rect_x = i * rect_w;
      const size_t rect_y = j * rect_h;
      draw_rectangle(surface, state.win_w, state.win_h, rect_x, rect_y, rect_w, rect_h,
                     pack_color(0, 128, 255));
    }
  }

  draw_rectangle(surface, state.win_w, state.win_h, state.x * rect_w,
                 state.y * rect_h, 5, 5, pack_color(255, 0, 0));

  for (size_t i = 0; i < state.win_w / 2; i++) {  // 512 rays
    // player_a minus half fov (top of cone)
    const float angle =
        // NOLINTNEXTLINE(bugprone-integer-division)
        state.a - fov / 2 + fov * i / static_cast<float>(state.win_w / 2);
    for (float t = 0; t < 20; t += 0.05) {
      const float cx = state.x + t * cos(angle);
      const float cy = state.y + t * sin(angle);
      // printf("t: %f, cx: %f, cy: %f\n", t, cx, cy);

      const int pix_x = cx * rect_w;
      const int pix_y = cy * rect_h;
      draw_rectangle(surface, state.win_w, state.win_h, pix_x, pix_y, 1, 1,
                     pack_color(160, 160, 160));

      const uint32_t maploc =
          static_cast<uint32_t>(cx) + static_cast<uint32_t>(cy) * map.w;
      if (maploc > map.h * map.w)
        break;
      const char maptile = map[maploc];
      if (maptile != ' ') {
        const size_t column_height = state.win_h / t;
        draw_rectangle(surface, state.win_w, state.win_h, state.win_w / 2 + i,
                       state.win_h / 2 - column_height / 2, 1, column_height,
                       pack_color(0, 128, 255));
        break;
      }
    }
  }
  blit(surface, window);
}

extern "C" int main(int argc, char* argv[]) {
  // the image itself, initialized to red
  atexit(SDL_Quit);
  int err = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
  if (err)
    err_exit(SDL_GetError());

  State state{};

  SDL_Window* window = SDL_CreateWindow("my3dhome", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, state.win_w,
                                        state.win_h, SDL_WINDOW_INPUT_GRABBED);
  if (!window)
    err_exit(SDL_GetError());

  SDL_Surface* surface =
      SDL_CreateRGBSurface(0, state.win_w, state.win_h, 32, 0, 0, 0, 0);
  if (!surface)
    err_exit(SDL_GetError());

  err = SDL_SetRelativeMouseMode(SDL_TRUE);
  if (err)
    err_exit(SDL_GetError());

  SDL_FillRect(surface, nullptr, pack_color(255, 255, 255));
  assert(state.map.m.size() == state.map.w * state.map.h + 1);
  // drop_png_image("./out.png", surface, win_w, win_h);

#ifdef __EMSCRIPTEN__
  iterargs args{&state, window, surface};
  emscripten_set_main_loop_arg((em_arg_callback_func)iter, (void*)&args, 0, true);
#else
  while (true) {
    uint32_t start = SDL_GetTicks();
    iter(state, window, surface);
    const uint32_t end = SDL_GetTicks();
    uint32_t delay = end - start;
    if (delay > 16)
      delay = 16;
    SDL_Delay(delay);
  }
#endif
  return 0;
}
