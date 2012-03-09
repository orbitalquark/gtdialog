# Installation

## Requirements

GCocoaDialog only requires [GTK+][] version 2.16 or higher. It runs on Linux,
Windows, and Mac OSX, provided a GTK runtime is available. Installing and
configuring the runtime is beyond the scope of this document.

[GTK+]: http://gtk.org

## Download

Download GCocoaDialog from the [project page][].

[project page]: http://foicica.com/gcocoadialog

## Compiling

Compiling the command-line program is currently only supported on Linux systems.
If you want to try and compile for Windows or Mac OSX, please see the
[Textadept][] project which compiles GCocoaDialog as a C library in the
`src/Makefile` file.

[Textadept]: http://foicica.com/textadept

### Requirements

#### Linux

Linux systems need the GTK+ development libraries. Your package manager should
allow you to install them. For Debian-based distributions like Ubuntu, the
package is typically called `libgtk2.0-dev`. Otherwise, compile and install GTK
from the [GTK+ website][]. Additionally you will need the [GNU C compiler][]
(`gcc`) and [GNU Make][] (`make`). Both should be available for your Linux
distribution through its package manager. For example, Ubuntu includes these
tools in the `build-essential` package.

[GTK+ website]: http://www.gtk.org/download/linux.html
[GNU C compiler]: http://gcc.gnu.org
[GNU Make]: http://www.gnu.org/software/make/

#### Windows

You need a C compiler that supports the C99 standard (Microsoft's does not) and
the [GTK+ for Windows bundle][] (2.22 is recommended).

[GTK+ for Windows bundle]: http://www.gtk.org/download/win32.html

#### Mac OSX

[XCode][] is needed for Mac OSX as well as [jhbuild][]. After building
`meta-gtk-osx-bootstrap` and `meta-gtk-osx-core`, you need to build
`meta-gtk-osx-themes`. Note that the entire compiling process can easily take
30 minutes or more and ultimately consume nearly 1GB of disk space. This is
pretty ridiculous for a small application like GCocoaDialog.

[XCode]: http://developer.apple.com/TOOLS/xcode/
[jhbuild]: http://sourceforge.net/apps/trac/gtk-osx/wiki/Build

### Compiling

#### Command-Line

##### Linux

For Linux systems, simply run `make` in the `src/` directory. The `gcocoadialog`
executable is created in the current directory.

##### Windows

Unsupported.

##### Mac OSX

Unsupported.

#### C Library

Include both `gcocoadialog.h` and `gcocoadialog.c` in your project and compile
`gcococadialog.c` with the `NOHELP` and `LIBRARY` flags to disable printing help
messages to `stdout` and ignoring the `main()` function.

## Installation

This section applies to the command-line program only.

### Linux

After compiling GCocoaDialog, make a symlink from it to `/usr/bin/` or elsewhere
in your `PATH`.

### Windows

Unsupported because compiling is unsupported. However, if you successfully
compiled GCocoaDialog, you probably know how to install it.

### Mac OSX

Unsupported because compiling is unsupported. However, if you successfully
compiled GCocoaDialog, you probably know how to install it.
