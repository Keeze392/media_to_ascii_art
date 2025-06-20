#ifndef GET_INFO_IMAGE
#define GET_INFO_IMAGE

#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>

#include "structs.h"

struct decoder_ctx {
  AVFormatContext* fmt_ctx;
  AVCodecContext* codec_ctx;
  struct SwsContext* sws_ctx;
  int video_stream_index;
  int width, height;
  int64_t frame_count;
  AVFrame* frame;
  AVFrame* rgb_frame;
  AVPacket* pkt;
  uint8_t* rgb_buf;

  int ascii_width, ascii_height;
};

int init_decoder(struct decoder_ctx* ctx, const char* filename, int ascii_width);
int read_next_frame(struct decoder_ctx* ctx, struct gray_pixel* out_rgb_buf);
void close_decoder(struct decoder_ctx* ctx);

#endif
