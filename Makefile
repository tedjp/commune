.PHONY: all clean
all: README.html commune-gui

README.html: README.md
	markdown $< > $@

commune-gui: gui.cpp lib/commune.cpp lib/commune.h lib/room.h lib/room.cpp lib/safefd.h
	g++ -std=gnu++17 -O0 -g -Wall -Wextra -o $@ gui.cpp lib/commune.cpp lib/room.cpp `pkg-config --cflags --libs gtkmm-3.0 | sed -e 's, -I, -isystem ,g'`

clean:
	rm -f README.html commune-gui
