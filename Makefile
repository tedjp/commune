.PHONY: all clean docs

all: commune-gui
docs: README.html

README.html: README.md
	if markdown $< > $@.tmp; then \
		mv $@.tmp $@; \
	else \
		rm -f $@; \
	fi

commune-gui: gui.cpp lib/commune.cpp lib/commune.h lib/room.h lib/room.cpp lib/safefd.h
	g++ -std=gnu++14 -O0 -g -Wall -Wextra -o $@ gui.cpp lib/commune.cpp lib/room.cpp `pkg-config --cflags --libs gtkmm-3.0 | sed -e 's, -I, -isystem ,g'`

clean:
	rm -f README.html commune-gui
