#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "image_info.h"
#include "convert_ascii.h"
#include "structs.h"

int main(void){
  struct timeval start, end;
  gettimeofday(&start, NULL);

  struct image_info image_info;
  image_info = get_info_image("./5jwtpab3nny81.png");
  convert_ascii(&image_info);

  for(size_t i=0; i <= 30; i++){
    printf("%c", image_info.ascii_letters[i].letter);
  }
  printf("\n");

  if(image_info.ascii_letters != NULL){
    free(image_info.ascii_letters);
  }
  if(image_info.gray_pixel != NULL){
    free(image_info.gray_pixel);
  }

  gettimeofday(&end, NULL);
  double seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
  printf("Finished: %.6f secs\n", seconds);
  if(!image_info.success) return 1;
  return 0;
}
