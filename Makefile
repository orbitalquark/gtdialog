all:gcocoadialog
gcocoadialog.o:
	gcc -g $(shell pkg-config --cflags gtk+-2.0) -c gcocoadialog.c -o gcocoadialog.o
gcocoadialog: gcocoadialog.o
	gcc -g $(shell pkg-config --libs gtk+-2.0) gcocoadialog.o -o gcocoadialog
clean:
	rm gcocoadialog gcocoadialog.o
