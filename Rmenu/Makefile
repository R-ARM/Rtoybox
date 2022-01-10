PKG_CONFIG ?= pkg-config

all:
	# this is so stupid
	cat initlist.h.in | sed 's/path/progs/g;s/nname/prog/g;s/Nname/Prog/g;s/NNAME/PROG/g;s/TTYPE/PROG/g;s/ISSET/0/g' > proglist.h
	cat initlist.h.in | sed 's/path/games/g;s/nname/game/g;s/Nname/Game/g;s/NNAME/GAME/g;s/TTYPE/GAME/g;s/ISSET/0/g' > gamelist.h
	cat initlist.h.in | sed 's/path/settings/g;s/nname/sets/g;s/Nname/Settings/g;s/NNAME/GAME/g;s/TTYPE/PARENT/g;s/ISSET/1/g;' > setslist.h
	$(CC) main.c -o rmenu -O3 -g `$(PKG_CONFIG) --cflags sdl2` `$(PKG_CONFIG) --libs sdl2` `$(PKG_CONFIG) --libs SDL2_ttf` `$(PKG_CONFIG) --cflags SDL2_ttf` -lpthread -Wall -Wextra -Wpedantic $(CFLAGS)
