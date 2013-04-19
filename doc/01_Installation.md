# Installation

## Requirements

The [GTK+][] version of gtDialog only requires GTK+ version 2.16 or higher. It
runs on Linux, Windows, and Mac OSX, provided a GTK runtime is available.
Installing and configuring the runtime is beyond the scope of this document, but
[Textadept][] bundles in a GTK runtime so you can use that for reference.

The curses version of gtDialog requires only a curses implementation (like
[ncurses][] or [pdcurses][]) and [CDK][] and runs on Linux, Windows, and Mac
OSX.

[GTK+]: http://gtk.org
[Textadept]: http://foicica.com/textadept
[ncurses]: http://invisible-island.net/ncurses/ncurses.html
[pdcurses]: http://pdcurses.sourceforge.net
[CDK]: http://invisible-island.net/cdk/

## Download

Download gtDialog from the project's [download page][].

[download page]: http://foicica.com/gtdialog/download

## Compiling

Compiling the command-line program is currently only supported on Linux systems.
If you want to try and compile for Windows or Mac OSX, please see the
[Textadept][] project which compiles gtDialog as a C library in its
*src/Makefile* file.

[Textadept]: http://foicica.com/textadept

### Requirements

#### Linux

Linux systems need the GTK+ development libraries (or the ncurses and CDK
libraries). Your package manager should allow you to install them. For
Debian-based distributions like Ubuntu, the package is typically called
`libgtk2.0-dev` (or `libncurses5-dev` and `libcdk5-dev`). Otherwise, compile and
install GTK from the [GTK+ website][] (or ncurses and CDK from their
[respective][] [websites][]). Additionally you will need the [GNU C compiler][]
(`gcc`) and [GNU Make][] (`make`). Both should be available for your Linux
distribution through its package manager. For example, Ubuntu includes these
tools in the `build-essential` package.

[GTK+ website]: http://www.gtk.org/download/linux.html
[respective]: http://invisible-island.net/ncurses/ncurses.html#download_ncurses
[websites]: http://invisible-island.net/cdk/#download
[GNU C compiler]: http://gcc.gnu.org
[GNU Make]: http://www.gnu.org/software/make/

#### Windows

You need a C compiler and the [GTK+ for Windows bundle][] (2.24 is recommended).

[GTK+ for Windows bundle]: http://www.gtk.org/download/win32.html

#### Mac OSX

For native GTK gtDialog, [XCode][] is needed for Mac OSX as well as [jhbuild][].
After building `meta-gtk-osx-bootstrap` and `meta-gtk-osx-core`, you need to
build `meta-gtk-osx-themes`. Note that the entire compiling process can easily
take 30 minutes or more and ultimately consume nearly 1GB of disk space. This is
pretty ridiculous for a small application like gtDialog.

[XCode]: http://developer.apple.com/TOOLS/xcode/
[jhbuild]: http://sourceforge.net/apps/trac/gtk-osx/wiki/Build

### Compiling

#### Command-Line

##### Linux

For Linux systems, simply run `make` in the *src/* directory. The `gtdialog`
executable is created in the current directory. To build the curses version,
run `make curses`.

Note: if the build fails because *cdk.h* is not found and you know it is
installed, you might have to manually patch *gtdialog.c* to use the right path
(e.g. `#include <cdk/cdk.h>` instead of `#include <cdk.h>`).

##### Windows

Unsupported.

##### Mac OSX

Unsupported.

#### C Library

Include both *gtdialog.h* and *gtdialog.c* in your project and compile
*gtdialog.c* with either the `GTK` or `CURSES` flag and optionally the `NOHELP`
and `LIBRARY` flags to disable printing help messages to `stdout` and ignoring
the `main()` function, respectively.

## Installation

This section applies to the command-line program only.

### Linux

After compiling gtDialog, run the usual `make install` or `sudo make install`
depending on your privilages. The default prefix is */usr/local* but you can
change this by setting `DESTDIR` (e.g.
`make install DESTDIR=/prefix/to/install/to`). Similarly, `make curses install`
installs the curses version.

### Windows

Unsupported because compiling is unsupported. However, if you successfully
compiled gtDialog, you probably know how to install it.

### Mac OSX

Unsupported because compiling is unsupported. However, if you successfully
compiled gtDialog, you probably know how to install it.
