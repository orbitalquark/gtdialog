# Copyright 2009-2014 Mitchell mitchell.att.foicica.com.

CC = gcc
PREFIX ?= /usr/local

bin_dir = $(DESTDIR)$(PREFIX)/bin

gtdialog_flags = -std=c99 -pedantic -Wall -Wno-unused-variable \
                 -Wno-unused-but-set-variable
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
  curses_flags = -DCURSES -D_POSIX_C_SOURCE=200809L
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

doc: manual
manual: doc/*.md *.md | doc/bombay
	doc/bombay -d doc -t doc --title gtDialog $^
cleandoc: ; rm -f doc/manual.html

# Package.

basedir = gtdialog_$(shell grep '^\#\#' CHANGELOG.md | head -1 | \
                           cut -d ' ' -f 2)

release: doc
	hg archive $(basedir)
	rm $(basedir)/.hg*
	cp -rL doc $(basedir)
	zip -r /tmp/$(basedir).zip $(basedir) && rm -r $(basedir)

# External dependencies.

bombay_zip = bombay.zip

$(bombay_zip):
	wget "http://foicica.com/hg/bombay/archive/tip.zip" && mv tip.zip $@
doc/bombay: | $(bombay_zip)
	mkdir $(notdir $@) && unzip -d $(notdir $@) $| && \
		mv $(notdir $@)/*/* $(dir $@)
