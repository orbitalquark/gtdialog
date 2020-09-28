## Changelog

[Atom Feed](https://github.com/orbitalquark/gtdialog/releases.atom)

### 1.5 (?)

Download:

* [gtDialog 1.5][]

Bugfixes:

* Workaround `--select-only-directories` in the curses version by returning only
  the directory of the file selected.

Changes:

* The C library can use the [progressbar][] dialog with a callback function.
* Implemented [progressbar][] dialog in curses, but only via C library.
* Changed the optionselect dialog's `--informative-text` option to `--text`.
* Extremely large filteredlists (> 10000 items) filter on a keypress timeout in
  the GUI.

[gtDialog 1.5]: https://github.com/orbitalquark/gtdialog/archive/gtdialog_1.5.zip
[progressbar]: manual.html#progressbar

### 1.4 (25 Jun 2017)

Download:

* [gtDialog 1.4][]

Bugfixes:

* None.

Changes:

* New [colorselect][] dialog for selecting colors.
* New [fontselect][] dialog for selecting fonts.

[gtDialog 1.4]: https://github.com/orbitalquark/gtdialog/archive/gtdialog_1.4.zip
[colorselect]: manual.html#color-selection-gtk-only
[fontselect]: manual.html#font-selection-gtk-only

### 1.3 (29 Aug 2016)

Download:

* [gtDialog 1.3][]

Bugfixes:

* Fixed button order in curses-based dialogs to match graphical dialogs.
* Ignore multiple labels for non-inputbox dialogs.

Changes:

* The C library's `gtdialog_type()` function now returns `GTDIALOG_UNKNOWN`
  instead of `-1` for an unrecognized dialog type string; `GTDialogType`
  enumerations have changed internally.

[gtDialog 1.3]: https://github.com/orbitalquark/gtdialog/archive/gtdialog_1.3.zip

### 1.2 (05 Jan 2015)

Download:

* [gtDialog 1.2][]

Bugfixes:

* Fixed curses memory leaks when canceling a dialog.

Changes:

* None.

[gtDialog 1.2]: https://github.com/orbitalquark/gtdialog/archive/gtdialog_1.2.zip

### 1.1 (08 Jul 2014)

Download:

* [gtDialog 1.1][]

Bugfixes:

* GTK textbox `--scroll-to bottom` works properly.
* Fixed edge cases with `--search-column` and `--output-column` options.

Changes:

* Removed `--utf8` option.
* [Inputboxes][] can have multiple entry boxes via `--text` list.
* New [optionselect][] dialog for checkbox option groups.

[gtDialog 1.1]: https://github.com/orbitalquark/gtdialog/archive/gtdialog_1.1.zip
[Inputboxes]: manual.html#inputboxes
[optionselect]: manual.html#option-selection

### 1.0 (20 Apr 2013)

Download:

* [gtDialog 1.0][]

Initial release.

[gtDialog 1.0]: https://github.com/orbitalquark/gtdialog/archive/gtdialog_1.0.zip
