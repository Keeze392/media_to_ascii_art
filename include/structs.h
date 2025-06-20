#ifndef STRUCTS
#define STRUCTS

#include <stdint.h>

struct gray_pixel{
  unsigned char value;
};

struct ascii_letters{
  char letter;
};

struct image_info{
  int width, height;
  int ascii_width, ascii_height;
  struct gray_pixel* gray_pixel;
  struct ascii_letters* ascii_letters;
  uint8_t* rgb;
};

#endif
