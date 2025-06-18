#include "stb_image.h"
#include "structs.h"

struct image_info get_info_image(const char* image_path){
  // please free the struct rgb_pixel or gray_pixel after using it
  struct image_info temp;
  temp.success = 1;

  unsigned char* data = stbi_load(image_path, &temp.width, &temp.height, &temp.channel, 0);

  // if channel set to rgb or rgba color
  if(temp.channel == 3 || temp.channel == 4){
    // malloc for rgb from width + height + channel.
    temp.gray_pixel = malloc(sizeof(struct gray_pixel) * temp.width * temp.height);
    if(!temp.gray_pixel){
      stbi_image_free(data);
      temp.success = 0;
      return temp;
    }

    for(size_t i = 0; i < (temp.height * temp.height); i++){
      temp.gray_pixel[i].value = 0.299 * data[i * 3] + 0.587 * data[i * 3 + 1] + 0.114 * data[i * 3 + 2];
    }
    stbi_image_free(data);

    // if channel is set gray or gray alpha color
  } else if(temp.channel == 1 || temp.channel == 2){
    // malloc for gray color only
    temp.gray_pixel = malloc(sizeof(struct gray_pixel) * temp.width * temp.height);
    if(!temp.gray_pixel){
      stbi_image_free(data);
      temp.success = 0;
      return temp;
    }

    for(int i = 0; i < (temp.height * temp.width); i++){
      temp.gray_pixel[i].value = data[i * 3];
    }
    stbi_image_free(data);

  }
  
  return temp;
}
