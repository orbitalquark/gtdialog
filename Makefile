# Copyright 2009-2012 Mitchell mitchell.att.foicica.com.

ifndef NCURSES
FLAGS = -DGTK $(shell pkg-config --cflags gtk+-2.0)
LIBS = $(shell pkg-config --libs gtk+-2.0)
else
FLAGS = -DNCURSES
LIBS = -lncursesw -lcdk
endif

all: gtdialog
gtdialog.o: gtdialog.c
	gcc -g $(FLAGS) -c $< -o $@
gtdialog: gtdialog.o
	gcc -g $(FLAGS) $< -o $@ $(LIBS)
clean:
	rm gtdialog gtdialog.o

manual: doc/*.md *.md
	doc/bombay -d doc -t doc --title gtDialog --navtitle Manual $^
cleandoc:
	rm -f doc/*.html
