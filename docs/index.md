## Introduction

gtDialog is a cross-platform application for creating interactive dialogs from
either the command line or from an application. It is written completely in C
and can display graphical dialogs using [GTK][] or display terminal-based
dialogs using a curses implementation like [ncurses][]. gtDialog is inspired by
cocoaDialog and shares many of its dialog types and arguments. You can use
gtDialog to easily create interactive dialogs for yes/no confirmations, textual
input, file selections, selection lists and more.

[GTK]: https://gtk.org
[ncurses]: https://invisible-island.net/ncurses/ncurses.html

## Features

* Graphical (GTK) or terminal-based (curses) dialogs.
* 17 different dialog types including:
  + Messageboxes
  + Inputboxes
  + File selections
  + Textbox
  + Progressbar
  + Dropdown lists
  + Filtered list
  + Option list
  + Color selector (GTK only)
  + Font selector (GTK only)
* No knowledge of GTK or curses is required to use either the command line
  program or the C library.
* Any programming language with shell access can utilize the command line
  program.

## Requirements

gtDialog requires only GTK version 2.16 or later or a curses implementation
(like [ncurses][] or [pdcurses][]) and [CDK][].

[ncurses]: https://invisible-island.net/ncurses/ncurses.html
[pdcurses]: https://pdcurses.org
[CDK]: https://invisible-island.net/cdk/

## Download

gtDialog releases can be found [here][].

[here]: https://github.com/orbitalquark/gtdialog/releases

## Compile

gtDialog can be built as a standalone command line application, or compiled as a
C library into an existing application.

Requirements:

* A C99-compliant compiler like [GCC][] or [Clang][]. (e.g. provided by
  the `build-essential` package on Debian-based distributions like Ubuntu.)
* [GNU Make][]
* GTK development libraries (e.g. `libgtk2.0-dev`).
* Curses and CDK development libraries (e.g. `libncurses5-dev` and
  `libcdk5-dev`).

The standalone command line application currently only builds on Linux and BSD,
though it can be cross-compiled for Windows and macOS (which is beyond the scope
of this document). The following table provides a list of `make` rules for
building gtDialog on Linux and BSD. (On BSD, substitute `make` with `gmake`.)

Command              |Description
---------------------|-----------
`make`               |Builds gtDialog with GTK
`make DEBUG=1`       |Optionally builds gtDialog with debug symbols
`make install`       |Optionally installs gtDialog (to */usr/local* by default)
`make curses`        |Builds gtDialog with curses and cdk
`make curses install`|Optionally installs the curses version of gtDialog
`make clean`         |Deletes all compiled files, leaving only source files

If you want to install gtDialog into a non-standard location, you can specify
that location using the `DESTDIR` variable. For example:

    make install DESTDIR=/prefix/to/install/to

In order to compile the C library into your existing application, add
*gtdialog.h* and *gtdialog.c* to your project's sources and pass either the
`-DGTK` or `-DCURSES` flag to the compiler, followed by `-DLIBRARY` and
optionally `-DNOHELP`.

[GCC]: https://gcc.gnu.org
[Clang]: https://clang.llvm.org/
[GNU Make]: https://www.gnu.org/software/make/

## Usage

The standalone command line application is executed as follows:

    gtdialog type arguments

where `type` specifies which dialog to use, and `arguments` is a set of
arguments for that dialog type. For example:

    gtdialog yesno-msgbox --title Confirm --text "Quit?" \
      --icon gtk-dialog-question

Running `gtdialog help` shows a list of dialog types along with a brief
description for each, and running `gtdialog help type` shows all available
arguments for that dialog type.

gtDialog comes with a reference guide in its *docs/* directory that covers all
dialog types and their arguments. It is also available [online][].

In order to use gtDialog as a C library, add `#include "gtdialog.h"` to the
source file you want to invoke gtDialog from, and then call `gtdialog()` with a
`GTDialogType type`, followed by an `int argc` and a `char *argv[]` of dialog
arguments, just like those that would be given to the command line application.
You can use the helper function `gtdialog_type()` to get a `GTDialogType` from a
string. You are responsible for calling `free()` on the string returned by
`gtdialog()`. For example:

    #include "gtdialog.h"
    ...
    void goto_line() {
      const char *argv[] = {
        "--title", "Goto Line", "--informative-text", "Line:", "--text", "1"
      };
      char *line_num = gtdialog(GTDIALOG_INPUTBOX, 6, argv));
      ...
      free(line_num);
    }

[online]: https://orbitalquark.github.com/gtdialog/manual.html

## Contribute

gtDialog is [open source][]. Feel free to report bugs and submit patches either
to the [mailing list][] or to me personally (mitchell.att.foicica.com).

[open source]: https://github.com/orbitalquark/gtdialog
[mailing list]: https://foicica.com/lists
