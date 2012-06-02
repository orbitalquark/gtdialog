# Usage

## Overview

gtDialog can be used as either a command-line program for shell scripts or as a
C library for applications.

### Using the Command-Line

gtDialog is executed like this:

    gtdialog type options

Where `type` is specifies which dialog to use and `options` is a set of options
for that dialog type.

### Using the C Library

In order to use gtDialog as a C library, first include `gtdialog.h` and
`gtdialog.c` in your application. Then call `gtdialog()` with a `GTDialogType`
followed by an `int argc` and a `char *argv[]` like you would have for a
command-line application.  You can use a helper function `gtdialog_type()` for
getting a `GTDialogType` from a string. An example is in the `main()` function
of `gtdialog.c`.

## Acknowledgements

The following documentation is taken from [cocoaDialog][].

[cocoaDialog]: http://cocoadialog.sf.net

## Global Options

The following options can be supplied for any of the dialogs.

* `‑‑title` "_text for title_": Sets the window's title.
* `‑‑string‑output`: Makes yes/no/ok/cancel buttons return values as "gtk-yes",
  "gtk-no", "gtk-ok", or "gtk-cancel" instead of integers. When used with custom
  button labels, returns the label you provided. If you use a
  [GTK stock item][], (ie: "gtk-copy") then that exact text is returned, not the
  text displayed on the button.
* `‑‑no‑newline`: By default, return values will be printed with a trailing
  newline. This will suppress that behavior.
  _Note that when a dialog returns multiple lines this will only suppress the_
  _trailing newline on the last line._
* `‑‑width` _integer_: Sets the width of the window. It's not advisable to use
  this option without good reason, and some dialogs won't even respond to it.
  The automatic size of most windows should suffice.
* `‑‑height` _integer_: Sets the height of the window. It's not advisable to use
  this option without good reason, and some dialogs won't even respond to it.
  The automatic size of most windows should suffice.
* `‑‑debug`: If you are not getting the results you expect, try turning on this
  option. When there is an error, it will print `ERROR:` followed by the error
  message.
* `‑‑help`: Gives a list of options and a link to this page.

[GTK stock item]: http://developer.gnome.org/gtk/unstable/gtk-Stock-Items.html

## Return Values

### Command Line

The return value of a dialog will simply be printed to `stdout`. This makes it
trivial to determine user input in any shell, programming, or scripting
language.

For example in Bash:

    val=`gtdialog yesno-msgbox --string-output`
    echo "You pressed the $val button"

Dialogs that return multiple values will print each on its own line (separated
by newlines).

### C Library

The return value of `gtdialog()` is a string that contains what the dialog would
print to `stdout` when run from the command-line. It is your responsibility to
`free()` that string when you are finished with it.

## Limitations

Despite trying to be a cocoaDialog clone, gtDialog currently does not support
the following dialogs and options (but it may at a later point in time):

* `bubble`
* `--icon`
* `--icon-file`
* `--select-directories`
* `--packages-as-directories`
* `--no-create-directories`
* `--pulldown`
* `--debug`

- - -

## Messageboxes

- - -

<a id="msgbox"/>
### `msgbox`

This dialog provides a generic message box. It allows you to customize the
labels of the buttons. At least one button (`--button1`) must be specified. If
labels for the other buttons are not given, the buttons will not appear on the
message box.

Buttons go from right to left. `--button1` is the right-most button.

Options: (in addition to [global options](#Global.Options))

* `--text` "_main text message_": This is the main, bold message text.
* `--informative-text` "_extra informative text to be displayed_": This is the
  extra, smaller message text.
* `--icon` _stockIconName_: The name of the stock icon to use. This is
  incompatible with `--icon-file`. Default is no icon.
* `--icon-file` "_/full/path/to/icon file_": The full path to the custom icon
  image you would like to use. Almost every image format is accepted. This is
  incompatible with the `--icon` option.
* `--float`: Float on top of all windows.
* `--timeout`: The amount of time, in seconds, that the window will be displayed
  if the user does not click a button. Does not time out by default.
* `--button1`: **Required**. This is the right-most button.
* `--button2`: This is the middle button.
* `--button3`:  This is the left-most button. This will not be displayed if
  there is no `--button2` label specified.

Return:

* `1`, `2`, or `3` depending on which button was pressed; or the label of the
  button if the `--string-output` option is given.
* If the dialog times out, it will return `0` or `timeout`.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`.

Usage:

    gtdialog msgbox --no-newline \
      --text "What's your favorite OS?" \
      --informative-text "The 'Cancel' label auto-binds that button to esc" \
      --button1 "OS X" --button2 "GNU/Linux" --button3 "Cancel"

See also:

* [yesno-msgbox](#yesno-msgbox)
* [ok-msgbox](#ok-msgbox)

- - -

<a id="ok-msgbox"/>
### `ok-msgbox`

This dialog provides a standard Ok/Cancel message box.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_main text message_": This is the main, bold message text.
* `‑‑informative‑text` "_extra informative text to be displayed_": This is the
  extra, smaller text.
* `‑‑no‑cancel`: Don't show a cancel button, only "Ok".
* `‑‑icon` _stockIconName_: The name of the stock icon to use. This is
  incompatible with `--icon-file`. Default is no icon.
* `‑‑icon‑file` "_/full/path/to/icon file_": The full path to the custom icon
  image you would like to use. Almost every image format is accepted. This is
  incompatible with the `--icon` option.
* `‑‑float`: Float on top of all windows.
* `‑‑timeout` _numSeconds_: The amount of time, in seconds, that the window will
  be displayed if the user does not click a button. Does not time out by
  default.

Return:

* `1` for ok, `2` for cancel depending on which button was pressed; `gtk-ok` or
  `gtk-cancel` if the `--string-output` option is given.
* If the dialog times out, it will return `0` or `timeout`.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`.

Usage:

    gtdialog ok-msgbox --text "We need to make sure you see this message" \
      --informative-text "(Yes, the message was to inform you about itself)" \
      --no-newline --float

- - -

<a id="yesno-msgbox"/>
### `yesno-msgbox`

This dialog provides a standard Yes/No/Cancel message box.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_main text message_": This is the main, bold message text.
* `‑‑informative‑text` "_extra informative text to be displayed_": This is the
  extra, smaller text.
* `‑‑no‑cancel`: Don't show a cancel button.
* `‑‑icon` _stockIconName_: The name of the stock icon to use. This is
  incompatible with `--icon-file`. Default is no icon.
* `‑‑icon‑file` "_/full/path/to/icon file_": The full path to the custom icon
  image you would like to use. Almost every image format is accepted. This is
  incompatible with the `--icon` option.
* `‑‑float`: Float on top of all windows.
* `‑‑timeout` _numSeconds_: The amount of time, in seconds, that the window will
  be displayed if the user does not click a button. Does not time out by
  default.

Return:

* `1` for yes, `2` for no, `3` for cancel depending on which button was pressed;
  `gtk-yes`, `gtk-no`, or `gtk-cancel` if the `--string-output` option is given.
* If the dialog times out, it will return `0` or `timeout`.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`.

Usage:

    gtdialog yesno-msgbox --no-cancel --string-output --no-newline \
      --text  "This is a simple first example" \
      --informative-text "We're just going to echo the string output"

- - -

## Inputboxes

- - -

<a id="inputbox"/>
### `inputbox`

This dialog provides a one line input box and customizable buttons. At least
one button (`--button1`) must be specified. If labels for the other buttons are
not given, the buttons will not appear on the message box.

Buttons go from right to left. `--button1` is the right-most button.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_initial text_": This is the initial text in the input box.
* `‑‑informative‑text` "_extra informative text to be displayed_": This is the
  text for the label above the input box.
* `‑‑button1` "_label for button 1_": **Required**. This is the right-most
  button.
* `‑‑button2` "_label for button 2_": This is the middle button.
* `‑‑button3` "_label for button 3_": This is the left-most button. This will
  not be displayed if there is no `--button2` label specified.
* `‑‑float`: Float on top of all windows.
* `‑‑timeout` _numSeconds_: The amount of time, in seconds, that the window will
  be displayed if the user does not click a button. Does not time out by
  default.
* `‑‑no‑show`: This makes it a secure inputbox. Instead of what the user types,
  only dots will be shown.

Return:

* `1`, `2`, or `3` depending on which button was pressed; or the label of the
  button if the `--string-output` option is given. On the next line will be the
  text provided by the user in the textbox.
* On a timeout, it will return `0` or `timeout`, and the text from the textbox
  _will not_ be returned.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`, and the text from the textbox _will not_ be returned.

Usage:

    gtdialog inputbox --title "Search" --no-newline \
      --informative-text "Enter your search term" \
      --text "foobar" \
      --button1 "Search" --button2 "Search all" \
      --width 600

See also:

* [standard-inputbox](#standard-inputbox)

- - -

<a id="standard-inputbox"/>
### `standard-inputbox`

This dialog provides a standard input box with "Ok" and "Cancel" buttons.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_initial text_": This is the initial text in the input box.
* `‑‑informative‑text` "_extra informative text to be displayed_": This is the
  text for the label above the input box.
* `‑‑no‑cancel`: Don't show a cancel button.
* `‑‑float`: Float on top of all windows.
* `‑‑timeout` _numSeconds_: The amount of time, in seconds, that the window will
  be displayed if the user does not click a button. Does not time out by
  default.
* `‑‑no‑show`: This makes it a secure inputbox. Instead of what the user types,
  only dots will be shown.

Return:

* `1` for ok, `2` for cancel depending on which button was pressed; `gtk-ok`, or
 `gtk-cancel` if the `--string-output` option is given. On the next line will be
 the text provided by the user in the textbox.
* On a timeout, it will return `0` or `timeout`, and the text from the textbox
  _will not_ be returned.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`, and the text from the textbox _will not_ be returned.

Usage:

    gtdialog standard-inputbox --title "Your Name" --no-newline \
      --informative-text "Enter your name"

- - -

<a id="secure-inputbox"/>
### `secure-inputbox`

This is an alias for running an [inputbox](#inputbox) with the `--no-show`
option. All options available to [inputbox](#inputbox) are available to
`secure-inputbox`.

- - -

<a id="secure-standard-inputbox"/>
### `secure-standard-inputbox`

This is an alias for running an [standard-inputbox](#standard-inputbox) with the
`--no-show` option. All options available to
[standard-inputbox](#standard-inputbox) are available to
`secure-standard-inputbox`.

- - -

## File Selections

- - -

<a id="fileselect"/>
### `fileselect`

This dialog provides a file selection window.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_main text message_": This is the text displayed at the top of the
  fileselect window.
* `‑‑select‑directories`: Allow the user to select directories as well as files.
  Default is to disallow it.
* `‑‑select‑only‑directories`: Allows the user to select only directories.
* `‑‑packages‑as‑directories`: Allows the user to navigate into packages as if
  they were directories, rather than selecting the package as a file.
* `‑‑select‑multiple`: Allow the user to select more than one file. Default is
  to allow only one file/directory selection.
* `‑‑with‑extensions` _list of extensions_: Limit selectable files to ones with
  these extensions. `list of extensions` should be space separated, and given as
  multiple arguments (ie: don't double quote the list).

  Example: `gtdialog fileselect --with-extensions .c .h .m .txt`

  The period/dot at the start of each extension is optional.
* `‑‑with‑directory` _directory_: Start the file select window in `directory`.
  The default value is up to the system, and will usually be the last directory
  visited in a file select dialog.
* `‑‑with‑file` _file_: Start the file select window with `file` already
  selected. By default no file will be selected. _This must be used with_
  `--with-directory`. It should be the filename of a file within the directory.

Return:

* The files or directories selected by the user, or nothing if they cancel.

Usage:

    gtdialog fileselect \
      --title "This is a fileselect"\
      --text "Choose the source file for the main controller" \
      --with-extensions .c .m .cpp

- - -

<a id="filesave"/>
### `filesave`

This dialog provides a file save window, which allows the user to select a
file, or specify a new file.

This dialog allows the user to create directories. However, if the user
specifies a file that does not yet exist, it _will not be created_. This is a
task for your script.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_main text message_": This is the text displayed at the top of the
  filesave window.
* `‑‑packages‑as‑directories`: Allows the user to navigate into packages as if
  they were directories, rather than selecting the package as a file.
* `‑‑no‑create‑directories`: Prevents the user from creating new directories.
* `‑‑with‑extensions` _list of extensions_: Limit selectable files (including
  files the user creates) to ones with these extensions. `list of extensions`
  should be space separated, and given as multiple arguments (ie: don't double
  quote the list).

  Example: `gtdialog filesave --with-extensions .c .h .m .txt`

  The period/dot at the start of each extension is optional.
* `‑‑with‑directory` _directory_: Start the file save window in `directory`. The
  default value is up to the system, and will usually be the last directory
  visited in a file dialog.
* `‑‑with‑file` _file_: Start the file save window with `file` already selected.
  By default no file will be selected. _This must be used with_
  `--with-directory`. It should be the filename of a file within the directory.

Return:

* The file selected (which may not exist), or nothing if they cancel.

- - -

## Textboxes

- - -

<a id="textbox"/>
### `textbox`

This is a text box with a large text area.

At least one button (`--button1`) must be specified. If labels for the other
buttons are not given, the buttons will not appear on the text box.

Buttons go from right to left. `--button1` is the right-most button.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_main text message_": This is the text that goes into the text box.
  This value overrides `--text-from-file`.
* `‑‑text‑from‑file` _filename_: Fills the text box with the contents of
  `filename`.
* `‑‑informative‑text` "_extra informative text to be displayed_": This is the
  message above the text box.
* `‑‑button1` "_label for button 1_": **Required**. This is the right-most
  button.
* `‑‑button2` "_label for button 2_": This is the middle button.
* `‑‑button3` "_label for button 3_": This is the left-most button. This will
  not be displayed if there is no `--button2` label specified.
* `‑‑editable`: Makes the text box editable. When this option is set, the return
  value for the button will be followed with the contents of the text box.
* `‑‑focus‑textbox`: This option is only useful when `--editable` is set. This
  makes the initial focus on the textbox rather than the rightmost button.
* `‑‑selected`: Selects all the text in the text box.
* `‑‑scroll-to` _bottom\_or\_top_: Where `bottom_or_top` is one of `bottom` or
  `top`. Causes the text box to initially scroll to the bottom or top if the
  text it contains is larger than its current view. Default is `top`.
* `‑‑float`: Float on top of all windows.
* `‑‑timeout` _numSeconds_: The amount of time, in seconds, that the window will
  be displayed if the user does not click a button. Does not time out by
  default.

Return:

* `1`, `2`, or `3` depending on which button was pressed; or the label of the
  button if the `--string-output` option is given.
* If the `--editable` option was given, it will print the contents of the text
  box following the return value for the button.
* On a timeout, it will return `0` or `timeout`, and the text from the textbox
  _will not_ be returned.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`, and the text from the textbox _will not_ be returned.


Usage:

    gtdialog textbox --title "License" --no-newline \
        --informative-text "Do you agree with the terms of this license?" \
        --text-from-file COPYING --button1 Ok --button2 Cancel

    gtdialog textbox --title "License" --no-newline \
        --informative-text "Do you agree with the terms of this license?" \
        --text "This is the text of the license...." \
        --button1 Ok --button2 Cancel

- - -

## Progressbars

- - -

<a id="progressbar"/>
### `progressbar`

The progress bar is a bit different, and slightly more complex than the other
dialogs. It reads input on `stdin` and displays until it reads an `EOF` (or the
stop button is pressed).

Input for the progress bar is in the form:
`newPercent updated text to be displayed` and must be terminated by a newline.
If you want to leave the current text intact, just provide the new percent.
`newPercent` should be a number.

Examples (the first sets new text, the second leaves the old text):

    26 We're now at 26%
    26

In your code it would like this: `"26 We're at 26%\n"`. That newline is
important.

If the `--stoppable` option was given, it also accepts the following lines:
`stop enable` and `stop disable`.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_initial text to display_": This is the text that will be initially
  displayed.
* `‑‑percent` _number_: Initial percentage, between 0 and 100, for the progress
  bar.
* `‑‑indeterminate`: This option makes the progress bar an animated "barbershop
  pole" (for lack of better description). It does not indicate how far the
  operations you're performing have progressed; it just shows that your
  application/script is busy. You can still update the text of the label when
  writing to gtdialog's `stdin` - and it doesn't matter what percentage you
  feed it.
* `‑‑float`: Float on top of all windows.
* `‑‑stoppable`: Show the stop button.

Return:

* `stopped` if the user pressed the stop button and confirmed it, and stopping
  is currently enabled.

- - -

## Dropdowns

- - -

<a id="dropdown"/>
### `dropdown`

This dialog provides a dropdown list of items to select from and customizable
buttons. Values for the dropdown list must be provided. At least one button
(`--button1`) must be specified. If labels for the other buttons are not given,
the buttons will not appear on the message box.

Buttons go from right to left. --button1 is the right-most button.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_text_": This is the text for the label above the dropdown box.
* `‑‑items` _list of values_: **Required**. These are the labels for the options
  provided in the dropdown box. `list of values` should be space separated, and
  given as multiple arguments (ie: don't double quote the entire list. Provide
  it as you would multiple arguments for any shell program). The first item in
  the list is always selected by default.

  Example: `gtdialog dropdown --text "Favorite OS?" --items "GNU/Linux" `
  `"OS X" Windows Amiga "TI 89" --button1 "Ok"`
* `‑‑pulldown`: Sets the style to a pull-down box, which differs slightly from
  the default pop-up style. The first item remains visible. This option probably
  isn't very useful for a single-function dialog such as those gtDialog
  provides, but it's been included it just in case it is. To see how their
  appearances differ, just try them both.
* `‑‑button1` "_label for button 1_": **Required**. This is the right-most
  button.
* `‑‑button2` "_label for button 2_": This is the middle button.
* `‑‑button3` "_label for button 3_": This is the left-most button. This will
  not be displayed if there is no `--button2` label specified.
* `‑‑exit‑onchange`: Makes the program exit immediately after the selection
  changes, rather than waiting for the user to press one of the buttons. This
  makes the return value for the button `4` (for both regular output and with
  `--string-output`).
* `‑‑float`: Float on top of all windows.
* `‑‑timeout` _numSeconds_: The amount of time, in seconds, that the window will
  be displayed if the user does not click a button. Does not time out by
  default.

Return:

* `1`, `2`, or `3` depending on which button was pressed; or the label of the
  button if the `--string-output` option is given.
* Returns `4` (for both regular and `--string-output`) if the user didn't press
  a button due to `--exit-onchange`.
* On the following line will be the index (zero-based) of the selected item, or
  its label if the `--string-output` option is given.
* On a timeout, it will return `0` or `timeout`, and the selected item
  _will not_ be returned.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`, and the selected item _will not_ be returned.

Usage:

    gtdialog dropdown --title "Preferred OS" --no-newline \
      --text "What is your favorite OS?" \
      --items "Mac OS X" "GNU/Linux" "Windows" --button1 'That one!' \
      --button2 Nevermind

See also:

* [standard-dropdown](#standard-dropdown)

- - -

<a id="standard-dropdown"/>
### `standard-dropdown`

This dialog provides a dropdown list of items to select from and the standard
"Ok" and "Cancel" buttons. Values for the dropdown list must be provided.

Options: (in addition to [global options](#Global.Options))

* `‑‑text` "_text_": This is the text for the label above the dropdown box.
* `‑‑items` _list of values_: **Required**. These are the labels for the options
  provided in the dropdown box. `list of values` should be space separated, and
  given as multiple arguments (ie: don't double quote the entire list. Provide
  it as you would multiple arguments for any shell program). The first item in
  the list is always selected by default.

  Example: `gtdialog dropdown --text "Favorite OS?" --items "GNU/Linux" `
  `"OS X" Windows Amiga "TI 89" --button1 "Ok"`
* `‑‑pulldown`: Sets the style to a pull-down box, which differs slightly from
  the default pop-up style. The first item remains visible. This option probably
  isn't very useful for a single-function dialog such as those gtDialog
  provides, but it's been included it just in case it is. To see how their
  appearances differ, just try them both.
* `‑‑exit‑onchange`: Makes the program exit immediately after the selection
  changes, rather than waiting for the user to press one of the buttons. This
  makes the return value for the button `4` (for both regular output and with
  `--string-output`).
* `‑‑no‑cancel`: Don't show a cancel button.
* `‑‑float`: Float on top of all windows.
* `‑‑timeout` _numSeconds_: The amount of time, in seconds, that the window will
  be displayed if the user does not click a button. Does not time out by
  default.

Return:

* `1` for ok, `2` for cancel, depending on which button was pressed; or `gtk-ok`
  or `gtk-cancel` if the `--string-output` option is given.
* Returns `4` (for both regular and `--string-output`) if the user didn't press
  a button due to `--exit-onchange`.
* On the following line will be the index (zero-based) of the selected item, or
  its label if the `--string-output` option is given.
* On a timeout, it will return `0` or `timeout`, and the selected item
  _will not_ be returned.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`, and the selected item _will not_ be returned.

- - -

## Filteredlists

- - -

<a id="filteredlist"/>
### `filteredlist`

This dialog provides a list of items to filter through. Text entered into the
input box is matched case-insensitively to the first column of list items.
Non-matches are not displayed. Spaces (`' '`) are treated as globs (`'*'`), so
"f b" is represented as "f\*b" and will match "foobar", "fobar", "fbar", etc.

Options: (in addition to [global options](#Global.Options))

* `--text` "_main text message_": The initial text in the input box.
* `--informative-text` "_extra informative text to be displayed_": The text for
  the label above the input box.
* `--columns` _list of columns_: **Required**. These are the names of the
  columns in the list. `list of columns` should be space separated and given as
  multiple arguments (ie: don't double quote the entire list. Provide it as you
  would multiple arguments for any shell program).

  Example: `gtdialog filteredlist --columns Foo Bar --items foo bar`
* `--items` _list of items_: **Required after `--columns`**. These are the items
  in the list each inserted into the first empty column in the first non-full
  row. `list of items` should be space separated and given as multiple arguments
  (ie: don't double quote the entire list. Provide it as you would multiple
  arguments for any shell program).
* `--search-column` _col_: Required to be after `--columns` if specified. The
  column to use for searching. Default is `0`.
* `--output-column` _col_: The column to use for `--string-output`. Default is
  `0`.
* `--button1` "_label for button 1_": **Required**. The right-most button.
* `--button2` "_label for button 2_": The middle button.
* `--button3` "_label for button 3_": The left-most button. It will not be
  displayed if there is no `--button2` label specified.
* `--select-multiple`: Allows the user to select more than one item. Default is
  to allow only one selection.

Return:

* `1`, `2`, or `3` depending on which button was pressed; or the label of the
  button if the `--string-output` option is given.
* On the following line will be the index (zero-based) of the selected item, or
  its label if the `--string-output` option is given, subject to
  `--output-column`.
* On a timeout, it will return `0` or `timeout`, and the selected item
  _will not_ be returned.
* If the user presses `Esc` or closes the dialog window, it will return `-1` or
  `delete`, and the selected item _will not_ be returned.

Usage:

    gtdialog filteredlist \
      --columns Foo Bar \
      --items foobar barfoo foobaz bazfoo barbaz bazbar \
      --select-multiple

- - -
