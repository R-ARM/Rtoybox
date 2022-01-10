PKG_CONFIG ?= pkg-config
CC ?= gcc

all:
	$(CROSS_COMPILE)$(CC) main.c -o r_srv -O3 `$(PKG_CONFIG) --cflags sdl2` `$(PKG_CONFIG) --libs sdl2` `$(PKG_CONFIG) --libs SDL2_ttf` `$(PKG_CONFIG) --cflags SDL2_ttf` -lpthread -Wall -Wextra -Wpedantic $(CFLAGS)
	$(CROSS_COMPILE)$(CC) client.c -o r_clt -O3 `$(PKG_CONFIG) --cflags sdl2` `$(PKG_CONFIG) --libs sdl2` `$(PKG_CONFIG) --libs SDL2_ttf` `$(PKG_CONFIG) --cflags SDL2_ttf` -lpthread -Wall -Wextra -Wpedantic $(CFLAGS)
