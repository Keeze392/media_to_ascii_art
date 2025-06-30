#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "structs.h"

int convert_ascii(struct image_info* image_info, const char* choice){
  if(choice == NULL) return 1;
  // simple is 10 characters
  char table_simple[] = " .:-=+*#%@";
  char table_reverse_simple[] = "@%#*+=-:. ";
  // detailed is 70 characters
  char table_detailed[] = ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
  char table_reverse_detailed[] = "$@B%8&WM#*oahkbdpqwmZO0QLCJUYXzcvznxrjft/\\|()1{}[]?-_+~<>j!lI;:,";

  char* pick_table = NULL;

  if(strcasecmp(choice, "simple") == 0){ 
    pick_table = table_simple;
  } else if(strcasecmp(choice, "reverse_simple") == 0){
    pick_table = table_reverse_simple;
  } else if(strcasecmp(choice, "reverse_detailed") == 0) {
    pick_table = table_reverse_detailed;
  } else {
    pick_table = table_detailed;
  }

  // pick letter based grayscale per pixel
  int table_len = strlen(pick_table) - 1;
  for(int i=0; i < (image_info->ascii_width * image_info->ascii_height); i++){
    int pick_index = (int)((image_info->gray_pixel[i].value / 255.0) * (table_len - 1));
    image_info->ascii_letters[i].letter = pick_table[pick_index];
  }
  return 0;
}

static void draw_char_to_rgb(FT_Bitmap* bmap, uint8_t* rgb_buffer,
                      int img_w, int img_h, int pos_x, int pos_y){

  for(unsigned int y = 0; y < bmap->rows; y++){
    for(unsigned int x = 0; x < bmap->width; x++){
      int xx = pos_x + x;
      int yy = pos_y + y;
      
      if(xx < 0 || xx >= img_w || yy < 0 || yy >= img_h){
        continue;
      }

      int offset = (yy * img_w + xx) * 3;
      uint8_t gray = bmap->buffer[y * bmap->pitch + x];

      rgb_buffer[offset] = gray;
      rgb_buffer[offset + 1] = gray;
      rgb_buffer[offset + 2] = gray;
    }
  }
}

int convert_actual_ascii(struct image_info* image_info, const char* font_path){
  FT_Library library;
  if(FT_Init_FreeType(&library)){
    fprintf(stderr, "Failed to init library.\n");
    return 1;
  }

  FT_Face face;
  if(FT_New_Face(library, font_path, 0, &face)){
    fprintf(stderr, "Failed to loading font: %s\n", font_path);
    FT_Done_FreeType(library);
    return 1;
  }

  FT_Set_Pixel_Sizes(face, 0, 16);

  char* ascii_string = malloc((image_info->ascii_width+1) * image_info->ascii_height + 1);
  if(!ascii_string){
    fprintf(stderr, "Failed to malloc ascii string.\n");
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return 1;
  }

  for(int y = 0; y < image_info->ascii_height; y++){
    for(int x = 0; x < image_info->ascii_width; x++){
      ascii_string[y * (image_info->ascii_width + 1) + x] = image_info->ascii_letters[y * image_info->ascii_width + x].letter;
    }
    ascii_string[y * (image_info->ascii_width + 1) + image_info->ascii_width] = '\n';
  }
  ascii_string[(image_info->ascii_width + 1) * image_info->ascii_height] = '\0';

  int scale = 16;
  FT_Set_Pixel_Sizes(face, 0, scale);

  FT_Load_Char(face, 'M', FT_LOAD_RENDER);
  int glyph_w = face->glyph->bitmap.width;

  int ascender = face->size->metrics.ascender >> 6;
  int descender = face->size->metrics.descender >> 6;
  int linegap = (face->size->metrics.height >> 6) - (ascender - descender);
  int line_height = ascender - descender + linegap;
  int total_lines = image_info->ascii_height;

  int out_width = image_info->ascii_width * glyph_w;
  int out_height = total_lines * line_height;

  image_info->width = out_width;
  image_info->height = out_height;

  image_info->rgb = calloc(out_width * out_height * 3, 1);
  if(!image_info->rgb){
    fprintf(stderr, "failed to calloc rgb buffer\n");
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    return 1;
  }

  int x_cursor = 0;
  int y_cursor = 0;

  // start drawing letter from ascii_string array
  for(const char* p = ascii_string; *p; p++){
    if(*p == '\n'){
      y_cursor += line_height;
      x_cursor = 0;
      continue;
    }

    if(FT_Load_Char(face, *p, FT_LOAD_RENDER)){
      fprintf(stderr, "Failed to load glyph '%c'\n", *p);
      x_cursor += face->glyph->advance.x >> 6;
      continue;
    }

    FT_Bitmap* bmap = &face->glyph->bitmap;
    int glyph_x = x_cursor + face->glyph->bitmap_left;
    int glyph_y = y_cursor + ascender - (scale - face->glyph->bitmap_top);

    draw_char_to_rgb(bmap, image_info->rgb, out_width, out_height, glyph_x, glyph_y);
    x_cursor += face->glyph->advance.x >> 6;
  }

  FT_Done_Face(face);
  FT_Done_FreeType(library);
  return 0;
}
