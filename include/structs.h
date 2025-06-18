#ifndef STRUCTS
#define STRUCTS
  struct gray_pixel{
    unsigned char value;
  };

  struct ascii_letters{
    char letter;
  };

  struct image_info{
    int success;
    int width, height;
    struct gray_pixel* gray_pixel;
    struct ascii_letters* ascii_letters;
  };
#endif
