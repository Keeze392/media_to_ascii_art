#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int has_extension(const char* filename, const char* ext) {
  size_t len_fname = strlen(filename);
  size_t len_ext = strlen(ext);
  if (len_fname < len_ext) return 0;
  return strcasecmp(filename + len_fname - len_ext, ext) == 0;
}

int compare_filenames(const void* a, const void* b) {
  const char* fa = *(const char**)a;
  const char* fb = *(const char**)b;

  const char* sa = strrchr(fa, '/');
  const char* sb = strrchr(fb, '/');
  sa = sa ? sa + 1 : fa;
  sb = sb ? sb + 1 : fb;

  int na = atoi(sa);
  int nb = atoi(sb);

  return na - nb;
}

int list_image_files(const char* dir_path, char*** files_out, int* count_out) {
  DIR* dir = opendir(dir_path);
  if (!dir) return -1;

  struct dirent* entry;
  int capacity = 16, size = 0;
  char** files = malloc(capacity * sizeof(char*));

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type != DT_REG) continue;
    if (!has_extension(entry->d_name, ".png") && !has_extension(entry->d_name, ".jpg")) continue;

    if (size == capacity) {
      capacity *= 2;
      files = realloc(files, capacity * sizeof(char*));
    }

    char full_path[512];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
    files[size++] = strdup(full_path);
  }
  closedir(dir);

  qsort(files, size, sizeof(char*), compare_filenames);

  *files_out = files;
  *count_out = size;
  return 0;
}

int decode_image_to_frame(const char* filename, AVFrame* dst, enum AVPixelFormat dst_fmt,
                          int width, int height, struct SwsContext** sws_ctx) {

  AVFormatContext* fmt = NULL;
  AVCodecContext* dec_ctx = NULL;
  const AVCodec* dec;
  AVPacket* pkt = av_packet_alloc();
  AVFrame* frame = av_frame_alloc();
  int ret = -1;

  if (avformat_open_input(&fmt, filename, NULL, NULL) < 0 || avformat_find_stream_info(fmt, NULL) < 0)
    goto cleanup;

  int vid = av_find_best_stream(fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (vid < 0) goto cleanup;

  dec = avcodec_find_decoder(fmt->streams[vid]->codecpar->codec_id);
  if (!dec) goto cleanup;

  dec_ctx = avcodec_alloc_context3(dec);
  avcodec_parameters_to_context(dec_ctx, fmt->streams[vid]->codecpar);
  if (avcodec_open2(dec_ctx, dec, NULL) < 0) goto cleanup;

  if (av_read_frame(fmt, pkt) < 0) goto cleanup;
  if (avcodec_send_packet(dec_ctx, pkt) < 0) goto cleanup;
  if (avcodec_receive_frame(dec_ctx, frame) < 0) goto cleanup;

  if (!*sws_ctx) {
    *sws_ctx = sws_getContext(frame->width, frame->height, frame->format,
                              width, height, dst_fmt, SWS_BICUBIC, NULL, NULL, NULL);
  }

  av_frame_make_writable(dst);
  sws_scale(*sws_ctx, (const uint8_t* const*)frame->data, frame->linesize,
            0, frame->height, dst->data, dst->linesize);

  dst->width = width;
  dst->height = height;
  dst->format = dst_fmt;
  ret = 0;

cleanup:
  av_packet_free(&pkt);
  av_frame_free(&frame);
  avcodec_free_context(&dec_ctx);
  avformat_close_input(&fmt);
  return ret;
}

int write_video_from_image_folder(const char* input_dir, const char* output_filename,
                                  enum AVCodecID codec_id, int width, int height, int fps) {

  printf("processing for video or gif...\n");
  AVFormatContext* fmt_ctx = NULL;
  AVCodecContext* codec_ctx = NULL;
  AVStream* stream = NULL;
  AVFrame* frame = NULL;
  struct SwsContext* sws_ctx = NULL;
  int ret = -1;

  char** files = NULL;
  int file_count = 0;
  if (list_image_files(input_dir, &files, &file_count) != 0 || file_count == 0) {
    fprintf(stderr, "No image files found in %s\n", input_dir);
    return -1;
  }

  avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, output_filename);
  const AVCodec* codec = avcodec_find_encoder(codec_id);
  codec_ctx = avcodec_alloc_context3(codec);
  stream = avformat_new_stream(fmt_ctx, codec);

  codec_ctx->codec_id = codec_id;
  codec_ctx->width = width;
  codec_ctx->height = height;
  codec_ctx->time_base = (AVRational){1, fps};
  codec_ctx->framerate = (AVRational){fps, 1};
  codec_ctx->pix_fmt = (codec_id == AV_CODEC_ID_GIF) ? AV_PIX_FMT_RGB8 : AV_PIX_FMT_YUV420P;
  codec_ctx->thread_count = 0;
  codec_ctx->thread_type = FF_THREAD_SLICE;
  if (codec_id != AV_CODEC_ID_GIF)
    codec_ctx->bit_rate = 1e6;

  if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  avcodec_open2(codec_ctx, codec, NULL);
  avcodec_parameters_from_context(stream->codecpar, codec_ctx);
  stream->time_base = codec_ctx->time_base;

  if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE))
    avio_open(&fmt_ctx->pb, output_filename, AVIO_FLAG_WRITE);

  if (avformat_write_header(fmt_ctx, NULL) < 0) goto end;

  frame = av_frame_alloc();
  frame->format = codec_ctx->pix_fmt;
  frame->width  = width;
  frame->height = height;
  av_frame_get_buffer(frame, 32);

  for (int i = 0; i < file_count; i++) {
    if (decode_image_to_frame(files[i], frame, codec_ctx->pix_fmt, width, height, &sws_ctx) != 0) {
      fprintf(stderr, "Failed to decode %s\n", files[i]);
      continue;
    }
    frame->pts = i;

    avcodec_send_frame(codec_ctx, frame);
    AVPacket* pkt = av_packet_alloc();
    while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
      av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
      pkt->stream_index = stream->index;
      av_interleaved_write_frame(fmt_ctx, pkt);
      av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
  }

  avcodec_send_frame(codec_ctx, NULL); // flush
  AVPacket* pkt = av_packet_alloc();
  while (avcodec_receive_packet(codec_ctx, pkt) == 0) {
    av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
    pkt->stream_index = stream->index;
    av_interleaved_write_frame(fmt_ctx, pkt);
    av_packet_unref(pkt);
  }
  av_packet_free(&pkt);

  av_write_trailer(fmt_ctx);
  ret = 0;

end:
  for (int i = 0; i < file_count; i++) free(files[i]);
  free(files);
  if (sws_ctx) sws_freeContext(sws_ctx);
  av_frame_free(&frame);
  avcodec_free_context(&codec_ctx);
  avformat_free_context(fmt_ctx);
  return ret;
}

