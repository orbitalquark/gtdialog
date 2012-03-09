/**
 * A CocoaDialog clone written in C using GTK.
 *
 * The MIT License
 *
 * Copyright (c) 2009-2012 Mitchell mitchell.att.foicica.com
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

#ifndef GCOCOADIALOG_H
#define GCOCOADIALOG_H

enum GCDialogTypes {
  GCDIALOG_MSGBOX,
  GCDIALOG_OK_MSGBOX,
  GCDIALOG_YESNO_MSGBOX,
  GCDIALOG_INPUTBOX,
  GCDIALOG_STANDARD_INPUTBOX,
  GCDIALOG_SECURE_INPUTBOX,
  GCDIALOG_SECURE_STANDARD_INPUTBOX,
  GCDIALOG_FILESELECT,
  GCDIALOG_FILESAVE,
  GCDIALOG_TEXTBOX,
  GCDIALOG_PROGRESSBAR,
  GCDIALOG_DROPDOWN,
  GCDIALOG_STANDARD_DROPDOWN,
  GCDIALOG_FILTEREDLIST,
};

typedef enum GCDialogTypes GCDialogType;

GCDialogType gcocoadialog_type(const char *type);
char *gcocoadialog(GCDialogType type, int narg, const char *args[]);

#endif
