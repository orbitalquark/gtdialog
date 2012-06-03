/**
 * A CocoaDialog clone written in C using GTK or ncurses.
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if GTK
#include <gtk/gtk.h>
#elif NCURSES
#include <cdk.h>
#endif

#include "gtdialog.h"

// Default variables.
int string_output = 0;
int output_col = 1, search_col = 1;

#if GTK
#define STR_OK "gtk-ok"
#define STR_CANCEL "gtk-cancel"
#define STR_YES "gtk-yes"
#define STR_NO "gtk-no"
#elif NCURSES
#define STR_OK "Ok"
#define STR_CANCEL "Cancel"
#define STR_YES "Yes"
#define STR_NO "No"
#endif

#if GTK
#if GTK_CHECK_VERSION(3,0,0)
#define gtk_combo_box_new_text gtk_combo_box_text_new
#define gtk_combo_box_append_text gtk_combo_box_text_append_text
#define gtk_combo_box_get_active_text gtk_combo_box_text_get_active_text
#endif
#elif NCURSES
#define copy(s) strcpy(malloc(strlen(s) + 1), s)
#endif

GTDialogType gtdialog_type(const char *type) {
  if (strcmp(type, "msgbox") == 0)
    return GTDIALOG_MSGBOX;
  else if (strcmp(type, "ok-msgbox") == 0)
    return GTDIALOG_OK_MSGBOX;
  else if (strcmp(type, "yesno-msgbox") == 0)
    return GTDIALOG_YESNO_MSGBOX;
  else if (strcmp(type, "inputbox") == 0)
    return GTDIALOG_INPUTBOX;
  else if (strcmp(type, "standard-inputbox") == 0)
    return GTDIALOG_STANDARD_INPUTBOX;
  else if (strcmp(type, "secure-inputbox") == 0)
    return GTDIALOG_SECURE_INPUTBOX;
  else if (strcmp(type, "secure-standard-inputbox") == 0)
    return GTDIALOG_SECURE_STANDARD_INPUTBOX;
  else if (strcmp(type, "fileselect") == 0)
    return GTDIALOG_FILESELECT;
  else if (strcmp(type, "filesave") == 0)
    return GTDIALOG_FILESAVE;
  else if (strcmp(type, "textbox") == 0)
    return GTDIALOG_TEXTBOX;
  else if (strcmp(type, "progressbar") == 0)
    return GTDIALOG_PROGRESSBAR;
  else if (strcmp(type, "dropdown") == 0)
    return GTDIALOG_DROPDOWN;
  else if (strcmp(type, "standard-dropdown") == 0)
    return GTDIALOG_STANDARD_DROPDOWN;
  else if (strcmp(type, "filteredlist") == 0)
    return GTDIALOG_FILTEREDLIST;
  return -1;
}

// Callbacks and utility functions.
#if GTK
static void close_dropdown(GtkWidget *dropdown, gpointer userdata) {
  g_signal_emit_by_name(userdata, "response", 4);
}

static gboolean entry_keypress(GtkWidget *entry, GdkEventKey *event,
                               gpointer userdata) {
  GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(userdata));
  gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(model));
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_first(model, &iter))
    gtk_tree_selection_select_iter(
      gtk_tree_view_get_selection(GTK_TREE_VIEW(userdata)), &iter);
  return FALSE;
}

static void list_foreach(GtkTreeModel *model, GtkTreePath *path,
                         GtkTreeIter *iter, gpointer userdata) {
  char *value;
  if (string_output) {
    gtk_tree_model_get(model, iter, output_col - 1, &value, -1);
  } else {
    path =
      gtk_tree_model_filter_convert_path_to_child_path(
        GTK_TREE_MODEL_FILTER(model), path);
    value = gtk_tree_path_to_string(path);
    gtk_tree_path_free(path);
  }
  g_string_append_printf((GString *)userdata, "%s\n", value);
  free(value);
}

static gboolean list_select(GtkTreeView *treeview, gboolean arg1,
                            gpointer userdata) {
  g_signal_emit_by_name(userdata, "response", 1);
  return FALSE;
}

static gboolean list_visible(GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer userdata) {
  const char *entry_text = gtk_entry_get_text(GTK_ENTRY(userdata));
  if (strlen(entry_text) > 0) {
    char *text = g_utf8_strdown(entry_text, -1);
    char *value, *lower, *p;
    gtk_tree_model_get(model, iter, search_col - 1, &value, -1);
    lower = g_utf8_strdown(value, -1);
    p = lower;
    char **tokens = g_strsplit(text, " ", 0);
    gboolean visible = TRUE;
    int i = 0;
    while (tokens[i] != NULL) {
      if (!(p = strstr(p, tokens[i]))) {
        visible = FALSE;
        break;
      }
      p += strlen(tokens[i++]);
    }
    free(text);
    free(value);
    free(lower);
    g_strfreev(tokens);
    return visible;
  }
  return TRUE;
}

static gboolean timeout_dialog(gpointer userdata) {
  g_signal_emit_by_name(userdata, "response", 0);
  return FALSE;
}
#elif NCURSES
static int wrap(char *str, int w, char ***plines) {
  int i;
  for (i = w; i < strlen(str); i += w) {
    int j = i;
    while (j >= 0 && !isspace(str[j]))
      j--;
    if (j + w > i) {
      i = j; // line length is less than w; cut here
      str[i++] = '\n';
    }
  }
  int nlines = 1;
  char *p = str - 1;
  while ((p = strstr(p + 1, "\n")))
    nlines++;
  char **lines = malloc(nlines * sizeof(char *));
  lines[0] = str;
  for (i = 1; i < nlines; i++) {
    p = strstr(lines[i - 1], "\n");
    *p = '\0';
    lines[i] = p + 1;
  }
  *plines = lines;
  return nlines;
}

static int buttonbox_tab(EObjectType cdkType, void *object, void *data,
                         chtype key) {
  injectCDKButtonbox((CDKBUTTONBOX *)data, key);
  return TRUE;
}

typedef struct {
  int ncols;
  int search_col;
  char **items;
  int len;
  char **rows;
  int num_rows;
  char **filtered_rows;
  CDKSCROLL *scrolled;
} Model;

static char *strdown(char *s) {
  char *lower = copy(s);
  int i, len = strlen(s);
  for (i = 0; i < len; i++)
    lower[i] = tolower(s[i]);
  return lower;
}

static int entry_keypress(EObjectType cdkType, void *object, void *data,
                           chtype key) {
  Model *model = (Model *)data;
  char *entry_text = getCDKEntryValue((CDKENTRY *)object);
  if (strlen(entry_text) > 0) {
    char *text = strdown(entry_text);
    char **tokens = CDKsplitString(text, ' ');
    int i, j, row = 0;
    for (i = 0; i < model->len; i += model->ncols) {
      char *lower = strdown(model->items[i + model->search_col - 1]);
      char *p = lower;
      int visible = 1;
      j = 0;
      while (tokens[j] != NULL) {
        if (!(p = strstr(p, tokens[j]))) {
          visible = 0;
          break;
        }
        p += strlen(tokens[j++]);
      }
      if (visible)
        model->filtered_rows[row++] = model->rows[i / model->ncols];
      free(lower);
    }
    free(text);
    CDKfreeStrings(tokens);
    setCDKScrollItems(model->scrolled, model->filtered_rows, row, FALSE);
  } else {
    setCDKScrollItems(model->scrolled, model->rows, model->len / model->ncols,
                      FALSE);
  }
  HasFocusObj(ObjOf(model->scrolled)) = TRUE; // needed to draw highlight
  eraseCDKScroll(model->scrolled); // drawCDKScroll does not completely redraw
  drawCDKScroll(model->scrolled, TRUE);
  HasFocusObj(ObjOf(model->scrolled)) = FALSE;
  return TRUE;
}

static char **item_rows(char **cols, int ncols, char **items, int len) {
  int *col_widths = malloc(sizeof(int) * ncols);
  int i, j, row_len = 0;
  for (i = 0; i < ncols; i++) {
    int max = strlen(cols[i]);
    for (j = i; j < len; j += ncols) {
      int slen = strlen(items[j]);
      if (slen > max)
        max = slen;
    }
    col_widths[i] = max;
    row_len += col_widths[i] + 1;
  }
  char **new_items = malloc(sizeof(char *) * ((len + ncols - 1) / ncols + 1));
  for (i = -ncols; i < len; i += ncols) {
    char *new_item = malloc((i < 0) ? row_len + 4 : row_len);
    char *p = (i < 0) ? stpcpy(new_item, "</U>") : new_item;
    for (j = i; j < i + ncols && j < len; j++) {
      char *item = (i < 0) ? cols[j - i] : items[j];
      p = stpcpy(p, item);
      int padding = col_widths[j - i] - strlen(item);
      while (padding-- > 0)
        *p++ = ' ';
      *p++ = (i < 0) ? '|' : ' ';
    }
    if (p > new_item)
      *(p - 1) = '\0';
    new_items[i / ncols + 1] = new_item;
  }
  free(col_widths);
  return &new_items[1]; // items start here
}

static int scrolled_key(EObjectType cdkType, void *object, void *data,
                        chtype key) {
  HasFocusObj(ObjOf((CDKSCROLL *)data)) = TRUE; // needed to draw highlight
  injectCDKScroll((CDKSCROLL *)data, key);
  HasFocusObj(ObjOf((CDKSCROLL *)data)) = FALSE;
  return TRUE;
}
#endif

char *gtdialog(GTDialogType type, int narg, const char *args[]) {
  int RESPONSE_DELETE = -1; // timeout is 0
  char *out = NULL;

  // Default variables.
  int height = -1, floating = 0, no_newline = 0, timeout = 0, width = -1;
  const char *title = "gtdialog";
  string_output = 0;

  // Dialog-specific variables.
  int editable = 0, exit_onchange = 0, focus_textbox = 0, indeterminate = 0,
      no_show = 0, percent = 0, multiple = 0, only_dirs = 0, selected = 0;
  const char *buttons[3] = { NULL, NULL, NULL}, **cols = NULL,
             *info_text = NULL, **items = NULL, *scroll_to = "top",
             *text = NULL, *dir = NULL, *file = NULL;
  int ncols = 0, len = 0;
#if GTK
  PangoFontDescription *font = NULL;
  GtkFileFilter *filter = NULL;
#endif
  output_col = 1;
  search_col = 1;

  // Set dialog defaults.
#if NCURSES
  width = 40;
  height = 10;
#endif
  if (type == GTDIALOG_MSGBOX) {
    buttons[0] = STR_OK;
  } else if (type == GTDIALOG_OK_MSGBOX) {
    buttons[0] = STR_OK;
    buttons[1] = STR_CANCEL;
  } else if (type == GTDIALOG_YESNO_MSGBOX) {
    buttons[0] = STR_YES;
    buttons[1] = STR_NO;
    buttons[2] = STR_CANCEL;
  } else if (type >= GTDIALOG_INPUTBOX &&
             type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
    buttons[0] = STR_OK;
    if (type == GTDIALOG_STANDARD_INPUTBOX ||
        type == GTDIALOG_SECURE_STANDARD_INPUTBOX)
      buttons[1] = STR_CANCEL;
  } else if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE) {
#if NCURSES
    height = 20;
#endif
  } else if (type == GTDIALOG_TEXTBOX) {
#if GTK
    width = 400;
    height = 300;
#elif NCURSES
    height = 20;
#endif
    buttons[0] = STR_OK;
  } else if (type == GTDIALOG_DROPDOWN || type == GTDIALOG_STANDARD_DROPDOWN) {
    buttons[0] = STR_OK;
    if (type == GTDIALOG_STANDARD_DROPDOWN)
      buttons[1] = STR_CANCEL;
  } else if (type == GTDIALOG_FILTEREDLIST) {
#if GTK
    width = 500;
    height = 360;
#elif NCURSES
    height = 20;
#endif
    buttons[0] = STR_OK;
  }

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
      if (h > 0)
        height = h;
    } else if (strcmp(arg, "--help") == 0) {
      // not implemented
    } else if (strcmp(arg, "--no-newline") == 0) {
      no_newline = 1;
    } else if (strcmp(arg, "--string-output") == 0) {
      string_output = 1;
    } else if (strcmp(arg, "--title") == 0) {
      title = args[i++];
    } else if (strcmp(arg, "--width") == 0) {
      int w = atoi(args[i++]);
      if (w > 0)
        width = w;
    // Dialog-specific options
    } else if (strcmp(arg, "--button1") == 0) {
      if (type == GTDIALOG_MSGBOX || type == GTDIALOG_INPUTBOX ||
          type == GTDIALOG_TEXTBOX || type == GTDIALOG_DROPDOWN ||
          type == GTDIALOG_FILTEREDLIST)
        buttons[0] = args[i++];
    } else if (strcmp(arg, "--button2") == 0) {
      if (type == GTDIALOG_MSGBOX || type == GTDIALOG_INPUTBOX ||
          type == GTDIALOG_TEXTBOX || type == GTDIALOG_DROPDOWN ||
          type == GTDIALOG_FILTEREDLIST)
        buttons[1] = args[i++];
    } else if (strcmp(arg, "--button3") == 0) {
      if (type == GTDIALOG_MSGBOX || type == GTDIALOG_INPUTBOX ||
          type == GTDIALOG_TEXTBOX || type == GTDIALOG_DROPDOWN ||
          type == GTDIALOG_FILTEREDLIST)
        buttons[2] = args[i++];
    } else if (strcmp(arg, "--columns") == 0) {
      if (type == GTDIALOG_FILTEREDLIST) {
        cols = &args[i];
        ncols = 0;
        while (i < narg && strncmp(args[i], "--", 2) != 0)
          ncols++, i++;
      }
    } else if (strcmp(arg, "--editable") == 0) {
      if (type == GTDIALOG_TEXTBOX)
        editable = 1;
    } else if (strcmp(arg, "--exit-onchange") == 0) {
      if (type == GTDIALOG_DROPDOWN || type == GTDIALOG_STANDARD_DROPDOWN)
        exit_onchange = 1;
    } else if (strcmp(arg, "--float") == 0) {
      if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE &&
          type != GTDIALOG_PROGRESSBAR)
        floating = 1;
    } else if (strcmp(arg, "--focus-textbox") == 0) {
      if (type == GTDIALOG_TEXTBOX)
        focus_textbox = 1;
     } else if (strcmp(arg, "--icon") == 0) {
      //if (type >= GTDIALOG_MSGBOX && type <= GTDIALOG_YESNO_MSGBOX)
        // not implemented
    } else if (strcmp(arg, "--icon-file") == 0) {
      //if (type >= GTDIALOG_MSGBOX && type <= GTDIALOG_YESNO_MSGBOX)
        // not implemented
    } else if (strcmp(arg, "--indeterminate") == 0) {
      if (type == GTDIALOG_PROGRESSBAR)
        indeterminate = 1;
    } else if (strcmp(arg, "--informative-text") == 0) {
      if (type < GTDIALOG_FILESELECT || type == GTDIALOG_TEXTBOX ||
          type == GTDIALOG_FILTEREDLIST)
        info_text = args[i++];
    } else if (strcmp(arg, "--items") == 0) {
      items = &args[i];
      len = 0;
      while (i < narg && strncmp(args[i], "--", 2) != 0)
        len++, i++;
    } else if (strcmp(arg, "--monospaced-font") == 0) {
#if GTK
      if (type == GTDIALOG_TEXTBOX) {
        font = pango_font_description_new();
        pango_font_description_set_family(font, "monospace");
      }
#endif
    } else if (strcmp(arg, "--no-cancel") == 0) {
      if (type == GTDIALOG_YESNO_MSGBOX) {
        buttons[2] = NULL;
      } else if (type == GTDIALOG_OK_MSGBOX ||
                 type == GTDIALOG_STANDARD_INPUTBOX ||
                 type == GTDIALOG_SECURE_STANDARD_INPUTBOX ||
                 type == GTDIALOG_STANDARD_DROPDOWN) {
        buttons[1] = NULL;
      }
    } else if (strcmp(arg, "--no-create-directories") == 0) {
      //if (type == GTDIALOG_FILESAVE)
        // not implemented
   } else if (strcmp(arg, "--no-show") == 0) {
      if (type >= GTDIALOG_INPUTBOX &&
          type <= GTDIALOG_SECURE_STANDARD_INPUTBOX)
        no_show = 1;
    } else if (strcmp(arg, "--output-column") == 0) {
      if (type == GTDIALOG_FILTEREDLIST) {
        output_col = atoi(args[i++]);
        if (output_col > ncols)
          output_col = ncols;
        else if (output_col < 1)
          output_col = 1;
      }
    } else if (strcmp(arg, "--packages-as-directories") == 0) {
      //if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE)
        // not implemented
    } else if (strcmp(arg, "--percent") == 0) {
      if (type == GTDIALOG_PROGRESSBAR)
        percent = atoi(args[i++]);
    } else if (strcmp(arg, "--pulldown") == 0) {
      //if (type == GTDIALOG_DROPDOWN || type == GTDIALOG_STANDARD_DROPDOWN)
        // not implemented
    } else if (strcmp(arg, "--scroll-to") == 0) {
      if (type == GTDIALOG_TEXTBOX) {
        focus_textbox = 1;
        scroll_to = args[i++];
      }
    } else if (strcmp(arg, "--search-column") == 0) {
      if (type == GTDIALOG_FILTEREDLIST) {
        search_col = atoi(args[i++]);
        if (search_col > ncols)
          search_col = ncols;
        else if (search_col < 1)
          search_col = 1;
      }
    } else if (strcmp(arg, "--select-directories") == 0) {
      //if (type == GTDIALOG_FILESELECT)
        // not implemented
    } else if (strcmp(arg, "--select-multiple") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILTEREDLIST)
        multiple = 1;
    } else if (strcmp(arg, "--select-only-directories") == 0) {
      if (type == GTDIALOG_FILESELECT)
        only_dirs = 1;
    } else if (strcmp(arg, "--selected") == 0) {
      if (type == GTDIALOG_TEXTBOX)
        selected = 1;
      else if (type == GTDIALOG_DROPDOWN || type == GTDIALOG_STANDARD_DROPDOWN)
        selected = atoi(args[i++]);
    } else if (strcmp(arg, "--text") == 0) {
        text = args[i++];
    } else if (strcmp(arg, "--text-from-file") == 0) {
      if (type == GTDIALOG_TEXTBOX)
        file = args[i++];
    } else if (strcmp(arg, "--timeout") == 0) {
      if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE &&
          type != GTDIALOG_PROGRESSBAR)
        timeout = atoi(args[i++]);
    } else if (strcmp(arg, "--with-directory") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE)
        dir = args[i++];
    } else if (strcmp(arg, "--with-extension") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE) {
#if GTK
        filter = gtk_file_filter_new();
#endif
        const char *ext = args[i++];
        while (ext && strncmp(ext, "--", 2) != 0) {
#if GTK
          char *glob = g_strconcat((*ext == '.') ? "*" : "*.", ext, NULL);
          gtk_file_filter_add_pattern(filter, glob);
          g_free(glob);
#endif
          ext = args[i++];
        }
        get_next_arg = 0;
        arg = ext;
      }
    } else if (strcmp(arg, "--with-file") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE)
        file = args[i++];
    }
    if (get_next_arg) arg = args[i++];
  }

  // Create dialog.
#if GTK
  GtkWidget *dialog, *entry, *textview, *progressbar, *combobox, *treeview;
  GtkListStore *list;
#elif NCURSES
  CDKSCREEN *dialog;
  CDKENTRY *entry;
  CDKMENTRY *textview;
  CDKSLIDER *progressbar;
  CDKITEMLIST *combobox;
  CDKBUTTONBOX *buttonbox;
  CDKSCROLL *scrolled;
  Model model = { ncols, search_col, (char **)items, len, NULL, 0, NULL, NULL };
  CDKFSELECT *fileselect;
#endif
  if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE) {
#if GTK
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    if (floating)
      gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(dialog), width, height);
#elif NCURSES
    // There will be a border drawn later, but account for it now.
    dialog = initCDKScreen(newwin(height - 2, width - 2, 2, 2));
#endif
    // Create buttons.
    if (type != GTDIALOG_PROGRESSBAR) {
#if GTK
      for (i = 3; i > 0; i--)
        if (buttons[i - 1])
          gtk_dialog_add_button(GTK_DIALOG(dialog), buttons[i - 1], i);
      gtk_dialog_set_default_response(GTK_DIALOG(dialog), 1);
#elif NCURSES
      int nbuttons = 0;
      for (i = 0; i < 3; i++)
        if (buttons[i])
          nbuttons++;
      buttonbox = newCDKButtonbox(dialog, 0, BOTTOM, 1, 0, "", 1, nbuttons,
                                  (char **)buttons, nbuttons, A_REVERSE, TRUE,
                                  FALSE);
      setCDKButtonboxCurrentButton(buttonbox, 0);
#endif
    }
    // Create dialog content.
#if GTK
    GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    if (text)
      gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(text), FALSE, TRUE, 5);
    if (info_text)
      gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(info_text), FALSE, TRUE,
                         5);
#endif
    if (type <= GTDIALOG_YESNO_MSGBOX) {
#if NCURSES
      char **lines;
      int nlines;
      if (text) {
        nlines = wrap((char *)text, width - 2, &lines);
        newCDKLabel(dialog, LEFT, TOP, lines, nlines, FALSE, FALSE);
        free(lines);
      }
      if (info_text) {
        nlines = wrap((char *)info_text, width - 2, &lines);
        newCDKLabel(dialog, LEFT, CENTER, lines, nlines, FALSE, FALSE);
        free(lines);
      }
#endif
    } else if (type >= GTDIALOG_INPUTBOX &&
               type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
#if GTK
      entry = gtk_entry_new();
      gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
      gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 5);
      if (type >= GTDIALOG_SECURE_INPUTBOX || no_show)
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
      if (text)
        gtk_entry_set_text(GTK_ENTRY(entry), text);
#elif NCURSES
      EDisplayType display = vMIXED;
      if (type >= GTDIALOG_SECURE_INPUTBOX || no_show)
        display = vHMIXED;
      entry = newCDKEntry(dialog, LEFT, TOP, (char *)title, (char *)info_text,
                          A_NORMAL, '_', display, 0, 0, 100, FALSE, FALSE);
      bindCDKObject(vENTRY, entry, KEY_TAB, buttonbox_tab, buttonbox);
      bindCDKObject(vENTRY, entry, KEY_BTAB, buttonbox_tab, buttonbox);
      if (text)
        setCDKEntryValue(entry, (char *)text);
#endif
    } else if (type == GTDIALOG_TEXTBOX) {
#if GTK
      textview = gtk_text_view_new();
      gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), editable);
      if (!focus_textbox && !editable)
        g_object_set(G_OBJECT(textview), "can-focus", FALSE, NULL);
      GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
      gtk_container_add(GTK_CONTAINER(scrolled), textview);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                     GTK_POLICY_AUTOMATIC,
                                     GTK_POLICY_AUTOMATIC);
      gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 5);
      if (font)
        gtk_widget_modify_font(textview, font);
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
      if (text)
        gtk_text_buffer_set_text(buffer, text, strlen(text));
#elif NCURSES
      EDisplayType display = editable ? vVIEWONLY : vMIXED;
      textview = newCDKMentry(dialog, LEFT, TOP, (char *)title,
                              (char *)info_text, A_NORMAL, '_', display, 0,
                              height - 8, height - 8, 0, FALSE, FALSE);
      if (text)
        setCDKMentryValue(textview, (char *)text);
#endif
      if (file) {
        FILE *f = fopen(file, "r");
        if (f) {
          fseek(f, 0, SEEK_END);
          int len = ftell(f);
          char *buf = malloc(len + 1);
          rewind(f);
          fread(buf, 1, len, f);
          buf[len] = '\0';
#if GTK
          gtk_text_buffer_set_text(buffer, buf, len);
#elif NCURSES
          setCDKMentryValue(textview, buf);
#endif
          free(buf);
          fclose(f);
        }
      }
#if GTK
      if (strcmp(scroll_to, "top") == 0)
        g_signal_emit_by_name(G_OBJECT(textview), "move-cursor",
                              GTK_MOVEMENT_BUFFER_ENDS, -1, 0);
      else
        g_signal_emit_by_name(G_OBJECT(textview), "move-cursor",
                              GTK_MOVEMENT_BUFFER_ENDS, 1, 0);
      if (selected)
        g_signal_emit_by_name(G_OBJECT(textview), "select-all", TRUE);
#elif NCURSES
      if (strcmp(scroll_to, "top") == 0)
        injectCDKMentry(textview, KEY_HOME);
      else
        injectCDKMentry(textview, KEY_END);
#endif
    } else if (type == GTDIALOG_PROGRESSBAR) {
#if GTK
      progressbar = gtk_progress_bar_new();
      gtk_box_pack_start(GTK_BOX(vbox), progressbar, FALSE, TRUE, 5);
      if (percent)
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar),
                                      0.01 * percent);
      if (text)
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar), text);
#elif NCURSES
#endif
    } else if (type == GTDIALOG_DROPDOWN ||
               type == GTDIALOG_STANDARD_DROPDOWN) {
#if GTK
      combobox = gtk_combo_box_text_new();
      combobox = gtk_combo_box_new_text();
      gtk_box_pack_start(GTK_BOX(vbox), combobox, FALSE, TRUE, 5);
      if (exit_onchange)
        g_signal_connect(G_OBJECT(combobox), "changed",
                         G_CALLBACK(close_dropdown), (gpointer)dialog);
      for (i = 0; i < len; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), items[i]);
      gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), selected);
#elif NCURSES
      combobox = newCDKItemlist(dialog, LEFT, TOP, (char *)title,
                                (char *)info_text, (char **)items, len, 0,
                                FALSE, FALSE);
#endif
    } else if (type == GTDIALOG_FILTEREDLIST) {
#if GTK
      entry = gtk_entry_new();
      gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
      gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);
      GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
      gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
      treeview = gtk_tree_view_new();
      gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), TRUE);
      g_signal_connect(G_OBJECT(treeview), "select-cursor-row",
                       G_CALLBACK(list_select), (gpointer)dialog);
      for (i = 0; i < ncols; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *treecol =
          gtk_tree_view_column_new_with_attributes(cols[i], renderer, "text", i,
                                                   NULL);
        gtk_tree_view_column_set_sizing(treecol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), treecol);
      }
      GType *cols = g_new0(GType, ncols);
      for (i = 0; i < ncols; i++)
        cols[i] = G_TYPE_STRING;
      list = gtk_list_store_newv(ncols, cols);
      free(cols);
      GtkTreeModel *filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(list),
                                                       NULL);
      gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
                                             list_visible, (gpointer)entry,
                                             NULL);
      gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), filter);
      g_signal_connect(G_OBJECT(entry), "key-release-event",
                       G_CALLBACK(entry_keypress), (gpointer)treeview);
      gtk_container_add(GTK_CONTAINER(scrolled), treeview);
      if (multiple)
        gtk_tree_selection_set_mode(
          gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
                                      GTK_SELECTION_MULTIPLE);
      if (text)
        gtk_entry_set_text(GTK_ENTRY(entry), text);
      int col = 0;
      GtkTreeIter iter;
      for (i = 0; i < len; i++) {
        if (col == 0)
          gtk_list_store_append(list, &iter);
        gtk_list_store_set(list, &iter, col++, items[i], -1);
        if (col == ncols)
          col = 0; // new row
      }
#elif NCURSES
      entry = newCDKEntry(dialog, LEFT, TOP, (char *)title, (char *)info_text,
                          A_NORMAL, '_', vMIXED, 0, 0, 100, FALSE, FALSE);
      char **rows = item_rows((char **)cols, ncols, (char **)items, len);
      int num_rows = (len + ncols - 1) / ncols; // account for non-full rows
      scrolled = newCDKScroll(dialog, LEFT, CENTER, RIGHT, -6, 0, rows[-1],
                              rows, num_rows, FALSE, A_REVERSE, TRUE, FALSE);
      model.rows = rows;
      model.num_rows = num_rows;
      model.filtered_rows = malloc(sizeof(char *) * num_rows);
      for (i = 0; i < num_rows; i++)
        model.filtered_rows[i] = model.rows[i];
      model.scrolled = scrolled;
      bindCDKObject(vENTRY, entry, KEY_TAB, buttonbox_tab, buttonbox);
      bindCDKObject(vENTRY, entry, KEY_BTAB, buttonbox_tab, buttonbox);
      bindCDKObject(vENTRY, entry, KEY_UP, scrolled_key, scrolled);
      bindCDKObject(vENTRY, entry, KEY_DOWN, scrolled_key, scrolled);
      bindCDKObject(vENTRY, entry, KEY_PPAGE, scrolled_key, scrolled);
      bindCDKObject(vENTRY, entry, KEY_NPAGE, scrolled_key, scrolled);
      setCDKEntryPostProcess(entry, entry_keypress, &model);
      // TODO: commands to scroll the list to the right and left.
      if (text)
        setCDKEntryValue(entry, (char *)text);
#endif
    }
  } else {
    if (type == GTDIALOG_FILESELECT) {
#if GTK
      dialog = gtk_file_chooser_dialog_new(title, NULL,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           GTK_STOCK_CANCEL,
                                           GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
                                           GTK_RESPONSE_ACCEPT, NULL);
      if (only_dirs)
          gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog),
                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
#elif NCURSES
      dialog = initCDKScreen(newwin(height, width, 0, 0));
      fileselect = newCDKFselect(dialog, LEFT, TOP, 0, 0, (char *)title,
                                 (char *)text, A_NORMAL, '_', A_REVERSE, "</B>",
                                 "</N>", "</N>", "</N>", TRUE, FALSE);
#endif
    } else {
#if GTK
      dialog = gtk_file_chooser_dialog_new(title, NULL,
                                           GTK_FILE_CHOOSER_ACTION_SAVE,
                                           GTK_STOCK_CANCEL,
                                           GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
                                           GTK_RESPONSE_ACCEPT, NULL);
      gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                     TRUE);
#elif NCURSES
      dialog = initCDKScreen(newwin(height, width, 0, 0));
      fileselect = newCDKFselect(dialog, LEFT, TOP, 0, 0, (char *)title,
                                 (char *)text, A_NORMAL, '_', A_REVERSE, "</B>",
                                 "</N>", "</N>", "</N>", TRUE, FALSE);
#endif
    }
#if GTK
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), multiple);
    if (dir)
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), dir);
    if (filter)
      gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    if (file)
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), file);
#elif NCURSES
    if (dir)
      setCDKFselectDirectory(fileselect, (char *)dir);
    if (file) {
      char *dir = dirName((char *)file);
      setCDKFselectDirectory(fileselect, (char *)dir);
      // TODO: select file in the list.
      free(dir);
    }
#endif
  }
#if GTK
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "gtdialog", "gtdialog");
#endif

  // Run dialog; text output is stored in 'out'
  if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE &&
      type != GTDIALOG_PROGRESSBAR) {
#if GTK
    gtk_widget_show_all(dialog);
    if (timeout)
      g_timeout_add_seconds(timeout, timeout_dialog, (gpointer)dialog);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == -4)
      response = RESPONSE_DELETE;
#elif NCURSES
    WINDOW *border = newwin(height, width, 1, 1);
    box(border, 0, 0), wrefresh(border);
    refreshCDKScreen(dialog);
    int response;
    if (type >= GTDIALOG_INPUTBOX &&
        type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
      activateCDKEntry(entry, NULL);
      response = (entry->exitType == vNORMAL) ? 1 + buttonbox->currentButton
                                              : RESPONSE_DELETE;
    } else if (type == GTDIALOG_TEXTBOX && focus_textbox) {
      activateCDKMentry(textview, NULL);
      response = (textview->exitType == vNORMAL) ? 1 + buttonbox->currentButton
                                                 : RESPONSE_DELETE;
    } else if (type == GTDIALOG_DROPDOWN ||
               type == GTDIALOG_STANDARD_DROPDOWN) {
      activateCDKItemlist(combobox, NULL);
      response = (combobox->exitType == vNORMAL) ? 1 + buttonbox->currentButton
                                                 : RESPONSE_DELETE;
    } else if (type == GTDIALOG_FILTEREDLIST) {
      activateCDKEntry(entry, NULL);
      response = (entry->exitType == vNORMAL) ? 1 + buttonbox->currentButton
                                              : RESPONSE_DELETE;
    } else {
      response = 1 + activateCDKButtonbox(buttonbox, NULL);
      if (response == 0) // activateCDKButtonbox returns -1 on escape
        response = RESPONSE_DELETE;
    }
    wborder(border, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '), wrefresh(border);
    delwin(border);
    destroyCDKButtonbox(buttonbox);
#endif
    if (string_output && response > 0 && response <= 3) {
      int len = strlen(buttons[response - 1]);
      out = malloc(len + 1);
      strncpy(out, buttons[response - 1], len);
      out[len] = '\0';
    } else {
      out = malloc(3);
      sprintf(out, "%i", response);
    }
    if (type >= GTDIALOG_INPUTBOX && type != GTDIALOG_FILESELECT &&
        type != GTDIALOG_FILESAVE && type != GTDIALOG_PROGRESSBAR) {
      if (response > 0) { // no delete or timeout
        char *txt;
        int created = 0;
        if (type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
#if GTK
          txt = (char *)gtk_entry_get_text(GTK_ENTRY(entry));
#elif NCURSES
          txt = copy(getCDKEntryValue(entry));
          created = 1;
          destroyCDKEntry(entry);
#endif
        } else if (type == GTDIALOG_TEXTBOX) {
#if GTK
          GtkTextBuffer *buffer =
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
          GtkTextIter s, e;
          gtk_text_buffer_get_start_iter(buffer, &s);
          gtk_text_buffer_get_end_iter(buffer, &e);
          txt = gtk_text_buffer_get_text(buffer, &s, &e, TRUE);
          if (font) {
            gtk_widget_modify_font(textview, NULL);
            pango_font_description_free(font);
          }
#elif NCURSES
          txt = copy(getCDKMentryValue(textview));
          created = 1;
          destroyCDKMentry(textview);
#endif
        } else if (type == GTDIALOG_DROPDOWN ||
                   type == GTDIALOG_STANDARD_DROPDOWN) {
          if (string_output) {
#if GTK
            txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox));
#elif NCURSES
            if (len > 0)
              txt = (char *)items[getCDKItemlistCurrentItem(combobox)];
            else
              txt = "";
            destroyCDKItemlist(combobox);
#endif
          } else {
            txt = malloc(4);
            sprintf(txt, "%i",
#if GTK
                    gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
#elif NCURSES
                    getCDKItemlistCurrentItem(combobox));
            destroyCDKItemlist(combobox);
#endif
            created = 1;
          }
        } else if (type == GTDIALOG_FILTEREDLIST) {
#if GTK
          GString *gstr = g_string_new("");
          gtk_tree_selection_selected_foreach(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), list_foreach,
                                        gstr);
          txt = g_strdup(gstr->str);
          if (strlen(txt) > 0)
            txt[strlen(txt) - 1] = '\0'; // chomp '\n'
          g_string_free(gstr, TRUE);
          created = 1;
#elif NCURSES
          if (getCDKScrollItems(scrolled, NULL) > 0) {
            i = getCDKScrollCurrentItem(scrolled);
            if (strlen(getCDKEntryValue(entry)) > 0) {
              char *item = model.filtered_rows[i];
              int j;
              for (j = 0; j < model.num_rows; j++)
                if (strcmp(item, model.rows[j]) == 0) {
                  i = j; // non-filtered index of selected item
                  break;
                }
            }
            if (string_output) {
              if (i * ncols + output_col - 1 < len)
                txt = (char *)items[i * ncols + output_col - 1];
              else
                txt = "";
            } else {
              txt = malloc(4);
              sprintf(txt, "%i", i);
              created = 1;
            }
          } else {
            txt = "";
          }
          destroyCDKEntry(entry);
          destroyCDKScroll(scrolled);
          if (model.rows) {
            for (i = -1; i < model.num_rows; i++)
              free(model.rows[i]);
            free(&model.rows[-1]);
          }
          if (model.filtered_rows)
            free(model.filtered_rows);
#endif
        }
        char *new_out = malloc(strlen(out) + strlen(txt) + 2);
        sprintf(new_out, "%s\n%s", out, txt);
        free(out);
        out = new_out;
        if (created) free(txt);
      }
    }
  } else if (type == GTDIALOG_FILESELECT || type ==  GTDIALOG_FILESAVE) {
#if GTK
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
      if (type == GTDIALOG_FILESELECT &&
          gtk_file_chooser_get_select_multiple(GTK_FILE_CHOOSER(dialog))) {
        out = malloc(1);
        *out = '\0';
        GSList *filenames =
          gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
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
#elif NCURSES
    char *text = activateCDKFselect(fileselect, NULL);
    if (text) {
      int len = strlen(text);
      out = malloc(len + 1);
      strncpy(out, text, len);
      out[len] = '\0';
    } else {
      out = malloc(1);
      *out = '\0';
    }
    destroyCDKFselect(fileselect);
#endif
  } else if (type == GTDIALOG_PROGRESSBAR) {
#if GTK
    gtk_widget_show_all(GTK_WIDGET(dialog));
    // TODO: read from stdin and update progressbar
#endif
  }
  if (strcmp(out, "0") == 0 && string_output) {
    out = malloc(8);
    sprintf(out, "timeout");
  } else if (strcmp(out, "-1") == 0 && string_output) {
    out = malloc(7);
    sprintf(out, "delete");
  }
#if GTK
  gtk_widget_destroy(dialog);
#elif NCURSES
  delwin(dialog->window);
  destroyCDKScreen(dialog);
#endif
  if (!no_newline) {
    char *new_out = malloc(strlen(out) + 2);
    sprintf(new_out, "%s\n", out);
    free(out);
    out = new_out;
  }
  return out;
}

int error(int argc, char *argv[]) {
#ifndef NOHELP
  int type = -1;
  if (argc == 3)
    type = gtdialog_type(argv[2]);
  const char *msg = 0;
  switch (type) {
  case GTDIALOG_MSGBOX:
    msg =
      "gtdialog msgbox [options]\n"
      "\n"
      "This control provides a generic message box. It allows you to customize the\n"
      "labels of the buttons. At least one button (--button1) must be specified. If\n"
      "labels for the other buttons are not given, the buttons will not appear on the\n"
      "message box\n"
      "\n"
      "Buttons go from right to left. --button1 is the right-most button.\n"
      "\n"
      "Also see the yesno-msgbox and ok-msgbox.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text\n"
      "      This is the main, bold message text.\n"
      "  --informative-text\n"
      "      This is the extra, smaller message text.\n"
      "  --icon\n"
      "      The name of the stock icon to use. This is incompatible with --icon-file.\n"
      "      Default is no icon.\n"
      "  --icon-file\n"
      "      The full path to the custom icon image you would like to use. Almost\n"
      "      every image format is accepted. This is incompatible with the --icon\n"
      "      option.\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n"
      "  --button1\n"
      "      required. This is the right-most button.\n"
      "  --button2\n"
      "      This is the middle button.\n"
      "  --button3\n"
      "      This is the left-most button. This will not be displayed if there is no\n"
      "      --button2 label specified.\n"
      "\n"
      "Example:\n"
      "  gtdialog msgbox --no-newline \\\n"
      "    --text \"What's your favorite OS?\" \\\n"
      "    --informative-text \"The 'Cancel' label auto-binds that button to esc\" \\\n"
      "    --button1 \"OS X\" --button2 \"GNU/LINUX\" --button3 \"Cancel\"\n";
    break;
  case GTDIALOG_OK_MSGBOX:
    msg =
      "gtdialog ok-msgbox [options]\n"
      "\n"
      "This control provides a standard Ok/Cancel message box.\n"
      "\n"
      "returns: 1 for ok, 2 for cancel depending on which button was pressed; gtk-ok\n"
      "or gtk-cancel if the --string-output option is given. If the dialog times out,\n"
      "it will return 0 or timeout.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --informative-text \"extra informative text to be displayed\"\n"
      "      This is the extra, smaller text.\n"
      "  --no-cancel\n"
      "      Don't show a cancel button, only \"Ok\".\n"
      "  --icon stockIconName\n"
      "      The name of the stock icon to use. This is incompatible with --icon-file\n"
      "      Default is no icon.\n"
      "  --icon-file \"/full/path/to/icon file\"\n"
      "      The full path to the custom icon image you would like to use. Almost\n"
      "      every image format is accepted. This is incompatible with the --icon\n"
      "      option.\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout numSeconds\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n"
      "\n"
      "Example:\n"
      "  gtdialog ok-msgbox --text \"We need to make sure you see this message\" \\\n"
      "    --informative-text \"(Yes, the message was to inform you about itself)\" \\\n"
      "    --no-newline --float\n";
    break;
  case GTDIALOG_YESNO_MSGBOX:
    msg =
      "gtdialog yesno-msgbox [options]\n"
      "\n"
      "This control provides a standard Yes/No/Cancel message box.\n"
      "\n"
      "returns: 1 for yes, 2 for no, 3 for cancel depending on which button was\n"
      "pressed; gtk-yes, gtk-no, or gtk-cancel if the --string-output option is\n"
      "given. If the dialog times out, it will return 0 or timeout.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --informative-text \"extra informative text to be displayed\"\n"
      "      This is the extra, smaller text.\n"
      "  --no-cancel\n"
      "      Don't show a cancel button, only \"Ok\".\n"
      "  --icon stockIconName\n"
      "      The name of the stock icon to use. This is incompatible with --icon-file\n"
      "      Default is no icon.\n"
      "  --icon-file \"/full/path/to/icon file\"\n"
      "      The full path to the custom icon image you would like to use. Almost\n"
      "      every image format is accepted. This is incompatible with the --icon\n"
      "      option.\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout numSeconds\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n"
      "\n"
      "Example:\n"
      "  gtdialog yesno-msgbox --no-cancel --string-output --no-newline \\\n"
      "    --text \"This is a simple first example\" \\\n"
      "    --informative-text \"We're just going to echo the string output\"\n";
    break;
  case GTDIALOG_INPUTBOX:
    msg =
      "gtdialog inputbox [options]\n"
      "\n"
      "This control provides a one line input box and customizable buttons. At least\n"
      "one button (--button1) must be specified. If labels for the other buttons are\n"
      "not given, the buttons will not appear on the message box.\n"
      "\n"
      "Buttons go from right to left. --button1 is the right-most button.\n"
      "\n"
      "returns: 1, 2, or 3 depending on which button was pressed; or the label of the\n"
      "button if the --string-output option is given. On the next line will be the\n"
      "text provided by the user in the textbox. On a timeout, it will return 0 or\n"
      "timeout, and the text from the textbox will not be returned.\n"
      "\n"
      "Also see the standard-inputbox.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --informative-text \"extra informative text to be displayed\"\n"
      "      This is the extra, smaller text.\n"
      "  --button1\n"
      "      required. This is the right-most button.\n"
      "  --button2\n"
      "      This is the middle button.\n"
      "  --button3\n"
      "      This is the left-most button. This will not be displayed if there is no\n"
      "      --button2 label specified.\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout numSeconds\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n"
      "  --no-show\n"
      "      This makes it a secure inputbox. Instead of what the user types, only\n"
      "      dots will be shown.\n"
      "\n"
      "Example:\n"
      "  gtdialog inputbox --title \"Search\" --no-newline \\\n"
      "    --informative-text \"Enter your search term\" \\\n"
      "    --text \"foobar\" \\\n"
      "    --button1 \"Search\" --button2 \"Search all\" \\\n"
      "    --width 600\n";
    break;
  case GTDIALOG_STANDARD_INPUTBOX:
    msg =
      "gtdialog standard-inputbox [options]\n"
      "\n"
      "This control provides a standard input box with \"Ok\" and \"Cancel\" buttons.\n"
      "\n"
      "returns: 1 for ok, 2 for cancel depending on which button was pressed; gtk-ok,\n"
      "or gtk-cancel if the --string-output option is given. On the next line will be\n"
      "the text provided by the user in the textbox. On a timeout, it will return 0 or\n"
      "timeout, and the text from the textbox will not be returned.\n"
      "\n"
      "Also see the standard-inputbox.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --informative-text \"extra informative text to be displayed\"\n"
      "      This is the extra, smaller text.\n"
      "  --no-cancel\n"
      "      Don't show a cancel button, only \"Ok\".\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout numSeconds\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n"
      "  --no-show\n"
      "      This makes it a secure inputbox. Instead of what the user types, only\n"
      "      dots will be shown.\n"
      "\n"
      "Example:\n"
      "  gtdialog standard-inputbox --title \"Your Name\" --no-newline \\\n"
      "    --informative \"Enter your name\"\n";
    break;
  case GTDIALOG_SECURE_INPUTBOX:
    msg =
      "gtdialog secure-inputbox [options]\n"
      "\n"
      "This is an alias for running an inputbox with the --no-show option. All\n"
      "options available to inputbox are available to secure-inputbox.\n";
    break;
  case GTDIALOG_SECURE_STANDARD_INPUTBOX:
    msg =
      "gtdialog secure-standard-inputbox [options]\n"
      "\n"
      "This is an alias for running a standard-inputbox with the --no-show option. All\n"
      "options available to standard-inputbox are available to secure-standard-inputbox.\n";
    break;
  case GTDIALOG_FILESELECT:
    msg =
      "gtdialog fileselect [options]\n"
      "\n"
      "This control provides a file selection window.\n"
      "\n"
      "returns: the files or directories selected by the user, or nothing if they\n"
      "cancel.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --select-directories\n"
      "      Allow the user to select directories as well as files. Default is to\n"
      "      disallow it.\n"
      "  --select-only-directories\n"
      "      Allows the user to select only directories.\n"
      "  --packages-as-directories\n"
      "      Allows the user to navigate into packages as if they were directories,\n"
      "      rather than selecting the package as a file.\n"
      "  --select-multiple\n"
      "      Allow the user to select more than one file. Default is to allow only one\n"
      "      file/directory selection.\n"
      "  --with-extensions list of extensions\n"
      "      Limit selectable files to ones with these extensions. list of extensions\n"
      "      should be space separated, and given as multiple arguments (ie: don't\n"
      "      double quote the list).\n"
      "      Example: gtdialog fileselect --with-extensions .c .h .m .txt\n"
      "      The period/dot at the start of each extension is optional.\n"
      "  --with-directory directory\n"
      "      Start the file select window in directory. The default value is up to the\n"
      "      system, and will usually be the last directory visited in a file select\n"
      "      dialog.\n"
      "  --with-file file\n"
      "      Start the file select window with file already selected. By default no\n"
      "      file will be selected. This must be used with --with-directory. It should\n"
      "      be the filename of a file within the directory.\n"
      "\n"
      "Example:\n"
      "  gtdialog fileselect \\\n"
      "    --title \"This is a fileselect\" \\\n"
      "    --text \"Choose the source file for the main controller\" \\\n"
      "    --with-extensions .c .m .cpp\n";
    break;
  case GTDIALOG_FILESAVE:
    msg =
      "gtdialog filesave [options]\n"
      "\n"
      "This control provides a file save window, which allows the user to select a\n"
      "file, or specify a new file.\n"
      "\n"
      "This dialog allows the user to create directories. However, if the user\n"
      "specifies a file that does not yet exist, it will not be created. This is a\n"
      "task for your script.\n"
      "\n"
      "returns: the file selected (which may not exist), or nothing if they cancel.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --packages-as-directories\n"
      "      Allows the user to navigate into packages as if they were directories,\n"
      "      rather than selecting the package as a file.\n"
      "  --no-create-directories\n"
      "      Prevents the user from creating new directories.\n"
      "  --with-extensions list of extensions\n"
      "      Limit selectable files to ones with these extensions. list of extensions\n"
      "      should be space separated, and given as multiple arguments (ie: don't\n"
      "      double quote the list).\n"
      "      Example: gtdialog filesave --with-extensions .c .h .m .txt\n"
      "      The period/dot at the start of each extension is optional.\n"
      "  --with-directory directory\n"
      "      Start the file select window in directory. The default value is up to the\n"
      "      system, and will usually be the last directory visited in a file select\n"
      "      dialog.\n"
      "  --with-file file\n"
      "      Start the file select window with file already selected. By default no\n"
      "      file will be selected. This must be used with --with-directory. It should\n"
      "      be the filename of a file within the directory.\n";
    break;
  case GTDIALOG_TEXTBOX:
    msg =
      "gtdialog textbox [options]\n"
      "\n"
      "This is a text box with a large text area.\n"
      "\n"
      "At least one button (--button1) must be specified. If labels for the other\n"
      "buttons are not given, the buttons will not appear on the text box.\n"
      "\n"
      "Buttons go from right to left. --button1 is the right-most button.\n"
      "\n"
      "returns: 1, 2, or 3 depending on which button was pressed; or the label of the\n"
      "button if the --string-output option is given. If the --editable option was\n"
      "given, it will print the contents of the text box following the return value\n"
      "for the button. On a timeout, it will return 0 or timeout, and the text from\n"
      "the textbox will not be returned.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --text-from-file filename\n"
      "      This is the message above the text box.\n"
      "  --informative-text \"extra informative text to be displayed\"\n"
      "      This is the extra, smaller text.\n"
      "  --button1\n"
      "      required. This is the right-most button.\n"
      "  --button2\n"
      "      This is the middle button.\n"
      "  --button3\n"
      "      This is the left-most button. This will not be displayed if there is no\n"
      "      --button2 label specified.\n"
      "  --editable\n"
      "      Makes the text box editable. When this option is set, the return value\n"
      "      for the button will be followed with the contents of the text box.\n"
      "  --focus-textbox\n"
      "      This option is only useful when --editable is set. This makes the initial\n"
      "      focus on the textbox rather than the rightmost button.\n"
      "  --selected\n"
      "      Selects all the text in the text box.\n"
      "  --scroll-to bottom_or_top\n"
      "      Where bottom_or_top is one of bottom or top. Causes the text box to\n"
      "      initially scroll to the bottom or top if the text it contains is larger\n"
      "      than its current view. Default is top.\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout numSeconds\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n"
      "  --monospaced-font\n"
      "      A monospaced font is used in the text box instead of a proportional one."
      "\n"
      "Example:\n"
      "  gtdialog textbox --title \"License\" --no-newline \\\n"
      "    --informative-text \"Do you agree with the terms of this license?\" \\\n"
      "    --text-from-file COPYING --button1 gtk-ok --button2 gtk-cancel\n";
    break;
  case GTDIALOG_PROGRESSBAR:
    msg =
      "gtdialog progressbar [options]\n"
      "\n"
      "The progress bar is a bit different, and slightly more complex than the other\n"
      "controls. It has no return value, but reads input on stdin. It will continue to\n"
      "display until it reads an EOF.\n"
      "\n"
      "Input for the progress bar is in the form: newPercent updated text to be\n"
      "displayed and must be terminated by a newline. If you want to leave the current\n"
      "text intact, just provide the new percent. newPercent should be a number.\n"
      "\n"
      "Examples (the first sets new text, the second leaves the old text):\n"
      "  26 We're now at 26%\n"
      "  26\n"
      "\n"
      "In your code it would like this: \"26 We're at 26%\\n\". That newline is\n"
      "important.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"initial text to display\"\n"
      "      This is the text that will be initially displayed.\n"
      "  --percent number\n"
      "      Initial percentage, between 0 and 100, for the progress bar\n"
      "  --indeterminate\n"
      "      This option makes the progress bar an animated \"barbershop pole\" (for\n"
      "      lack of better description). It does not indicate how far the operations\n"
      "      you're performing have progressed; it just shows that your\n"
      "      application/script is busy. You can still update the text of the label\n"
      "      when writing to gtdialog's stdin - and it doesn't matter what percentage\n"
      "      you feed it.\n";
    break;
  case GTDIALOG_DROPDOWN:
    msg =
      "gtdialog dropdown [options]\n"
      "\n"
      "This control provides a dropdown list of items to select from and customizable\n"
      "buttons. Values for the dropdown list must be provided. At least one button\n"
      "(--button1) must be specified. If labels for the other buttons are not given,\n"
      "the buttons will not appear on the message box.\n"
      "\n"
      "Buttons go from right to left. --button1 is the right-most button.\n"
      "\n"
      "returns: 1, 2, or 3 depending on which button was pressed; or the label of the\n"
      "button if the --string-output option is given. Returns 4 (for both regular and\n"
      "--string-output) if the user didn't press a button due to --exit-onchange. On\n"
      "the following line will be the index (zero-based) of the selected item, or its\n"
      "label if the --string-output option is given. On a timeout, it will return 0 or\n"
      "timeout, and the selected item will not be returned.\n"
      "\n"
      "Also see the standard-dropdown.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --items list of values\n"
      "      required. These are the labels for the options provided in the dropdown\n"
      "      box. list of values should be space separated, and given as multiple\n"
      "      arguments (ie: don't double quote the entire list. Provide it as you\n"
      "      would multiple arguments for any shell program).\n"
      "      Example: gtdialog dropdown --text \"Favorite OS?\" --items \"GNU/Linux\"\n"
      "      \"OS X\" Windows Amiga \"TI 89\" --button1 gtk-ok\n"
      "  --pulldown\n"
      "      Sets the style to a pull-down box, which differs slightly from the\n"
      "      default pop-up style. The first item remains visible. This option\n"
      "      probably isn't very useful for a single-function dialog such as those\n"
      "      gtdialog provides, but I've included it just in case it is. To see how\n"
      "      their appearances differ, just try them both.\n"
      "  --button1\n"
      "      required. This is the right-most button.\n"
      "  --button2\n"
      "      This is the middle button.\n"
      "  --button3\n"
      "      This is the left-most button. This will not be displayed if there is no\n"
      "      --button2 label specified.\n"
      "  --exit-onchange\n"
      "      Makes the program exit immediately after the selection changes, rather\n"
      "      than waiting for the user to press one of the buttons. This makes the\n"
      "      return value for the button 4 (for both regular output and with\n"
      "      --string-output).\n"
      "  --selected index\n"
      "      The zero-based index of the item in the list to select. The first item in\n"
      "      the list is always selected by default.\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout numSeconds\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n"
      "\n"
      "Example:\n"
      "  gtdialog dropdown --title \"Preferred OS\" --no-newline \\\n"
      "    --text \"What is your favorite OS?\" \\\n"
      "    --items \"Mac OS X\" \"GNU/Linux\" \"Windows\" --button1 'That one!' \\\n"
      "    --button2 Nevermind\n";
    break;
  case GTDIALOG_STANDARD_DROPDOWN:
    msg =
      "gtdialog standard-dropdown [options]\n"
      "\n"
      "This control provides a dropdown list of items to select from and the standard\n"
      "\"Ok\" and \"Cancel\" buttons. Values for the dropdown list must be provided.\n"
      "\n"
      "returns: 1 for ok, 2 for cancel, depending on which button was pressed; or\n"
      "gtk-ok or gtk-cancel if the --string-output option is given. Returns 4 (for\n"
      "both regular and --string-output) if the user didn't press a button due to\n"
      "--exit-onchange. On the following line will be the index (zero-based) of the\n"
      "selected item, or its label if the --string-output option is given. On a\n"
      "timeout, it will return 0 or timeout, and the selected item will not be\n"
      "returned.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      This is the main, bold message text.\n"
      "  --items list of values\n"
      "      required. These are the labels for the options provided in the dropdown\n"
      "      box. list of values should be space separated, and given as multiple\n"
      "      arguments (ie: don't double quote the entire list. Provide it as you\n"
      "      would multiple arguments for any shell program).\n"
      "      Example: gtdialog dropdown --text \"Favorite OS?\" --items \"GNU/Linux\"\n"
      "      \"OS X\" Windows Amiga \"TI 89\" --button1 gtk-ok\n"
      "  --pulldown\n"
      "      Sets the style to a pull-down box, which differs slightly from the\n"
      "      default pop-up style. The first item remains visible. This option\n"
      "      probably isn't very useful for a single-function dialog such as those\n"
      "      gtdialog provides, but I've included it just in case it is. To see how\n"
      "      their appearances differ, just try them both.\n"
      "  --selected index\n"
      "      The zero-based index of the item in the list to select. The first item in\n"
      "      the list is always selected by default.\n"
      "  --exit-onchange\n"
      "      Makes the program exit immediately after the selection changes, rather\n"
      "      than waiting for the user to press one of the buttons. This makes the\n"
      "      return value for the button 4 (for both regular output and with\n"
      "      --string-output).\n"
      "  --float\n"
      "      Float on top of all windows.\n"
      "  --timeout numSeconds\n"
      "      The amount of time, in seconds, that the window will be displayed if the\n"
      "      user does not click a button. Does not time out by default.\n";
    break;
  case GTDIALOG_FILTEREDLIST:
    msg =
      "gtdialog filteredlist [options]\n"
      "\n"
      "This control provides a list of items to filter through. Text entered into the\n"
      "input box is matched case-sensitively to the first column of list items. Non-\n"
      "matches are not displayed. Spaces (' ') are treated as globs ('*'), so \"f b\" is\n"
      "represented as \"f*b\" and will match \"foobar\", \"fobar\", \"fbar\", etc.\n"
      "\n"
      "Options: (in addition to global options)\n"
      "  --text \"main text message\"\n"
      "      The initial text in the input box.\n"
      "  --informative-text \"extra informative text to be displayed\"\n"
      "      The text for the label above the input box.\n"
      "  --columns list of columns\n"
      "      Required. These are the names of the columns in the list. list of columns\n"
      "      should be space separated and given as multiple arguments (ie: don't\n"
      "      double quote the entire list. Provide it as you would multiple arguments\n"
      "      for any shell program).\n"
      "      Example: gtdialog filteredlist --columns Foo Bar --items foo bar\n"
      "  --items list of items\n"
      "      Required after --columns. These are the items in the list each inserted\n"
      "      into the first empty column in the first non-full row. list of items\n"
      "      should be space separated and given as multiple arguments (ie: don't\n"
      "      double quote the entire list. Provide it as you would multiple arguments\n"
      "      for any shell program).\n"
      "  --search-column col\n"
      "      Required after --columns. The column to use for searching. Default is 1.\n"
      "  --output-column col\n"
      "      The column to use for --string-output. Default is 1.\n"
      "  --button1 \"label for button 1\"\n"
      "      Required. The right-most button.\n"
      "  --button2 \"label for button 2\"\n"
      "      The middle button.\n"
      "  --button3 \"label for button 3\"\n"
      "      The left-most button. It will not be displayed if there is no --button2\n"
      "      label specified.\n"
      "  --select-multiple\n"
      "      Allows the user to select more than one item. Default is to allow only one\n"
      "      selection.\n"
      "\n"
      "Example:\n"
      "  gtdialog filteredlist \\\n"
      "    --columns Foo Bar \\\n"
      "    --items foobar barfoo foobaz bazfoo barbaz bazbar \\\n"
      "    --select-multiple\n";
    break;
  default:
    msg =
      "Usage: gtdialog type [options]\n"
      "  Available Types:\n"
      "    msgbox, ok-msgbox, yesno-msgbox, inputbox, standard-inputbox,\n"
      "    secure-inputbox, secure-standard-inputbox, fileselect, filesave,\n"
      "    textbox, progressbar, dropdown, standard-dropdown, filteredlist\n"
      "\n"
      "  Global Options:\n"
      "    --title \"text for title\"\n"
      "        Sets the window's title\n"
      "    --string-output\n"
      "        Makes yes/no/ok/cancel buttons return values as \"gtk-yes\", \"gtk-no\",\n"
      "        \"gtk-ok\", or \"gtk-cancel\" instead of integers. When used with custom\n"
      "        button labels, returns the label you provided. If you use a GTK stock\n"
      "        item, (ie: \"gtk-copy\") then that exact text is returned, not the text\n"
      "        displayed on the button.\n"
      "    --no-newline\n"
      "        By default, return values will be printed with a trailing newline. This\n"
      "        will suppress that behavior. Note that when a control returns multiple\n"
      "        lines this will only suppress the trailing newline on the last line.\n"
      "    --width integer\n"
      "        Sets the width of the window. It's not advisable to use this option\n"
      "        without good reason, and some controls won't even respond to it. The\n"
      "        automatic size of most windows should suffice.\n"
      "    --height integer\n"
      "        Sets the height of the window. It's not advisable to use this option\n"
      "        without good reason, and some controls won't even respond to it. The\n"
      "        automatic size of most windows should suffice.\n"
      "    --debug\n"
      "        If you are not getting the results you expect, try turning on this\n"
      "        option. When there is an error, it will print ERROR: followed by the\n"
      "        error message. "
      "    --help\n"
      "        Gives a list of options and a link to this page.\n"
      "\n"
      "For detailed documentation: gtdialog help [type]"
      "\n"
      "Note that bubble mode is not supported, as well as the following options:\n"
      "\"--icon\", \"--icon-file\", \"--select-directories\", \"--packages-as-directories\",\n"
      "\"--no-create-directories\", and \"--pulldown\". The \"--debug\" flag does nothing\n"
      "either.\n"
      "\n"
      "When the dialog receives a 'delete' event, (e.g. when the user presses the\n"
      "Escape key) -1 or \"delete\" will be returned depending on --string-output.\n";
  }
  printf(msg);
#endif
  return 1;
}

#ifndef LIBRARY
int main(int argc, char *argv[]) {
  if (argc == 1 || strcmp(argv[1], "help") == 0)
    return error(argc, argv);
  int type = gtdialog_type(argv[1]);
  if (type < 0)
    return error(argc, argv);
#if GTK
  gtk_init(&argc, &argv);
#elif NCURSES
  initscr();
#endif
  char *out = gtdialog(type, argc - 2, (const char **)&argv[2]);
#if NCURSES
  endCDK();
#endif
  printf(out);
  free(out);
  return 0;
}
#endif
