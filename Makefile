CC = gcc
CFLAGS = -I./include/ -Iffmpeg -Wall -Wextra -ffunction-sections -fdata-sections
LDFLAGS = -Wl,--gc-sections -lavformat -lavcodec -lavutil -lswscale

SRC = main.c \
			$(wildcard ./include/image_info.c) \
			$(wildcard ./include/convert_ascii.c) \

OUT = test

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)
