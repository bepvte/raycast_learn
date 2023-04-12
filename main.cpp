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

// void drop_png_image(const std::string filename,
//                     SDL_Surface* image,
//                     const size_t w,
//                     const size_t h) {
//   std::FILE* f = std::fopen(filename.c_str(), "wb");
//   if (!f)
//     err_exit("file wont open");
//   png_structp png_ptr =
//       png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr,
//       nullptr);
//   png_infop info_ptr = png_create_info_struct(png_ptr);
//   if (!png_ptr || !info_ptr)
//     err_exit("cant allocate info png");
//   if (setjmp(png_jmpbuf(png_ptr))) {
//     png_destroy_write_struct(&png_ptr, &info_ptr);
//     err_exit("failed");
//   }
//   png_set_IHDR(png_ptr, info_ptr, w, h, 8, PNG_COLOR_TYPE_RGB_ALPHA,
//                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
//                PNG_FILTER_TYPE_DEFAULT);
//   png_init_io(png_ptr, f);
//   png_set_invert_alpha(png_ptr);
//   png_write_info(png_ptr, info_ptr);
//   std::vector<png_byte> imagel(h * w * BYTES_PER_PIXEL);
//   std::vector<png_byte*> row_pointers(h);

//   for (uint32_t y = 0; y < h; y++) {
//     row_pointers[y] = imagel.data() + y * w * 4;
//     for (uint32_t x = 0; x < w; x++) {
//       uint8_t r, g, b, a;
//       unpack_color(reinterpret_cast<uint32_t*>(image->pixels)[x + y * w], r,
//       g,
//                    b, a);
//       // printf("at y: %u and x: %u | r: %u, g: %u, b: %u, a: %u\n", y, x, r,
//       g,
//       // b, a);
//       row_pointers[y][x * 4] = r;
//       row_pointers[y][x * 4 + 1] = g;
//       row_pointers[y][x * 4 + 2] = b;
//       row_pointers[y][x * 4 + 3] = a;
//     }
//   }

//   png_write_image(png_ptr, row_pointers.data());

//   png_write_end(png_ptr, info_ptr);
//   png_destroy_write_struct(&png_ptr, &info_ptr);

//   if (fclose(f))
//     err_exit("failed to close file");
// }

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

const size_t win_w = 1024;  // image width
const size_t win_h = 512;   // image height
const size_t map_w = 16;    // map width
const size_t map_h = 16;  // map height
const char map[] =
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
    "0002222222200000";  // our game map

float player_x = 3.456;
float player_y = 2.345;
float player_a = 1.523;
const float fov = M_PI / 3.;
SDL_Surface* surface = nullptr;
SDL_Window* window = nullptr;

void iter() {
  SDL_Event evt;
  while (SDL_PollEvent(&evt)) {
    switch (evt.type) {
      case SDL_MOUSEMOTION: {
        const SDL_MouseMotionEvent* mouse =
            reinterpret_cast<SDL_MouseMotionEvent*>(&evt);
        float movement = mouse->xrel * (M_PI / 180);
        player_a += movement;
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
    player_y -= 0.05;
  if (ks[SDL_SCANCODE_DOWN] || ks[SDL_SCANCODE_S])
    player_y += 0.05;
  if (ks[SDL_SCANCODE_RIGHT] || ks[SDL_SCANCODE_D])
    player_x += 0.05;
  if (ks[SDL_SCANCODE_LEFT] || ks[SDL_SCANCODE_A])
    player_x -= 0.05;

  SDL_FillRect(surface, nullptr, pack_color(255, 255, 255));

  const size_t rect_w = win_w / (map_w * 2);
  const size_t rect_h = win_h / map_h;
  for (size_t j = 0; j < map_h; j++) {
    for (size_t i = 0; i < map_w; i++) {
      if (map[i + j * map_w] == ' ')
        continue;
      const size_t rect_x = i * rect_w;
      const size_t rect_y = j * rect_h;
      draw_rectangle(surface, win_w, win_h, rect_x, rect_y, rect_w, rect_h,
                     pack_color(0, 128, 255));
    }
  }

  draw_rectangle(surface, win_w, win_h, player_x * rect_w, player_y * rect_h, 5,
                 5, pack_color(255, 0, 0));

  for (size_t i = 0; i < win_w / 2; i++) {  // 512 rays
    // player_a minus half fov (top of cone)
    const float angle =
        // NOLINTNEXTLINE(bugprone-integer-division)
        player_a - fov / 2 + fov * i / static_cast<float>(win_w / 2);
    for (float t = 0; t < 20; t += 0.05) {
      const float cx = player_x + t * cos(angle);
      const float cy = player_y + t * sin(angle);
      // printf("t: %f, cx: %f, cy: %f\n", t, cx, cy);

      const int pix_x = cx * rect_w;
      const int pix_y = cy * rect_h;
      draw_rectangle(surface, win_w, win_h, pix_x, pix_y, 1, 1, pack_color(160,160,160));

      const uint32_t maploc =
          static_cast<uint32_t>(cx) + static_cast<uint32_t>(cy) * map_w;
      if (maploc > map_h * map_w)
        break;
      const char maptile = map[maploc];
      if (maptile != ' ') {
        const size_t column_height = win_h / t;
        draw_rectangle(surface, win_w, win_h, win_w / 2 + i,
                       win_h / 2 - column_height / 2, 1, column_height,
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

  window = SDL_CreateWindow("my3dhome", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, win_w, win_h,
                            SDL_WINDOW_INPUT_GRABBED);
  if (!window)
    err_exit(SDL_GetError());

  surface = SDL_CreateRGBSurface(0, win_w, win_h, 32, 0, 0, 0, 0);
  if (!surface)
    err_exit(SDL_GetError());

  err = SDL_SetRelativeMouseMode(SDL_TRUE);
  if (err)
    err_exit(SDL_GetError());

  SDL_FillRect(surface, nullptr, pack_color(255, 255, 255));
  assert(sizeof(map) == map_w * map_h + 1);
  // drop_png_image("./out.png", surface, win_w, win_h);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop(iter, 0, true);
#else
  while (true) {
    uint32_t start = SDL_GetTicks();
    iter();
    const uint32_t end = SDL_GetTicks();
    uint32_t delay = end - start;
    if (delay > 16)
      delay = 16;
    SDL_Delay(delay);
  }
#endif
  return 0;
}
