CC = gcc

CFLAGS = -O3 -Iffmpeg -Wall -Wextra -ffunction-sections -fdata-sections \
         -I./include/ -I./include/freetype/include/
				 #-fsanitize=address -fno-omit-frame-pointer -g

LDFLAGS = -Wl,--gc-sections -lavformat -lavcodec -lavutil -lswscale -lz -lbz2 -lpng \
          ./include/freetype/objs/.libs/libfreetype.a -lbrotlidec -lharfbuzz

SRC = main.c \
      $(wildcard ./include/image_info.c) \
      $(wildcard ./include/convert_ascii.c) \
      $(wildcard ./include/save_media.c) \
      $(wildcard ./include/convert_video.c)

OBJS = $(SRC:.c=.o)

OUT = media_to_asciiart

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(OUT) $(LDFLAGS)

# Linux object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUT) $(OBJS)

