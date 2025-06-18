#include <stdlib.h>

#include "structs.h"

void convert_ascii(struct image_info* image_info){
  // simple is 10 characters
  //char table_simple[] = " .:-=+*#%@";
  // detailed is 70 characters
  char table_detailed[] = ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

  image_info->ascii_letters = malloc(sizeof(struct ascii_letters) * image_info->width * image_info->height);

  for(int i=0; i < (image_info->width * image_info->height); i++){
    int pick = (int)(image_info->gray_pixel[i].value / 255) * 70;
    image_info->ascii_letters[i].letter = table_detailed[pick];
  }

  // free gray pixels as we dont need them anymore.
  // and set NULL just incase to avoid misuse.
  free(image_info->gray_pixel);
  image_info->gray_pixel = NULL;
}
