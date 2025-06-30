#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ffmpeg libraries
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
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

int init_decoder(struct decoder_ctx* ctx, const char* filename, int ascii_width) {
  avformat_network_init();
  memset(ctx, 0, sizeof(*ctx));

  if (avformat_open_input(&ctx->fmt_ctx, filename, NULL, NULL) < 0) {
    fprintf(stderr, "Cannot open file\n");
    return 1;
  }

  if (avformat_find_stream_info(ctx->fmt_ctx, NULL) < 0) {
    fprintf(stderr, "Cannot find stream info\n");
    return 1;
  }

  ctx->video_stream_index = av_find_best_stream(ctx->fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

  if (ctx->video_stream_index >= 0) {
    AVStream* video_stream = ctx->fmt_ctx->streams[ctx->video_stream_index];
    AVCodecParameters* codecpar = video_stream->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    ctx->codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx->codec_ctx, codecpar);
    avcodec_open2(ctx->codec_ctx, codec, NULL);

    ctx->frame_count = video_stream->nb_frames;
    if (ctx->frame_count == 0) {
      AVRational rate = av_guess_frame_rate(ctx->fmt_ctx, video_stream, NULL);
      double seconds = (double)video_stream->duration * video_stream->time_base.num / video_stream->time_base.den;
      ctx->frame_count = (int64_t)(seconds * rate.num / rate.den);
    }
    if (ctx->frame_count <= 0 || ctx->frame_count == AV_NOPTS_VALUE) {
      ctx->frame_count = 1;
    }

    ctx->width = codecpar->width;
    ctx->height = codecpar->height;

  } else {
    // Try to treat it as an image (e.g. PNG, JPEG)
    AVCodecParameters* codecpar = NULL;
    for (unsigned int i = 0; i < ctx->fmt_ctx->nb_streams; i++) {
      if (ctx->fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
        codecpar = ctx->fmt_ctx->streams[i]->codecpar;
        break;
      }
    }

    if (!codecpar) {
      fprintf(stderr, "Could not find codec parameters for image file.\n");
      return 1;
    }

    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
      fprintf(stderr, "No suitable decoder found for image\n");
      return 1;
    }

    ctx->codec_ctx = avcodec_alloc_context3(codec);
    if (!ctx->codec_ctx) return 1;
    avcodec_parameters_to_context(ctx->codec_ctx, codecpar);

    if (avcodec_open2(ctx->codec_ctx, codec, NULL) < 0) {
      fprintf(stderr, "Failed to open codec for image\n");
      return 1;
    }

    ctx->width = codecpar->width;
    ctx->height = codecpar->height;
    ctx->frame_count = 1;
    ctx->video_stream_index = -1; // mark no video stream
  }

  ctx->ascii_width = ascii_width;
  const double font_aspect_ratio = 2.0;
  ctx->ascii_height = (int)((ctx->height / (double)ctx->width) * ascii_width / font_aspect_ratio);

  ctx->frame = av_frame_alloc();
  ctx->rgb_frame = av_frame_alloc();
  ctx->pkt = av_packet_alloc();
  ctx->codec_ctx->thread_count = 0;
  ctx->codec_ctx->thread_type = FF_THREAD_SLICE;

  if (ctx->codec_ctx->pix_fmt == AV_PIX_FMT_YUVJ420P)
      ctx->codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  else if (ctx->codec_ctx->pix_fmt == AV_PIX_FMT_YUVJ422P)
      ctx->codec_ctx->pix_fmt = AV_PIX_FMT_YUV422P;
  else if (ctx->codec_ctx->pix_fmt == AV_PIX_FMT_YUVJ444P)
      ctx->codec_ctx->pix_fmt = AV_PIX_FMT_YUV444P;

  ctx->sws_ctx = sws_getContext(
    ctx->width, ctx->height, ctx->codec_ctx->pix_fmt,
    ctx->ascii_width, ctx->ascii_height, AV_PIX_FMT_RGB24,
    SWS_BILINEAR, NULL, NULL, NULL
  );

  int rgb_buf_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, ctx->ascii_width, ctx->ascii_height, 1);
  ctx->rgb_buf = av_malloc(rgb_buf_size);
  if(ctx->rgb_buf == NULL) return 1;
  if(av_image_fill_arrays(ctx->rgb_frame->data, ctx->rgb_frame->linesize, ctx->rgb_buf,
                          AV_PIX_FMT_RGB24, ctx->ascii_width, ctx->ascii_height, 1) <= 0) return 1;

  return 0;
}

// get info each frame and convert rgb to grayscale.
int read_next_frame(struct decoder_ctx* ctx, struct gray_pixel* out_gray_buf) {
  while (av_read_frame(ctx->fmt_ctx, ctx->pkt) >= 0) {
    if (ctx->video_stream_index >= 0 && ctx->pkt->stream_index == ctx->video_stream_index) {
      avcodec_send_packet(ctx->codec_ctx, ctx->pkt);
      if (avcodec_receive_frame(ctx->codec_ctx, ctx->frame) == 0) {
        sws_scale(ctx->sws_ctx,
                  (const uint8_t* const*)ctx->frame->data,
                  ctx->frame->linesize,
                  0, ctx->height,
                  ctx->rgb_frame->data,
                  ctx->rgb_frame->linesize);

        uint8_t* rgb_data = ctx->rgb_frame->data[0];
        int total_pixels = ctx->ascii_width * ctx->ascii_height;

        for (int i = 0; i < total_pixels; i++) {
          uint8_t r = rgb_data[i * 3];
          uint8_t g = rgb_data[i * 3 + 1];
          uint8_t b = rgb_data[i * 3 + 2];
          out_gray_buf[i].value = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
        }

        av_packet_unref(ctx->pkt);
        return 0;
      }
    }
    av_packet_unref(ctx->pkt);
  }

  // If not a video stream, decode single image frame
  if (ctx->video_stream_index < 0) {
    if (avcodec_send_packet(ctx->codec_ctx, ctx->pkt) < 0) return 1;
    if (avcodec_receive_frame(ctx->codec_ctx, ctx->frame) == 0) {
      sws_scale(ctx->sws_ctx,
                (const uint8_t* const*)ctx->frame->data,
                ctx->frame->linesize,
                0, ctx->height,
                ctx->rgb_frame->data,
                ctx->rgb_frame->linesize);

      uint8_t* rgb_data = ctx->rgb_frame->data[0];
      int total_pixels = ctx->ascii_width * ctx->ascii_height;

      for (int i = 0; i < total_pixels; i++) {
        uint8_t r = rgb_data[i * 3];
        uint8_t g = rgb_data[i * 3 + 1];
        uint8_t b = rgb_data[i * 3 + 2];
        out_gray_buf[i].value = (uint8_t)(0.299 * r + 0.587 * g + 0.114 * b);
      }

      return 0;
    }
  }

  return 1;
}

// cleaning up.
void close_decoder(struct decoder_ctx* ctx) {
  av_frame_free(&ctx->frame);
  av_frame_free(&ctx->rgb_frame);
  av_packet_free(&ctx->pkt);
  sws_freeContext(ctx->sws_ctx);
  avcodec_free_context(&ctx->codec_ctx);
  avformat_close_input(&ctx->fmt_ctx);
  av_free(ctx->rgb_buf);
}
