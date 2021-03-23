prefix ?= /usr/local

VERSION = 1.1.0
VERSION_MAJOR = 1

CC = gcc
FLAGS = -std=c11 -O2 -Wno-unused-result -Wno-format-truncation

MACROS = \
	-DNDEBUG \
	-D_GNU_SOURCE \
	-D_POSIX_C_SOURCE=200112L \
	-D_XOPEN_SOURCE=600 \
	-DREMOTE_HTTP_PREFIX="\"$(prefix)/share/mpv-remote/http/public\""

INCLUDES = \
	`pkg-config mpv --cflags` \
	`pkg-config libmicrohttpd --cflags` \
	`pkg-config json-c --cflags` \
	`libgcrypt-config --cflags`

LIBS = \
	`pkg-config mpv --libs` \
	`pkg-config libmicrohttpd --libs` \
	`pkg-config json-c --libs` \
	`libgcrypt-config --libs`

CFLAGS = $(FLAGS) $(MACROS) $(INCLUDES)
LDFLAGS = $(LIBS)


# 
# Source files
# 
LIBREMOTE_SRCS = \
        libremote/cmd_rsp/cmd_rsp.c \
	libremote/command.c \
	libremote/environment.c \
	libremote/logger.c \
	libremote/status.c

PLAYER_SRCS = \
	player/http/auth.c \
	player/http/get.c \
	player/http/http.c \
	player/http/post.c \
	player/main.c \
	player/player.c

REMOTE_SRCS = \
	remote/main.c

LIBREMOTE_OBJS = \
	$(patsubst libremote/%.c, build/libremote/%.o, $(LIBREMOTE_SRCS))

PLAYER_OBJS = $(patsubst player/%.c, build/player/%.o, $(PLAYER_SRCS))

REMOTE_OBJS = $(patsubst remote/%.c, build/remote/%.o, $(REMOTE_SRCS))




all: libremote player remote
	@ :

mkdir:
	@ mkdir -p build
	@ mkdir -p build/libremote
	@ mkdir -p build/libremote/cmd_rsp
	@ mkdir -p build/player
	@ mkdir -p build/player/http
	@ mkdir -p build/remote


# 
# Common function library used by the player and the remote programs
# 
libremote: mkdir $(LIBREMOTE_OBJS)
	@ $(CC) $(CFLAGS) -shared -Wl,-soname,libmpv-remote.so.$(VERSION_MAJOR) \
		-o libmpv-remote.so.$(VERSION) $(LIBREMOTE_OBJS) \
		$(LDFLAGS) -shared
	@ ln -sf libmpv-remote.so.$(VERSION) libmpv-remote.so.$(VERSION_MAJOR)
	@ ln -sf libmpv-remote.so.$(VERSION) libmpv-remote.so
	@ mv libmpv-remote.so build
	@ mv libmpv-remote.so.$(VERSION) build
	@ mv libmpv-remote.so.$(VERSION_MAJOR) build

$(LIBREMOTE_OBJS): build/libremote/%.o: libremote/%.c
	@ $(CC) -fPIC -c $(CFLAGS) -o $@ $<


# 
# Remote controlled media player station
# 
player: $(PLAYER_OBJS)
	@ $(CC) $(CFLAGS) -o build/mpv-play $(PLAYER_OBJS) $(LDFLAGS) \
		-L./build -lmpv-remote

$(PLAYER_OBJS): build/player/%.o: player/%.c
	@ $(CC) -fPIC -c $(CFLAGS) -o $@ $<


# 
# Media player remote program
# 
remote: libremote $(REMOTE_OBJS)
	@ $(CC) $(CFLAGS) -o build/mpv-remote $(REMOTE_OBJS) $(LDFLAGS) \
		-L./build -lmpv-remote

$(REMOTE_OBJS): build/remote/%.o: remote/%.c
	@ $(CC) -fPIC -c $(CFLAGS) -o $@ $<




install:
	install -Dm 755 build/libmpv-remote.so.$(VERSION) \
		$(DESTDIR)$(prefix)/lib/libmpv-remote.so.$(VERSION)
	cp -a build/libmpv-remote.so \
		$(DESTDIR)$(prefix)/lib/libmpv-remote.so
	cp -a build/libmpv-remote.so.$(VERSION_MAJOR) \
		$(DESTDIR)$(prefix)/lib/libmpv-remote.so.$(VERSION_MAJOR)
	install -Dm 755 build/mpv-play \
		$(DESTDIR)$(prefix)/bin/mpv-play
	install -Dm 755 build/mpv-remote \
		$(DESTDIR)$(prefix)/bin/mpv-remote
	find http -type f -exec install -Dm 644 "{}" \
		"$(DESTDIR)$(prefix)/share/mpv-remote/{}" \;

clean:
	-rm -r build

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/lib/libmpv-remote.so
	-rm -f $(DESTDIR)$(prefix)/lib/libmpv-remote.so.$(VERSION_MAJOR)
	-rm -f $(DESTDIR)$(prefix)/lib/libmpv-remote.so.$(VERSION)
	-rm -f $(DESTDIR)$(prefix)/bin/mpv-play
	-rm -f $(DESTDIR)$(prefix)/bin/mpv-remote
	-rm -f -r $(DESTDIR)$(prefix)/share/mpv-remote

.PHONY: all remote mpv-remote mpv-play install clean distclean uninstall
