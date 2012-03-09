all:gcocoadialog
gcocoadialog.o: gcocoadialog.c
	gcc -g $(shell pkg-config --cflags gtk+-2.0) -c $< -o $@
gcocoadialog: gcocoadialog.o
	gcc -g $(shell pkg-config --libs gtk+-2.0) $< -o $@
clean:
	rm gcocoadialog gcocoadialog.o

manual:
	cd doc && lua gen_manual.lua
cleandoc:
	rm -f doc/*.html
