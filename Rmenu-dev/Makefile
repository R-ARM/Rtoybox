PKG_CONFIG ?= pkg-config

default:
	gcc main.c -o rmenu2 -O3 -g `$(PKG_CONFIG) --cflags sdl2` `$(PKG_CONFIG) --libs sdl2` `$(PKG_CONFIG) --libs SDL2_ttf` `$(PKG_CONFIG) --cflags SDL2_ttf` -lpthread -Wall -Wextra -Wpedantic $(CFLAGS)
