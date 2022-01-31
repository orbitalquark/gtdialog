/**
 * A CocoaDialog clone written in C using GTK or curses.
 *
 * The MIT License
 *
 * Copyright (c) 2009-2022 Mitchell
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef GTDIALOG_H
#define GTDIALOG_H

#if GTK
#include <gtk/gtk.h>
#endif

enum GTDialogTypes {
  GTDIALOG_UNKNOWN,
  GTDIALOG_MSGBOX,
  GTDIALOG_OK_MSGBOX,
  GTDIALOG_YESNO_MSGBOX,
  GTDIALOG_INPUTBOX,
  GTDIALOG_STANDARD_INPUTBOX,
  GTDIALOG_SECURE_INPUTBOX,
  GTDIALOG_SECURE_STANDARD_INPUTBOX,
  GTDIALOG_FILESELECT,
  GTDIALOG_FILESAVE,
  GTDIALOG_TEXTBOX,
  GTDIALOG_PROGRESSBAR,
  GTDIALOG_DROPDOWN,
  GTDIALOG_STANDARD_DROPDOWN,
  GTDIALOG_FILTEREDLIST,
  GTDIALOG_OPTIONSELECT,
  GTDIALOG_COLORSELECT,
  GTDIALOG_FONTSELECT
};

typedef enum GTDialogTypes GTDialogType;

#if GTK
/**
 * Sets the parent window for gtdialogs.
 * @param window The parent window.
 */
void gtdialog_set_parent(GtkWindow *parent);
#endif

/**
 * Sets the callback function used for progressbar dialogs.
 * @param callback Function to call to do some work. It must return either a newly allocated
 *   string of the form "num str\n", where num is integer progress between 0 and 100 and str is
 *   optional progress display text, or it must return `NULL`, signaling work is complete. The
 *   returned string will be freed by the caller.
 * @param data Optional data passed to the callback function.
 */
void gtdialog_set_progressbar_callback(char *(*f)(void *), void *data);

/**
 * Returns the GTDialogType for the given type string.
 * @param type The string dialog type. Acceptable types are "msgbox", "ok-msgbox", "yesno-msgbox",
 *   "inputbox", "standard-inputbox", "secure-inputbox", "secure-standard-inputbox", "fileselect",
 *   "filesave", "textbox", "progressbar", "dropdown", "standard-dropdown", "filteredlist",
 *   "optionselect", "colorselect", and "fontselect".
 * @return GTDialogType or GTDIALOG_UNKNOWN
 */
GTDialogType gtdialog_type(const char *type);

/**
 * Creates, displays, and returns the result from a gtdialog of the given type from the given
 * set of parameters.
 * The string returned must be freed when finished.
 * @param type The GTDialogType type.
 * @param narg The number of parameters in *args*.
 * @param args The set of parameters for the dialog.
 * @return string result
 */
char *gtdialog(GTDialogType type, int narg, const char *args[]);

#endif
