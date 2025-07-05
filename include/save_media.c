#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <sys/stat.h>

#include "main.h"

// just saving each frame (asciiart) into save_files.
int save_media(const char* string_filename, uint8_t* rgb_data, int width, int height) {
  avformat_network_init();

  // create directory if not exist
  if(!is_exist_dir("./save_files/")){
    if(mkdir("save_files", 0755) != 0){
      fprintf(stderr, "failed to create directory.\n");
      return 1;
    }
  }
  char filename[2048];
  snprintf(filename, sizeof(filename), "./save_files/%s", string_filename);

  // set codec based what file type.
  const AVCodec* codec = NULL;
  if (strstr(filename, ".png")) {
      codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
  } else if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) {
      codec = avcodec_find_encoder(AV_CODEC_ID_MJPEGB);
  } else {
      fprintf(stderr, "Unsupported file format\n");
      return 1;
  }

  if (!codec) {
      fprintf(stderr, "Encoder not found for file: %s\n", filename);
      return 1;
  }

  // init
  AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
  codec_ctx->bit_rate = 400000;
  codec_ctx->width = width;
  codec_ctx->height = height;
  codec_ctx->time_base = (AVRational){1, 1};
  codec_ctx->pix_fmt = AV_PIX_FMT_RGB24;
  av_opt_set_int(codec_ctx->priv_data, "threads", 1, 0);

  if (avcodec_open2(codec_ctx, codec, NULL) < 0) {
      fprintf(stderr, "Could not open codec\n");
      return 1;
  }

  AVFrame* frame = av_frame_alloc();
  frame->format = codec_ctx->pix_fmt;
  frame->width = width;
  frame->height = height;
  av_image_fill_arrays(frame->data, frame->linesize, rgb_data, codec_ctx->pix_fmt, width, height, 1);

  AVPacket* pkt = av_packet_alloc();

  if (avcodec_send_frame(codec_ctx, frame) < 0) {
      fprintf(stderr, "Error sending frame\n");
      return 1;
  }

  // saving a file
  FILE* f = fopen(filename, "wb");
  if (!f) {
      fprintf(stderr, "Could not open file for writing\n");
      return 1;
  }

  while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
      fwrite(pkt->data, 1, pkt->size, f);
      av_packet_unref(pkt);
  }
  printf("Done saved file: %s\n", filename);

  fclose(f);
  av_packet_free(&pkt);
  av_frame_free(&frame);
  avcodec_free_context(&codec_ctx);

  return 0;
}
