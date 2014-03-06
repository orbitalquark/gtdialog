/**
 * A CocoaDialog clone written in C using GTK or curses.
 *
 * The MIT License
 *
 * Copyright (c) 2009-2014 Mitchell mitchell.att.foicica.com
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
};

typedef enum GTDialogTypes GTDialogType;

#if GTK
void gtdialog_set_parent(GtkWindow *parent);
#endif
GTDialogType gtdialog_type(const char *type);
char *gtdialog(GTDialogType type, int narg, const char *args[]);

#endif
