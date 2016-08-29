# Changelog

[Atom Feed][] | [PGP Public Key][]

[Atom Feed]: feed
[PGP Public Key]: https://foicica.com/foicica.pgp

## 1.3 (29 Aug 2016)

Download:

* [gtDialog 1.3][] | [PGP -- 1.3][]

Bugfixes:

* Fixed button order in curses-based dialogs to match graphical dialogs.
* Ignore multiple labels for non-inputbox dialogs.

Changes:

* The C library's `gtdialog_type()` function now returns `GTDIALOG_UNKNOWN`
  instead of `-1` for an unrecognized dialog type string; `GTDialogType`
  enumerations have changed internally.

[gtDialog 1.3]: download/gtdialog_1.3.zip
[PGP -- 1.3]: download/gtdialog_1.3.zip.asc

## 1.2 (05 Jan 2015)

Download:

* [gtDialog 1.2][] | [PGP -- 1.2][]

Bugfixes:

* Fixed curses memory leaks when cancelling a dialog.

Changes:

* None.

[gtDialog 1.2]: download/gtdialog_1.2.zip
[PGP -- 1.2]: download/gtdialog_1.2.zip.asc

## 1.1 (08 Jul 2014)

Download:

* [gtDialog 1.1][] | [PGP -- 1.1][]

Bugfixes:

* GTK textbox `--scroll-to bottom` works properly.
* Fixed edge cases with `--search-column` and `--output-column` options.

Changes:

* Removed `--utf8` option.
* [Inputboxes][] can have multiple entry boxes via `--text` list.
* New [optionselect][] dialog for checkbox option groups.

[gtDialog 1.1]: download/gtdialog_1.1.zip
[PGP -- 1.1]: download/gtdialog_1.1.zip.asc
[Inputboxes]: manual.html#Inputboxes
[optionselect]: manual.html#Option.Selection

## 1.0 (20 Apr 2013)

Download:

* [gtDialog 1.0][] | [PGP -- 1.0][]

Initial release.

[gtDialog 1.0]: download/gtdialog_1.0.zip
[PGP -- 1.0]: download/gtdialog_1.0.zip.asc
