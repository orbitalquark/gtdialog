# Copyright 2009-2020 Mitchell mitchell.att.foicica.com.

CC = gcc
PREFIX ?= /usr/local

bin_dir = $(DESTDIR)$(PREFIX)/bin

gtdialog_flags = -std=c99 -pedantic -W -Wall -Wno-unused
ifeq (, $(findstring curses, $(MAKECMDGOALS)))
  ifndef GTK3
    gtk_version = 2.0
  else
    gtk_version = 3.0
  endif
  gtk_flags = -DGTK $(shell pkg-config --cflags gtk+-$(gtk_version))
  gtk_libs = $(shell pkg-config --libs gtk+-$(gtk_version))
  install_targets = gtdialog
else
  curses_flags = -DCURSES
  curses_libs = -lncursesw -lcdk
  install_targets = gtdialog-curses
endif

ifdef DEBUG
  CFLAGS += -g
endif

# Build.

all: gtdialog
curses: gtdialog-curses
gtdialog.o: gtdialog.c
	$(CC) -c $(CFLAGS) $(gtdialog_flags) $(gtk_flags) -o $@ $<
gtdialog-curses.o: gtdialog.c
	$(CC) -c $(CFLAGS) $(gtdialog_flags) $(curses_flags) -o $@ $<
gtdialog: gtdialog.o
	$(CC) $(CFLAGS) $(gtk_flags) -o $@ $< $(gtk_libs) $(LDFLAGS)
gtdialog-curses: gtdialog-curses.o
	$(CC) $(CFLAGS) $(curses_flags) -o $@ $< $(curses_libs) $(LDFLAGS)
clean: ; rm -f gtdialog gtdialog-curses *.o

# Install/Uninstall.

install: $(install_targets)
	install -d $(bin_dir)
	install $^ $(bin_dir)
uninstall: ; rm $(bin_dir)/gtdialog*

# Documentation.

docs: docs/index.md $(wildcard docs/*.md) | docs/_layouts/default.html
	for file in $(basename $^); do \
		cat $| | docs/fill_layout.lua $$file.md > $$file.html; \
	done
docs/index.md: README.md
	sed -e 's/^\# [[:alpha:]]\+/## Introduction/;' -e \
		's|https://[[:alpha:]]\+\.github\.io/[[:alpha:]]\+/||;' $< > $@
cleandocs: ; rm -f docs/*.html docs/index.md
