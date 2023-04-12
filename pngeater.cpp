#include <cstdio>
#include <cstdlib>
#include "png.h"

void err(const char* err_text) {
  std::perror(err_text);
  std::exit(1);
}

int main(int argc, char** argv) {
  if (argc != 2)
    err("invalid arguments");
  std::FILE* f = std::fopen(argv[1], "rb");
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!png_ptr || !info_ptr)
    err("no struct");
  if (setjmp(png_jmpbuf(png_ptr)))
    err("png error");
  png_init_io(png_ptr, f);
  png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, 0);
  png_bytepp row_pointers = png_get_rows(png_ptr, info_ptr);
  unsigned int width, height;
  png_get_IHDR(png_ptr, info_ptr, &width, &height, nullptr, nullptr, nullptr, nullptr, nullptr);
  for (unsigned int y = 0; y < height; y++) {
    for (unsigned int x = 0; x < width; x++) {
      png_byte r = row_pointers[y][x*3];
      png_byte g = row_pointers[y][x*3+1];
      png_byte b = row_pointers[y][x*3+2];
      png_byte a = row_pointers[y][x*3+3];
      printf("pixel at x%u y%u: r: %x g: %x b: %x a: %x\n", x, y, r, g, b, a);
    }
  }
  png_read_end(png_ptr, info_ptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
  std::fclose(f);
  return 0;
}
