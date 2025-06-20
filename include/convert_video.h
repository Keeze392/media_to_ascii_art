#ifndef CONVERT_MEDIA
#define CONVERT_MEDIA

#include <libavformat/avformat.h>

int write_video_from_image_folder(const char *input_dir, const char *output_filename,
                                  enum AVCodecID codec_id, int width, int height, int fps);

#endif
