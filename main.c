#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dirent.h>
#include <threads.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "image_info.h"
#include "convert_ascii.h"
#include "save_media.h"
#include "structs.h"
#include "convert_video.h"
#include "main.h"

static inline int get_cpu_count(){
  return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

struct struct_per_thread{
  atomic_bool is_running;
  int i;
  char* mode;
  char* style_path;
  struct image_info image_info;
  struct decoder_ctx ctx;
};

static void multi_threading_pool(struct image_info* image_info, char* filename, char* mode, char* style_path);
static int working_per_thread(void* worker_fun);
static void wait_for_thread(thrd_t* thread, atomic_bool* is_running);
void delete_files(char* path);

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("use: -input <filename> [--quality <number>] [--mode <option>] [--fps <number>] [--type] [--style-path]\n");
    return 1;
  }

  // default settings
  struct image_info image_info = {0};
  image_info.ascii_width = 240; // (it's quality, i know it's width) higher means higher details
  int fps = 24;
  char* filename = NULL;
  char* mode = "detailed";
  char* type = NULL;
  char* style_path = "./include/fonts/dejavu-fonts-ttf-2.37/ttf/DejaVuSansMono.ttf";

  // set if has args.
  for(int i = 0; i < argc; i++){
    if(strcasecmp(argv[i], "-input") == 0 && i + 1 < argc){
      filename = argv[++i];
    } else if(strcasecmp(argv[i], "--quality") == 0 && i + 1 < argc){
      image_info.ascii_width = atoi(argv[++i]);
    } else if(strcasecmp(argv[i], "--fps") == 0 && i + 1 < argc){
      fps = atoi(argv[++i]);
    } else if(strcasecmp(argv[i], "--mode") == 0 && i + 1 < argc){
      mode = argv[++i];
    } else if(strcasecmp(argv[i], "--type") == 0 && i + 1 < argc){
      type = argv[++i];
    } else if(strcasecmp(argv[i], "--style-path") == 0 && i + 1 < argc){
      style_path = argv[++i];
    }
  }

  // ensure has input
  if(filename == NULL){
    fprintf(stderr, "Please input: --input <filename>\n");
    return 1;
  }

  printf("filename: %s quality: %d mode: %s fps: %d style_path: %s type: %s :: ok\n", 
         filename, image_info.ascii_width, mode, fps, style_path, type);

  struct timeval start, end;
  gettimeofday(&start, NULL);

  // start mutli-threading task
  multi_threading_pool(&image_info, filename, mode, style_path);

  // final save as video type otherwise move single png to finish directory.
  // if has type input, use instead.
  if(type){
    if(strcasecmp(type, "gif") == 0){
      printf("choose gif type\n");
      write_video_from_image_folder("./save_files/", "./finish/finish.gif", AV_CODEC_ID_GIF,
                                    image_info.ascii_width, image_info.ascii_height, fps);
      delete_files("./save_files/");
    } else if(strcasecmp(type, "mp4") == 0){
      printf("choose mp4 type\n");
      if(image_info.ascii_width % 2 != 0) image_info.ascii_width++;
      if(image_info.ascii_height % 2 != 0) image_info.ascii_height++;
      write_video_from_image_folder("./save_files/", "./finish/finish.mp4", AV_CODEC_ID_H264,
                                    image_info.ascii_width, image_info.ascii_height, fps);
      delete_files("./save_files/");
    } else if(strcasecmp(type, "webm") == 0){
      printf("choose webm type\n");
      write_video_from_image_folder("./save_files/", "./finish/finish.webm", AV_CODEC_ID_VP8,
                                    image_info.ascii_width, image_info.ascii_height, fps);
    }
  }else{
    // otherwise going pick same type from input media
    if(strstr(filename, ".gif") != 0) {
      printf("choose gif type\n");
      write_video_from_image_folder("./save_files/", "./finish/finish.gif", AV_CODEC_ID_GIF,
                                    image_info.ascii_width, image_info.ascii_height, fps);
      delete_files("./save_files/");
    } else if(strstr(filename, ".mp4") != 0) {
      printf("choose mp4 type\n");
      if(image_info.ascii_width % 2 != 0) image_info.ascii_width++;
      if(image_info.ascii_height % 2 != 0) image_info.ascii_height++;
      write_video_from_image_folder("./save_files/", "./finish/finish.mp4", AV_CODEC_ID_H264,
                                    image_info.ascii_width, image_info.ascii_height, fps);
      delete_files("./save_files/");
    } else if(strstr(filename, ".webm") != 0) {
      printf("choose webm type\n");
      write_video_from_image_folder("./save_files/", "./finish/finish.webm", AV_CODEC_ID_VP8,
                                    image_info.ascii_width, image_info.ascii_height, fps);

      delete_files("./save_files/");
    } else {
      if(!is_exist_dir("./finish")){
        mkdir("finish", 0700);
      }
      // it's just move file.
      rename("./save_files/0.png", "./finish/finish.png");
    }
  }



  // free up incase
  if(image_info.rgb != NULL) free(image_info.rgb);
  if(image_info.ascii_letters != NULL) free(image_info.ascii_letters);
  if(image_info.gray_pixel != NULL) free(image_info.gray_pixel);
  gettimeofday(&end, NULL);
  double seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
  printf("Finished: %.6f secs\n", seconds);
  return 0;
}

// check if directory is exist.
int is_exist_dir(const char *path){
  struct stat st;
  return(stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// remove any files in save_files after finish job.
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

// make sure wait until thread is finish.
static void wait_for_thread(thrd_t* thread, atomic_bool* is_running){
  while(atomic_load(is_running)){
    struct timespec ts = {0, 1e6};
    thrd_sleep(&ts, NULL);
  }
  thrd_join(*thread, NULL);
}

// it give task per thread.
static int working_per_thread(void* struct_per_thread){
  struct struct_per_thread* thread_struct = (struct struct_per_thread*) struct_per_thread;
  atomic_store(&thread_struct->is_running, true);

  char temp_filename[128];
  if(convert_ascii(&thread_struct->image_info, thread_struct->mode)) return 1;
  if(convert_actual_ascii(&thread_struct->image_info, thread_struct->style_path)) return 1;
  sprintf(temp_filename, "%d.png", thread_struct->i);
  if(save_media(temp_filename, thread_struct->image_info.rgb, thread_struct->image_info.width, thread_struct->image_info.height)) return 1;

  atomic_store(&thread_struct->is_running, false);
  return 0;
}

// start multi-threading task
void multi_threading_pool(struct image_info* image_info, char* filename, char* mode, char* style_path){
  int threads_count = get_cpu_count();
  int threads_working = 0;
  if(threads_count > 1) threads_count--; // leave one thread for this
  thrd_t threads[threads_count];

  struct decoder_ctx ctx;

  struct struct_per_thread own_threads[threads_count];
  memset(own_threads, 0, sizeof(struct struct_per_thread));

  if(init_decoder(&ctx, filename, image_info->ascii_width) == 0){
    image_info->width = ctx.width;
    image_info->height = ctx.height;
    image_info->ascii_width = ctx.ascii_width;
    image_info->ascii_height = ctx.ascii_height;

    image_info->gray_pixel = malloc(sizeof(struct gray_pixel) * ctx.ascii_width * ctx.ascii_height);

    for(int i = 0; i < threads_count; i++){
      own_threads[i].mode = mode;
      own_threads[i].style_path = style_path;
      own_threads[i].image_info.width = ctx.width;
      own_threads[i].image_info.height = ctx.height;
      own_threads[i].image_info.ascii_width = ctx.ascii_width;
      own_threads[i].image_info.ascii_height = ctx.ascii_height;

      own_threads[i].image_info.gray_pixel = malloc(sizeof(struct gray_pixel) * ctx.ascii_width * ctx.ascii_height);
      own_threads[i].image_info.rgb = calloc(ctx.ascii_width * ctx.ascii_height * 3, 1);
      own_threads[i].image_info.ascii_letters = malloc(sizeof(struct ascii_letters) * image_info->ascii_width * image_info->ascii_height);
    }

    for(int i = 0; read_next_frame(&ctx, image_info->gray_pixel) == 0; i++){
      int slot = i % threads_count;
      threads_working++;

      if(i >= threads_count){
        wait_for_thread(&threads[slot], &own_threads[slot].is_running);
        threads_working--;
      }

      own_threads[slot].i = i;
      memcpy(own_threads[slot].image_info.gray_pixel, image_info->gray_pixel, 
             sizeof(struct gray_pixel) * image_info->ascii_width * image_info->ascii_height);
      own_threads[slot].is_running = true;
      thrd_create(&threads[slot], working_per_thread, &own_threads[slot]);
    }

    for(int i = 0; i < threads_working; i++){
      wait_for_thread(&threads[i], &own_threads[i].is_running);
    }

    close_decoder(&ctx);

    image_info->ascii_width = own_threads[0].image_info.width;
    image_info->ascii_height = own_threads[0].image_info.height;

    // free all malloc from each thread.
    for(int i = 0; i < threads_count; i++){
      if(own_threads[i].image_info.gray_pixel != NULL){
        free(own_threads[i].image_info.gray_pixel);
        own_threads[i].image_info.gray_pixel = NULL;
      }

      if(own_threads[i].image_info.rgb != NULL){
        free(own_threads[i].image_info.rgb);
        own_threads[i].image_info.rgb = NULL;
      }

      if(own_threads[i].image_info.ascii_letters != NULL){
        free(own_threads[i].image_info.ascii_letters);
        own_threads[i].image_info.ascii_letters = NULL;
      }
    }
  }
}
