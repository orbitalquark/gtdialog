/**
 * A CocoaDialog clone written in C using GTK.
 *
 * The MIT License
 *
 * Copyright (c) 2009 Mitchell mitchell<att>caladbolg.net
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "gcocoadialog.h"

GCDialogType gcocoadialog_type(const char *type) {
  if (strcmp(type, "msgbox") == 0)
    return GCDIALOG_MSGBOX;
  else if (strcmp(type, "ok-msgbox") == 0)
    return GCDIALOG_OK_MSGBOX;
  else if (strcmp(type, "yesno-msgbox") == 0)
    return GCDIALOG_YESNO_MSGBOX;
  else if (strcmp(type, "inputbox") == 0)
    return GCDIALOG_INPUTBOX;
  else if (strcmp(type, "standard-inputbox") == 0)
    return GCDIALOG_STANDARD_INPUTBOX;
  else if (strcmp(type, "secure-inputbox") == 0)
    return GCDIALOG_SECURE_INPUTBOX;
  else if (strcmp(type, "secure-standard-inputbox") == 0)
    return GCDIALOG_SECURE_STANDARD_INPUTBOX;
  else if (strcmp(type, "fileselect") == 0)
    return GCDIALOG_FILESELECT;
  else if (strcmp(type, "filesave") == 0)
    return GCDIALOG_FILESAVE;
  else if (strcmp(type, "textbox") == 0)
    return GCDIALOG_TEXTBOX;
  else if (strcmp(type, "progressbar") == 0)
    return GCDIALOG_PROGRESSBAR;
  else if (strcmp(type, "dropdown") == 0)
    return GCDIALOG_DROPDOWN;
  else if (strcmp(type, "standard-dropdown") == 0)
    return GCDIALOG_STANDARD_DROPDOWN;
  return -1;
}

static void close_dropdown(GtkWidget *dropdown, gpointer userdata) {
  g_signal_emit_by_name((GtkWidget *)userdata, "response", 4);
}

static gboolean timeout_dialog(gpointer userdata) {
  g_signal_emit_by_name((GtkWidget *)userdata, "response", 0);
  return FALSE;
}

char *gcocoadialog(GCDialogType type, int narg, const char *args[]) {
  // Default variables.
  GtkWidget *dialog = NULL, *text = NULL, *info_text = NULL;
  int string_output = 0, no_newline = 0;
  int width = -1, height = -1, timeout = 0;
  int RESPONSE_DELETE = -1; // timeout is 0
  char *out = NULL;

  // Dialog-specific variables.
  GtkWidget *entry = NULL, *textview = NULL, *progressbar = NULL,
            *combobox = NULL;
  GtkTextBuffer *buffer = NULL;
  int focus_textbox = 0, selected = 0, indeterminate = 0;
  const char *buttons[3] = { NULL, NULL, NULL }, *scroll_to = "top";

  // Setup dialog.
  if (type != GCDIALOG_FILESELECT && type != GCDIALOG_FILESAVE) {
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), "gcocoadialog");
    text = gtk_label_new("");
    info_text = gtk_label_new("");
    GtkWidget *vbox = GTK_DIALOG(dialog)->vbox;
    gtk_box_pack_start(GTK_BOX(vbox), text, FALSE, TRUE, 5);
    gtk_box_pack_start(GTK_BOX(vbox), info_text, FALSE, TRUE, 5);
    if (type == GCDIALOG_MSGBOX) {
      buttons[0] = "gtk-ok";
    } else if (type == GCDIALOG_OK_MSGBOX) {
      buttons[0] = "gtk-ok";
      buttons[1] = "gtk-cancel";
    } else if (type == GCDIALOG_YESNO_MSGBOX) {
      buttons[0] = "gtk-yes";
      buttons[1] = "gtk-no";
      buttons[2] = "gtk-cancel";
    } else if (type >= GCDIALOG_INPUTBOX &&
               type <= GCDIALOG_SECURE_STANDARD_INPUTBOX) {
      entry = gtk_entry_new();
      gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
      gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 5);
      buttons[0] = "gtk-ok";
      if (type >= GCDIALOG_SECURE_INPUTBOX)
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    } else if (type == GCDIALOG_TEXTBOX) {
      width = 400;
      height = 300;
      textview = gtk_text_view_new();
      buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
      gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
      GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
      gtk_container_add(GTK_CONTAINER(scrolled), textview);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_POLICY_AUTOMATIC,
                                     GTK_POLICY_AUTOMATIC);
      gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);
      buttons[0] = "gtk-ok";
    } else if (type == GCDIALOG_PROGRESSBAR) {
      progressbar = gtk_progress_bar_new();
      gtk_box_pack_start(GTK_BOX(vbox), progressbar, FALSE, TRUE, 5);
    } else if (type == GCDIALOG_DROPDOWN) {
      combobox = gtk_combo_box_new_text();
      gtk_box_pack_start(GTK_BOX(vbox), combobox, FALSE, TRUE, 5);
      buttons[0] = "gtk-ok";
    }
    if (width > 0 && height > 0)
       gtk_widget_set_size_request(GTK_WIDGET(dialog), width, height);
  } else if (type == GCDIALOG_FILESELECT) {
    dialog = gtk_file_chooser_dialog_new("", NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                         NULL);
  } else if (type == GCDIALOG_FILESAVE) {
    dialog = gtk_file_chooser_dialog_new("", NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
                                         GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                         NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                   TRUE);
  }
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "gcocoadialog", "gcocoadialog");

  // Parse arguments.
  int i = 0;
  const char *arg = args[i++];
  while (arg && i <= narg) {
    int get_next_arg = 1;
    // Default options.
    if (strcmp(arg, "--debug") == 0) {
      // not implemented
    } else if (strcmp(arg, "--height") == 0) {
      int h = atoi(args[i++]);
      if (h > 0) gtk_widget_set_size_request(GTK_WIDGET(dialog), width, h);
    } else if (strcmp(arg, "--help") == 0) {
      // not implemented
    } else if (strcmp(arg, "--no-newline") == 0) {
      no_newline = 1;
    } else if (strcmp(arg, "--string-output") == 0) {
      string_output = 1;
    } else if (strcmp(arg, "--title") == 0) {
      gtk_window_set_title(GTK_WINDOW(dialog), args[i++]);
    } else if (strcmp(arg, "--width") == 0) {
      int w = atoi(args[i++]);
      if (w > 0) gtk_widget_set_size_request(GTK_WIDGET(dialog), w, height);
    // Dialog-specific options
    } else if (strcmp(arg, "--button1") == 0) {
      if (type == GCDIALOG_MSGBOX || type == GCDIALOG_INPUTBOX ||
          type == GCDIALOG_TEXTBOX || type == GCDIALOG_DROPDOWN)
        buttons[0] = args[i++];
    } else if (strcmp(arg, "--button2") == 0) {
      if (type == GCDIALOG_MSGBOX || type == GCDIALOG_INPUTBOX ||
          type == GCDIALOG_TEXTBOX || type == GCDIALOG_DROPDOWN)
        buttons[1] = args[i++];
    } else if (strcmp(arg, "--button3") == 0) {
      if (type == GCDIALOG_MSGBOX || type == GCDIALOG_INPUTBOX ||
          type == GCDIALOG_TEXTBOX || type == GCDIALOG_DROPDOWN)
        buttons[2] = args[i++];
    } else if (strcmp(arg, "--editable") == 0) {
      if (type == GCDIALOG_TEXTBOX)
        gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), TRUE);
    } else if (strcmp(arg, "--exit-onchange") == 0) {
      if (type == GCDIALOG_DROPDOWN || type == GCDIALOG_STANDARD_DROPDOWN)
        g_signal_connect(G_OBJECT(dialog), "changed",
                         G_CALLBACK(close_dropdown), dialog);
    } else if (strcmp(arg, "--float") == 0) {
      if (type != GCDIALOG_FILESELECT && type != GCDIALOG_FILESAVE &&
          type != GCDIALOG_PROGRESSBAR)
        gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    } else if (strcmp(arg, "--focus-textbox") == 0) {
      if (type == GCDIALOG_TEXTBOX)
        focus_textbox = 1;
     } else if (strcmp(arg, "--icon") == 0) {
      //if (type >= GCDIALOG_MSGBOX && type <= GCDIALOG_YESNO_MSGBOX)
        // not implemented
    } else if (strcmp(arg, "--icon-file") == 0) {
      //if (type >= GCDIALOG_MSGBOX && type <= GCDIALOG_YESNO_MSGBOX)
        // not implemented
    } else if (strcmp(arg, "--indeterminate") == 0) {
      if (type == GCDIALOG_PROGRESSBAR)
        indeterminate = 1;
    } else if (strcmp(arg, "--informative-text") == 0) {
      if (type < GCDIALOG_FILESELECT || type == GCDIALOG_TEXTBOX)
        gtk_label_set_markup(GTK_LABEL(info_text), args[i++]);
    } else if (strcmp(arg, "--items") == 0) {
      if (type == GCDIALOG_DROPDOWN || type == GCDIALOG_STANDARD_DROPDOWN) {
        const char *item = args[i++];
        while (item && strncmp(item, "--", 2) != 0) {
          gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), item);
          item = args[i++];
        }
        get_next_arg = 0;
        arg = item;
      }
    } else if (strcmp(arg, "--no-cancel") == 0) {
      if (type == GCDIALOG_YESNO_MSGBOX) {
        buttons[2] = NULL;
      } else if (type == GCDIALOG_OK_MSGBOX ||
                 type == GCDIALOG_STANDARD_INPUTBOX ||
                 type == GCDIALOG_SECURE_STANDARD_INPUTBOX ||
                 type == GCDIALOG_STANDARD_DROPDOWN) {
        buttons[1] = NULL;
      }
    } else if (strcmp(arg, "--no-create-directories") == 0) {
      //if (type == GCDIALOG_FILESAVE)
        // not implemented
   } else if (strcmp(arg, "--no-show") == 0) {
      if (type >= GCDIALOG_INPUTBOX &&
          type <= GCDIALOG_SECURE_STANDARD_INPUTBOX)
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    } else if (strcmp(arg, "--packages-as-directories") == 0) {
      //if (type == GCDIALOG_FILESELECT || type == GCDIALOG_FILESAVE)
        // not implemented
    } else if (strcmp(arg, "--percent") == 0) {
      if (type == GCDIALOG_PROGRESSBAR)
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar),
                                      0.01 * atoi(args[i++]));
    } else if (strcmp(arg, "--pulldown") == 0) {
      //if (type == GCDIALOG_DROPDOWN || type == GCDIALOG_STANDARD_DROPDOWN)
        // not implemented
    } else if (strcmp(arg, "--scroll-to") == 0) {
      if (type == GCDIALOG_TEXTBOX) {
        focus_textbox = 1;
        scroll_to = args[i++];
      }
    } else if (strcmp(arg, "--select-directories") == 0) {
      //if (type == GCDIALOG_FILESELECT)
        // not implemented
    } else if (strcmp(arg, "--select-multiple") == 0) {
      if (type == GCDIALOG_FILESELECT)
        gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
    } else if (strcmp(arg, "--select-only-directories") == 0) {
      if (type == GCDIALOG_FILESELECT)
        gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog),
                                    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    } else if (strcmp(arg, "--selected") == 0) {
      if (type == GCDIALOG_TEXTBOX)
        selected = 1;
    } else if (strcmp(arg, "--text") == 0) {
      if (type != GCDIALOG_FILESELECT && type != GCDIALOG_FILESAVE) {
        if (type >= GCDIALOG_INPUTBOX &&
            type <= GCDIALOG_SECURE_STANDARD_INPUTBOX) {
          gtk_entry_set_text(GTK_ENTRY(entry), args[i++]);
        } else if (type == GCDIALOG_TEXTBOX) {
          const char *txt = args[i++];
          gtk_text_buffer_set_text(buffer, txt, strlen(txt));
        } else if (type == GCDIALOG_PROGRESSBAR) {
          gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar), args[i++]);
        } else {
          gtk_label_set_markup(GTK_LABEL(text), args[i++]);
        }
      }
    } else if (strcmp(arg, "--text-from-file") == 0) {
      if (type == GCDIALOG_TEXTBOX) {
        FILE *f = fopen(args[i++], "r");
        if (f) {
          fseek(f, 0, SEEK_END);
          int len = ftell(f);
          char *buf = malloc(len);
          rewind(f);
          fread(buf, 1, len, f);
          gtk_text_buffer_set_text(buffer, buf, len);
          free(buf);
          fclose(f);
        }
      }
    } else if (strcmp(arg, "--timeout") == 0) {
      if (type != GCDIALOG_FILESELECT && type != GCDIALOG_FILESAVE &&
          type != GCDIALOG_PROGRESSBAR)
        timeout = atoi(args[i++]);
    } else if (strcmp(arg, "--with-directory") == 0) {
      if (type == GCDIALOG_FILESELECT || type == GCDIALOG_FILESAVE)
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            args[i++]);
    } else if (strcmp(arg, "--with-extension") == 0) {
      if (type == GCDIALOG_FILESELECT || type == GCDIALOG_FILESAVE) {
        GtkFileFilter *filter = gtk_file_filter_new();
        const char *ext = args[i++];
        while (ext && strncmp(ext, "--", 2) != 0) {
          char *glob = g_strconcat((*ext == '.') ? "*" : "*.", ext, NULL);
          gtk_file_filter_add_pattern(filter, glob);
          g_free(glob);
          ext = args[i++];
        }
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
        get_next_arg = 0;
        arg = ext;
      }
    } else if (strcmp(arg, "--with-file") == 0) {
      if (type == GCDIALOG_FILESELECT || type == GCDIALOG_FILESAVE)
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), args[i++]);
    }
    if (get_next_arg) arg = args[i++];
  }

  // Run dialog; text output is stored in 'out'
  if (type != GCDIALOG_FILESELECT && type != GCDIALOG_FILESAVE &&
      type != GCDIALOG_PROGRESSBAR) {
    if (type == GCDIALOG_TEXTBOX) {
      if (!focus_textbox &&
          !gtk_text_view_get_editable(GTK_TEXT_VIEW(textview)))
        g_object_set(G_OBJECT(textview), "can-focus", FALSE, NULL);
      if (strcmp(scroll_to, "top") == 0)
        g_signal_emit_by_name(G_OBJECT(textview), "move-cursor",
                              GTK_MOVEMENT_BUFFER_ENDS, -1, 0);
      else
        g_signal_emit_by_name(G_OBJECT(textview), "move-cursor",
                              GTK_MOVEMENT_BUFFER_ENDS, 1, 0);
      if (selected)
        g_signal_emit_by_name(G_OBJECT(textview), "select-all", TRUE);
    } else if (type >= GCDIALOG_DROPDOWN) {
      gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
    }
    for (i = 3; i > 0; i--)
      if (buttons[i - 1])
        gtk_dialog_add_button(GTK_DIALOG(dialog), buttons[i - 1], i);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), 1);
    gtk_widget_show_all(dialog);
    if (strlen(gtk_label_get_text(GTK_LABEL(text))) == 0) gtk_widget_hide(text);
    if (strlen(gtk_label_get_text(GTK_LABEL(info_text))) == 0)
      gtk_widget_hide(info_text);
    if (timeout) g_timeout_add_seconds(timeout, timeout_dialog, dialog);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == -4) response = RESPONSE_DELETE;
    if (string_output && response > 0 && response <= 3) {
      int len = strlen(buttons[response - 1]);
      out = malloc(len + 1);
      strncpy(out, buttons[response - 1], len);
      out[len] = '\0';
    } else {
      out = malloc(3);
      sprintf(out, "%i", response);
    }
    if (type >= GCDIALOG_INPUTBOX && type != GCDIALOG_FILESELECT &&
        type != GCDIALOG_FILESAVE && type != GCDIALOG_PROGRESSBAR) {
      if (response > 0) { // no delete or timeout
        char *txt;
        int created = 0;
        if (type <= GCDIALOG_SECURE_STANDARD_INPUTBOX) {
          txt = (char *)gtk_entry_get_text(GTK_ENTRY(entry));
        } else if (type == GCDIALOG_TEXTBOX) {
          GtkTextIter s, e;
          gtk_text_buffer_get_start_iter(buffer, &s);
          gtk_text_buffer_get_end_iter(buffer, &e);
          txt = gtk_text_buffer_get_text(buffer, &s, &e, TRUE);
        } else { // type >= GCDIALOG_DROPDOWN
          if (string_output) {
            txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox));
          } else {
            txt = malloc(4);
            sprintf(txt, "%i", gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
            created = 1;
          }
        }
        char *new_out = malloc(strlen(out) + strlen(txt) + 2);
        sprintf(new_out, "%s\n%s", out, txt);
        free(out);
        out = new_out;
        if (created) free(txt);
      }
    }
  } else if (type == GCDIALOG_FILESELECT || type ==  GCDIALOG_FILESAVE) {
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
      if (type == GCDIALOG_FILESELECT &&
          gtk_file_chooser_get_select_multiple(GTK_FILE_CHOOSER(dialog))) {
        out = malloc(1);
        *out = '\0';
        GSList *filenames = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
        GSList *item = filenames;
        while (item) {
          char *data = (char *)item->data;
          char *new_out = malloc(strlen(out) + strlen(data) + 2);
          sprintf(new_out, "%s\n%s", out, data);
          free(out);
          g_free(item->data);
          item = item->next;
          out = new_out;
        }
        g_slist_free(filenames);
      } else {
        char *text = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        int len = strlen(text);
        out = malloc(len + 1);
        strncpy(out, text, len);
        out[len] = '\0';
      }
    } else {
      out = malloc(1);
      *out = '\0';
    }
  } else if (type == GCDIALOG_PROGRESSBAR) {
    gtk_widget_show_all(GTK_WIDGET(dialog));
    // TODO: read from stdin and update progressbar
  }
  if (strcmp(out, "0") == 0 && string_output) {
    out = malloc(8);
    sprintf(out, "timeout");
  } else if (strcmp(out, "-1") == 0 && string_output) {
    out = malloc(7);
    sprintf(out, "delete");
  }
  gtk_widget_destroy(dialog);
  if (!no_newline) {
    char *new_out = malloc(strlen(out) + 2);
    sprintf(new_out, "%s\n", out);
    free(out);
    out = new_out;
  }
  return out;
}

int error() {
  printf(
    "Usage: CocoaDialog type [options]\n"
    "  Available Types:\n"
    "    msgbox, ok-msgbox, yesno-msgbox, inputbox, standard-inputbox,\n"
    "    secure-inputbox, secure-standard-inputbox, fileselect, filesave,\n"
    "    textbox, progressbar, dropdown, standard-dropdown\n"
    "  Global Options:\n"
    "    --help, --debug, --title, --width, --height,\n"
    "    --string-output, --no-newline\n"
    "\n"
    "See http://cocoadialog.sourceforge.net/documentation.html for detailed\n"
    "documentation.\n"
    "\n"
    "Note that bubble mode is not supported, as well as the following options:\n"
    "\"--icon\", \"--icon-file\", \"--select-directories\", \"--packages-as-directories\",\n"
    "\"--no-create-directories\", and \"--pulldown\". The \"--debug\" flag does nothing\n"
    "either.\n"
    "\n"
    "Also, the \"OK\", \"Yes\", \"No\", \"Cancel\" buttons return \"gtk-ok\", \"gtk-yes\",\n"
    "\"gtk-no\", \"gtk-cancel\" respectively as text for --string-output. If your\n"
    "button[n] text is a GTK stock item, (e.g. \"gtk-copy\") then that exact text\n"
    "is returned, not the text displayed on the button.\n"
    "\n"
    "When the dialog receives a 'delete' event, (e.g. when the user presses the\n"
    "Escape key) -1 or \"delete\" will be returned depending on --string-output.\n"
  );
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc == 1) return error();
  int type = gcocoadialog_type(argv[1]);
  gtk_init(&argc, &argv);
  if (type < 0) return error();
  char *out = gcocoadialog(type, argc - 1, &argv[2]);
  printf(out);
  free(out);
  return 0;
}
