prefix = /usr/local

all:
	gcc -o mpv-play mpv-play.c `pkg-config mpv --libs`
	gcc -o mpv-remote mpv-remote.c

install:
	install -D mpv-play \
		$(DESTDIR)$(prefix)/bin/mpv-play
	install -D mpv-remote \
		$(DESTDIR)$(prefix)/bin/mpv-remote

clean:
	: # do nothing

distclean: clean

uninstall:
	-rm -f $(DESTDIR)$(prefix)/bin/mpv-play
	-rm -f $(DESTDIR)$(prefix)/bin/mpv-remote

.PHONY: all install clean distclean uninstall
