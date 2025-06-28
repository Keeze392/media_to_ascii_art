#if defined(__linux__)

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dirent.h>
#include <threads.h>
#include <unistd.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "image_info.h"
#include "convert_ascii.h"
#include "save_media.h"
#include "structs.h"
#include "convert_video.h"

static inline int get_cpu_count(){
  return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

#elif defined(_WIN32)
#include <windows.h>

static inline int get_cpu_count(){
  return (int)sysinfo.dwNumberOfProcessors;
}

#endif

struct worker_fun{
  atomic_bool is_running;
  int i;
  char* mode;
  struct image_info image_info;
  struct decoder_ctx ctx;
};

static void multi_threading_pool(struct image_info* image_info, char* filename, char* mode);
static int working_per_thread(void* worker_fun);
static void wait_for_thread(thrd_t* thread, atomic_bool* is_running);
int set_force(char* choice);
void delete_files(char* path);

int main(int argc, char* argv[]){
  if(argc < 2){
    printf("use: --input <filename> [--quality <number>] [--mode <option>] [--fps <number>]\n");
    return 1;
  }

  struct image_info image_info = {0};
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

  if(filename == NULL){
    fprintf(stderr, "Please input: --input <filename>\n");
    return 1;
  }

  printf("filename: %s quality: %d mode: %s fps: %d :: ok\n", filename, image_info.ascii_width, mode, fps);

  struct timeval start, end;
  gettimeofday(&start, NULL);

  multi_threading_pool(&image_info, filename, mode);

  if (force == 0 || strstr(filename, ".gif") != 0) {
    write_video_from_image_folder("./save_files/", "./finish/finish.gif", AV_CODEC_ID_GIF,
                                  image_info.ascii_width, image_info.ascii_height, fps);
    delete_files("./save_files/");
  } else if (force == 1 || strstr(filename, ".mp4") != 0) {
    if(image_info.ascii_width % 2 != 0) image_info.ascii_width++;
    if(image_info.ascii_height % 2 != 0) image_info.ascii_height++;
    write_video_from_image_folder("./save_files/", "./finish/finish.mp4", AV_CODEC_ID_H264,
                                  image_info.ascii_width, image_info.ascii_height, fps);
    delete_files("./save_files/");
  } else if (force == 2 || strstr(filename, ".webm") != 0) {
    write_video_from_image_folder("./save_files/", "./finish/finish.webm", AV_CODEC_ID_VP8,
                                  image_info.ascii_width, image_info.ascii_height, fps);

    delete_files("./save_files/");
  } else {
    rename("./save_files/0.png", "./finish/0.png");
  }

  if(image_info.rgb != NULL) free(image_info.rgb);
  if(image_info.ascii_letters != NULL) free(image_info.ascii_letters);
  if(image_info.gray_pixel != NULL) free(image_info.gray_pixel);
  gettimeofday(&end, NULL);
  double seconds = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1e6;
  printf("Finished: %.6f secs\n", seconds);
  return 0;
}

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

static void wait_for_thread(thrd_t* thread, atomic_bool* is_running){
  while(atomic_load(is_running)){
    struct timespec ts = {0, 1e6};
    thrd_sleep(&ts, NULL);
  }
  thrd_join(*thread, NULL);
}

static int working_per_thread(void* worker_fun){
  struct worker_fun* str_worker_fun = (struct worker_fun*) worker_fun;
  atomic_store(&str_worker_fun->is_running, true);

  char temp_filename[128];
  if(convert_ascii(&str_worker_fun->image_info, str_worker_fun->mode)) return 1;
  if(convert_actual_ascii(&str_worker_fun->image_info, "./include/fonts/dejavu-fonts-ttf-2.37/ttf/DejaVuSansMono.ttf")) return 1;
  sprintf(temp_filename, "%d.png", str_worker_fun->i);
  if(save_media(temp_filename, str_worker_fun->image_info.rgb, str_worker_fun->image_info.width, str_worker_fun->image_info.height)) return 1;

  atomic_store(&str_worker_fun->is_running, false);
  return 0;
}

void multi_threading_pool(struct image_info* image_info, char* filename, char* mode){
  int threads_count = get_cpu_count();
  int threads_working = 0;
  if(threads_count > 1) threads_count--;
  thrd_t threads[threads_count];

  struct decoder_ctx ctx;

  struct worker_fun worker_fun[threads_count];
  memset(worker_fun, 0, sizeof(struct worker_fun));

  if(init_decoder(&ctx, filename, image_info->ascii_width) == 0){
    image_info->width = ctx.width;
    image_info->height = ctx.height;
    image_info->ascii_width = ctx.ascii_width;
    image_info->ascii_height = ctx.ascii_height;

    image_info->gray_pixel = malloc(sizeof(struct gray_pixel) * ctx.ascii_width * ctx.ascii_height);

    for(int i = 0; i < threads_count; i++){
      worker_fun[i].mode = mode;
      worker_fun[i].image_info.width = ctx.width;
      worker_fun[i].image_info.height = ctx.height;
      worker_fun[i].image_info.ascii_width = ctx.ascii_width;
      worker_fun[i].image_info.ascii_height = ctx.ascii_height;

      worker_fun[i].image_info.gray_pixel = malloc(sizeof(struct gray_pixel) * ctx.ascii_width * ctx.ascii_height);
      worker_fun[i].image_info.rgb = calloc(ctx.ascii_width * ctx.ascii_height * 3, 1);
      worker_fun[i].image_info.ascii_letters = malloc(sizeof(struct ascii_letters) * image_info->ascii_width * image_info->ascii_height);
    }

    for(int i = 0; read_next_frame(&ctx, image_info->gray_pixel) == 0; i++){
      int slot = i % threads_count;
      threads_working++;

      if(i >= threads_count){
        wait_for_thread(&threads[slot], &worker_fun[slot].is_running);
        threads_working--;
      }

      worker_fun[slot].i = i;
      memcpy(worker_fun[slot].image_info.gray_pixel, image_info->gray_pixel, 
             sizeof(struct gray_pixel) * image_info->ascii_width * image_info->ascii_height);
      worker_fun[slot].is_running = true;
      thrd_create(&threads[slot], working_per_thread, &worker_fun[slot]);
    }

    for(int i = 0; i < threads_working; i++){
      wait_for_thread(&threads[i], &worker_fun[i].is_running);
    }

    close_decoder(&ctx);

    image_info->ascii_width = worker_fun[0].image_info.width;
    image_info->ascii_height = worker_fun[0].image_info.height;

    for(int i = 0; i < threads_count; i++){
      if(worker_fun[i].image_info.gray_pixel != NULL){
        free(worker_fun[i].image_info.gray_pixel);
        worker_fun[i].image_info.gray_pixel = NULL;
      }
      
      if(worker_fun[i].image_info.rgb != NULL){
        free(worker_fun[i].image_info.rgb);
        worker_fun[i].image_info.rgb = NULL;
      }

      if(worker_fun[i].image_info.ascii_letters != NULL){
        free(worker_fun[i].image_info.ascii_letters);
        worker_fun[i].image_info.ascii_letters = NULL;
      }
    }
  }
}
