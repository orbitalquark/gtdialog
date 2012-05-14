# Copyright 2009-2012 Mitchell mitchell.att.foicica.com.

ifndef NCURSES
FLAGS = -DGTK $(shell pkg-config --cflags gtk+-2.0)
LIBS = $(shell pkg-config --libs gtk+-2.0)
else
FLAGS = -DNCURSES
LIBS = -lncursesw -lcdk
endif

all:gcocoadialog
gcocoadialog.o: gcocoadialog.c
	gcc -g $(FLAGS) -c $< -o $@
gcocoadialog: gcocoadialog.o
	gcc -g $(FLAGS) $< -o $@ $(LIBS)
clean:
	rm gcocoadialog gcocoadialog.o

manual: doc/*.md *.md
	doc/bombay -d doc -t doc --title GCocoaDialog --navtitle Manual $^
cleandoc:
	rm -f doc/*.html
