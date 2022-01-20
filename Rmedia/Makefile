PKG_CONFIG ?= pkg-config
CC ?= gcc

default:
	$(CROSS_COMPILE)$(CC) main.c -o rmedia -O3 -g -lm `$(PKG_CONFIG) --cflags sdl2` `$(PKG_CONFIG) --libs sdl2` `$(PKG_CONFIG) --libs SDL2_ttf` `$(PKG_CONFIG) --cflags SDL2_ttf` -lpthread -Wall -Wextra -Wpedantic $(CFLAGS)
