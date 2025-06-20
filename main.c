#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dirent.h>

#include "image_info.h"
#include "convert_ascii.h"
#include "save_media.h"
#include "structs.h"
#include "convert_video.h"

int set_force(char* choice){
  int force = -1;
  if(strcasecmp(choice, "gif") == 0){
    force = 0;
  } else if(strcasecmp(choice, "mp4") == 0){
    force = 1;
  } else if(strcasecmp(choice, "webm") == 0){
    force = 2;
  }

  return force;
}

void delete_files(char* path){
  struct dirent* entry;
  DIR* dir = opendir(path);
  if(!dir) return;

  char del_filename[512];
  while((entry = readdir(dir)) != NULL){
    if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
    sprintf(del_filename, "%s%s", path, entry->d_name);

    remove(del_filename);
  }
}

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("use: --input <filename> [--quality <number>] [--mode <option>] [--fps <number>]");
    return 1;
  }

  struct image_info image_info;
  image_info.ascii_width = 240;
  int fps = 24;
  char* filename = NULL;
  char* mode = "detailed";
  int force = -1;

  for(int i = 0; i < argc; i++){
    if(strcasecmp(argv[i], "--input") == 0 && i + 1 < argc){
      filename = argv[++i];
    } else if(strcasecmp(argv[i], "--quality") == 0 && i + 1 < argc){
      image_info.ascii_width = atoi(argv[++i]);
    } else if(strcasecmp(argv[i], "--fps") == 0 && i + 1 < argc){
      fps = atoi(argv[++i]);
    } else if(strcasecmp(argv[i], "--mode") == 0 && i + 1 < argc){
      mode = argv[++i];
    } else if(strcasecmp(argv[i], "--force") == 0 && i + 1 < argc){
      force = set_force(argv[++i]);
    }
  }
  printf("filename: %s quality: %d mode: %s fps: %d :: ok\n", filename, image_info.ascii_width, mode, fps);

  if(filename == NULL){
    fprintf(stderr, "Please input: --input <filename>\n");
    return 1;
  }


  struct timeval start, end;
  gettimeofday(&start, NULL);
  char temp_filename[128];

  struct decoder_ctx ctx;

  if(init_decoder(&ctx, filename, image_info.ascii_width) == 0){
    image_info.width = ctx.width;
    image_info.height = ctx.height;
    image_info.ascii_width = ctx.ascii_width;
    image_info.ascii_height = ctx.ascii_height;

    image_info.gray_pixel = malloc(sizeof(struct gray_pixel) * image_info.ascii_width * image_info.ascii_height);

    for(int i = 0; read_next_frame(&ctx, image_info.gray_pixel) == 0; i++){
      if(convert_ascii(&image_info, mode)) return 1;
      if(convert_actual_ascii(&image_info, "./include/fonts/dejavu-fonts-ttf-2.37/ttf/DejaVuSansMono.ttf")) return 1;
      sprintf(temp_filename, "%d.png", i);
      if(save_media(temp_filename, image_info.rgb, image_info.width, image_info.height)) return 1;

      if(image_info.ascii_letters != NULL) free(image_info.ascii_letters);
      if(image_info.rgb != NULL) free(image_info.rgb);
    }

    if(image_info.gray_pixel != NULL) free(image_info.gray_pixel);
    close_decoder(&ctx);

    if (force == 0 || strstr(filename, ".gif") != 0) {
      write_video_from_image_folder("./save_files/", "./finish/finish.gif", AV_CODEC_ID_GIF,
                                    image_info.width, image_info.height, fps);
      delete_files("./save_files/");
    } else if (force == 1 || strstr(filename, ".mp4") != 0) {
      if(image_info.width % 2 != 0) image_info.width++;
      if(image_info.height % 2 != 0) image_info.height++;
      write_video_from_image_folder("./save_files/", "./finish/finish.mp4", AV_CODEC_ID_H264,
                                    image_info.width, image_info.height, fps);
      delete_files("./save_files/");
    } else if (force == 2 || strstr(filename, ".webm") != 0) {
      write_video_from_image_folder("./save_files/", "./finish/finish.webm", AV_CODEC_ID_VP8,
                                    image_info.width, image_info.height, fps);

      delete_files("./save_files/");
    } else {
      fprintf(stderr, "Unsupported output format\n");
    }
  }

  gettimeofday(&end, NULL);
  double seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
  printf("Finished: %.6f secs\n", seconds);
  return 0;
}
