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

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if GTK
#include <gtk/gtk.h>
#elif CURSES
#include <unistd.h>
#if (LIBRARY && !_WIN32)
#include <termios.h>
#elif _WIN32
#include <windows.h>
#endif
#include <iconv.h>
#include <cdk.h>
#endif

#include "gtdialog.h"

#if GTK
static GtkWindow *parent;
#else
static CDKENTRY *focused_entry;
#endif
static int RESPONSE_DELETE = -1, RESPONSE_TIMEOUT = 0, RESPONSE_CHANGE = 4;
// Options used by other functions.
static int indeterminate, stoppable, string_output, output_col = 1,
           search_col = 1;

// Default button labels.
#if GTK
#define STR_OK "gtk-ok"
#define STR_CANCEL "gtk-cancel"
#define STR_YES "gtk-yes"
#define STR_NO "gtk-no"
#define STR_STOP "gtk-stop"
#elif CURSES
#define STR_OK "Ok"
#define STR_CANCEL "Cancel"
#define STR_YES "Yes"
#define STR_NO "No"
#define STR_STOP "Stop"
#endif

#if GTK
// Translate GTK 2.x API to GTK 3.0 for compatibility.
#if GTK_CHECK_VERSION(3,0,0)
#define gtk_hbox_new(_,s) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, s)
#define gtk_vbox_new(_,s) gtk_box_new(GTK_ORIENTATION_VERTICAL, s)
#define gtk_combo_box_new_text gtk_combo_box_text_new
#define gtk_combo_box_append_text(w,t) \
  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w),t)
#define gtk_combo_box_get_active_text(w) \
  gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(w))
#endif
#if !GTK_CHECK_VERSION(3,4,0)
#define attach(...) gtk_table_attach(GTK_TABLE(table), __VA_ARGS__)
#define FILL(option) (GtkAttachOptions)(GTK_FILL | GTK_##option)
#else
// GTK 3.4 deprecated tables; translate from 2.x for compatibility.
#define gtk_table_new(...) \
  gtk_grid_new(), gtk_grid_set_column_spacing(GTK_GRID(table), 5)
#define attach(w, x1, _, y1, __, ...) \
  gtk_grid_attach(GTK_GRID(table), w, x1, y1, 1, 1)
#endif
#endif
#define copy(s) strcpy(malloc(strlen(s) + 1), s)

#if GTK
/**
 * Sets the parent window for gtdialogs.
 * @param window The parent window.
 */
void gtdialog_set_parent(GtkWindow *window) {
  parent = window;
}
#endif

/**
 * Returns the GTDialogType for the given type string.
 * @param type The string dialog type. Acceptable types are "msgbox",
 *   "ok-msgbox", "yesno-msgbox", "inputbox", "standard-inputbox",
 *   "secure-inputbox", "secure-standard-inputbox", "fileselect", "filesave",
 *   "textbox", "progressbar", "dropdown", "standard-dropdown", and
 *   "filteredlist".
 * @return GTDialogType or -1
 */
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
  else if (strcmp(type, "optionselect") == 0)
    return GTDIALOG_OPTIONSELECT;
  return -1;
}

// Callbacks and utility functions.
#if GTK
/** Signal for a dropdown selection change. */
static void close_dropdown(GtkWidget *dropdown, gpointer userdata) {
  g_signal_emit_by_name(userdata, "response", RESPONSE_CHANGE);
}

/** Signal for a keypress in the filteredlist entry. */
static gboolean entry_keypress(GtkWidget *entry, GdkEventKey *event,
                               gpointer userdata) {
  GtkTreeView *view = GTK_TREE_VIEW(userdata);
  GtkTreeModel *model = gtk_tree_view_get_model(view);
  gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(model));
  GtkTreeIter iter;
  if (gtk_tree_model_get_iter_first(model, &iter))
    gtk_tree_selection_select_iter(gtk_tree_view_get_selection(view), &iter);
  return FALSE;
}

/** Signal for when stdin is available for the progressbar. */
static gboolean read_stdin(GIOChannel *ch, GIOCondition cond, gpointer data) {
  GtkWidget *dialog = (GtkWidget *)data, *box, *progressbar, *button;
  if (cond == G_IO_IN) {
    char *input, *p;
    int status = g_io_channel_read_line(ch, &input, NULL, NULL, NULL);
    if (status == G_IO_STATUS_NORMAL) {
      box = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
      GList *children = gtk_container_get_children(GTK_CONTAINER(box));
      progressbar = (GtkWidget *)children->data;
      p = input;
      while (!isspace(*p)) p++;
      *p = '\0', p[strlen(p + 1)] = '\0'; // chomp '\n'
      if (!indeterminate)
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar),
                                      0.01 * atoi(input));
      else
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
      if (*(p + 1)) {
        if (stoppable) {
          box = gtk_dialog_get_action_area(GTK_DIALOG(dialog));
          GList *children2 = gtk_container_get_children(GTK_CONTAINER(box));
          button = (GtkWidget *)children2->data;
          if (strcmp(p + 1, "stop enable") == 0)
            gtk_widget_set_sensitive(button, TRUE);
          else if (strcmp(p + 1, "stop disable") == 0)
            gtk_widget_set_sensitive(button, FALSE);
          else
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar), p + 1);
          g_list_free(children2);
        } else gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar), p + 1);
      }
      free(input);
      g_list_free(children);
    }
  } else g_signal_emit_by_name(dialog, "response", 2); // 1 is for Stop pressed
  return !(cond & G_IO_HUP);
}

/**
 * Function for iterating over filteredlist selections.
 * Concatenates all selections into a single string delimited by newline
 * characters.
 */
static void list_foreach(GtkTreeModel *model, GtkTreePath *path,
                         GtkTreeIter *iter, gpointer userdata) {
  char *value;
  if (!string_output) {
    GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(model);
    path = gtk_tree_model_filter_convert_path_to_child_path(filter, path);
    value = gtk_tree_path_to_string(path);
    gtk_tree_path_free(path);
  } else gtk_tree_model_get(model, iter, output_col - 1, &value, -1);
  g_string_append_printf((GString *)userdata, "%s\n", value);
  free(value);
}

/** Signal for the 'enter' key being pressed in the filteredlist view. */
static gboolean list_keypress(GtkWidget *treeview, GdkEventKey *event,
                              gpointer userdata) {
  if (event->keyval == 0xff0d) // return key
    return (g_signal_emit_by_name(userdata, "response", 1), TRUE);
  return FALSE;
}

/** Signal for a row being activated in the filteredlist view. */
static gboolean list_select(GtkTreeView *treeview, GtkTreePath *path,
                            GtkTreeViewColumn *column, gpointer userdata) {
  return (g_signal_emit_by_name(userdata, "response", 1), TRUE);
}

/** Function for filtering filterdlist items based on user input. */
static gboolean list_visible(GtkTreeModel *model, GtkTreeIter *iter,
                             gpointer userdata) {
  const char *entry_text = gtk_entry_get_text(GTK_ENTRY(userdata));
  if (strlen(entry_text) == 0) return TRUE;
  char *text = g_utf8_strdown(entry_text, -1);
  char *value, *lower, *p;
  gtk_tree_model_get(model, iter, search_col - 1, &value, -1);
  if (!value) return TRUE; // no data yet
  lower = g_utf8_strdown(value, -1), p = lower;
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
  free(text), free(value), free(lower);
  g_strfreev(tokens);
  return visible;
}

/** Signal for a dialog timeout. */
static gboolean timeout_dialog(gpointer userdata) {
  return (g_signal_emit_by_name(userdata, "response", RESPONSE_TIMEOUT), FALSE);
}
#elif CURSES
/**
 * Returns the number of lines the given string occupies when wrapped to fit the
 * given number of characters per line and sets the given pointer to the wrapped
 * set of lines.
 * The pointer itself must be freed when finished; do not free its contents.
 * @param str The string to wrap. It is modified in place.
 * @param w The number of characters per line to wrap at.
 * @param plines An empty pointer that will ultimately contain the set of
 *   wrapped lines.
 * @return int number of wrapped lines
 */
static int wrap(char *str, int w, char ***plines) {
  // Wrap lines by replacing spaces with '\n' at the appropriate locations.
  int i, len = strlen(str);
  for (i = w; i < len; i += w) {
    int j = i;
    while (j >= 0 && !isspace(str[j])) j--;
    if (j + w > i) i = j, str[i++] = '\n'; // line length < w; cut here
  }
  // Count the number of '\n's to get the number of wrapped lines.
  int nlines = 1;
  char *p = str - 1;
  while ((p = strstr(p + 1, "\n"))) nlines++;
  // Create the list of lines.
  char **lines = malloc(nlines * sizeof(char *));
  lines[0] = str;
  for (i = 1; i < nlines; i++) {
    p = strstr(lines[i - 1], "\n"), *p = '\0';
    lines[i] = p + 1;
  }
  *plines = lines;
  return nlines;
}

/** Signal for the 'tab' and 'shift+tab' keys being pressed. */
static int entries_tab(EObjectType cdkType, void *object, void *data,
                       chtype key) {
  CDKENTRY *entry = (CDKENTRY *)object, **entries = (CDKENTRY **)data;
  int i = 0, len = 0;
  for (len = 0; entries[len]; len++) if (entries[len] == entry) i = len;
  if (key == KEY_TAB || key == KEY_DOWN)
    focused_entry = entries[i + 1];
  else
    focused_entry = (i > 0) ? entries[i - 1] : entries[len - 1];
  return (injectCDKEntry(entry, KEY_ENTER), TRUE);
}

/** Signal for the 'tab' and 'shift+tab' keys being pressed. */
static int buttonbox_tab(EObjectType cdkType, void *object, void *data,
                         chtype key) {
  return (injectCDKButtonbox((CDKBUTTONBOX *)data, key), TRUE);
}

/** The curses filteredlist model. */
typedef struct {
  /** The number of columns. */
  int ncols;
  /** The index of the column to filter against. */
  int search_col;
  /** The raw list of items. */
  char **items;
  /** The raw number of items. */
  int len;
  /**
   * The list of display rows.
   * The contents of each row are constructed from *items* and *ncols* and
   * suitable for display to the user.
   */
  char **rows;
  /** The number of display rows. */
  int num_rows;
  /** The list of filtered rows to actually display. */
  char **filtered_rows;
  /** The CDKENTRY the model is assigned to. */
  CDKENTRY *entry;
  /** CDKSCROLL the model is assigned to. */
  CDKSCROLL *scrolled;
} Model;

/**
 * Returns a copy of the given string converted to lower case.
 * The returned string must be freed when finished.
 * @param s The given string to lowercase. It may be freed immediately.
 */
static char *strdown(char *s) {
  char *lower = copy(s);
  int i, len = strlen(s);
  for (i = 0; i < len; i++) lower[i] = tolower(s[i]);
  return lower;
}

/** Signal for a keypress in the filteredlist entry. */
static int entry_keypress(EObjectType cdkType, void *object, void *data,
                           chtype key) {
  Model *model = (Model *)data;
  char *entry_text = getCDKEntryValue((CDKENTRY *)object);
  if (strlen(entry_text) > 0) {
    char *text = strdown(entry_text), **tokens = CDKsplitString(text, ' ');
    int i, j, row = 0;
    for (i = 0; i < model->len; i += model->ncols) {
      char *item = strdown(model->items[i + model->search_col - 1]), *p = item;
      int visible = TRUE;
      j = 0;
      while (tokens[j] != NULL) {
        if (!(p = strstr(p, tokens[j]))) {
          visible = FALSE;
          break;
        }
        p += strlen(tokens[j++]);
      }
      if (visible) model->filtered_rows[row++] = model->rows[i / model->ncols];
      free(item);
    }
    free(text);
    CDKfreeStrings(tokens);
    setCDKScrollItems(model->scrolled, model->filtered_rows, row, FALSE);
  } else setCDKScrollItems(model->scrolled, model->rows,
                           model->len / model->ncols, FALSE);
  HasFocusObj(ObjOf(model->scrolled)) = TRUE; // needed to draw highlight
  eraseCDKScroll(model->scrolled); // drawCDKScroll does not completely redraw
  drawCDKScroll(model->scrolled, TRUE), drawCDKEntry(model->entry, FALSE);
  HasFocusObj(ObjOf(model->scrolled)) = FALSE;
  return TRUE;
}

#if _WIN32
#define stpcpy(d, s) (strcpy(d, s), d + strlen(s))
#endif

#if !HAVE_CDKUTF8
/** Returns the number of bytes in the given UTF-8 character. */
static unsigned int utf8charlen(unsigned char ch) {
  if (ch < 0x80)
    return 1;
  else if (ch < 0x80 + 0x40 + 0x20)
    return 2;
  else if (ch < 0x80 + 0x40 + 0x20 + 0x10)
    return 3;
  else
    return 4;
}

/** Returns the number of characters in the given UTF-8 string. */
static size_t utf8strlen(const char *s) {
  size_t len = strlen(s), utf8len = 0;
  for (int i = 0; i < len; i += utf8charlen((unsigned char)(s[i]))) utf8len++;
  return utf8len;
}
#endif

/**
 * Creates and returns the set of display rows from the given list of columns,
 * number of columns, list of items, and number of items.
 * The returned list and its contents must be freed when finished. Note that the
 * returned list actually starts at index -1 where a column display row is. That
 * must be taken into account when freeing.
 * @param cols The list of column names.
 * @param ncols The number of columns.
 * @param items The list of items.
 * @param len The number of items.
 */
static char **item_rows(char **cols, int ncols, char **items, int len) {
  // Compute the column sizes needed to fit all row items in.
  int *col_widths = malloc(sizeof(int) * ncols);
  int i, j, row_len = 0;
  for (i = 0; i < ncols; i++) {
    int utf8max = utf8strlen(cols[i]), max = strlen(cols[i]);
    for (j = i; j < len; j += ncols) {
      int utf8len = utf8strlen(items[j]);
      if (utf8len > utf8max) utf8max = utf8len, max = strlen(items[j]);
    }
    col_widths[i] = utf8max, row_len += max + 1;
  }
  // Generate the display rows, padding row items to fit column widths and
  // separating columns with '|'s.
  // The column headers are a special case and need to be underlined too.
  char **new_items = malloc(sizeof(char *) * ((len + ncols - 1) / ncols + 1));
  for (i = -ncols; i < len; i += ncols) {
    char *new_item = malloc((i < 0) ? row_len + 4 : row_len);
    char *p = (i < 0) ? stpcpy(new_item, "</U>") : new_item;
    for (j = i; j < i + ncols && j < len; j++) {
      char *item = (i < 0) ? cols[j - i] : items[j];
      p = stpcpy(p, item);
      int padding = col_widths[j - i] - utf8strlen(item);
      while (padding-- > 0) *p++ = ' ';
      *p++ = (i < 0) ? '|' : ' ';
    }
    if (p > new_item) *(p - 1) = '\0';
    new_items[i / ncols + 1] = new_item;
  }
  free(col_widths);
  return &new_items[1]; // items start here
}

/** Signal for a scrolling keypress in the filteredlist entry. */
static int scrolled_key(EObjectType cdkType, void *object, void *data,
                        chtype key) {
  HasFocusObj(ObjOf((CDKSCROLL *)data)) = TRUE; // needed to draw highlight
  injectCDKScroll((CDKSCROLL *)data, key);
  HasFocusObj(ObjOf((CDKSCROLL *)data)) = FALSE;
  return TRUE;
}
#endif

/**
 * Creates, displays, and returns the result from a gtdialog of the given type
 * from the given set of parameters.
 * The string returned must be freed when finished.
 * @param type The GTDialogType type.
 * @param narg The number of parameters in *args*.
 * @param args The set of parameters for the dialog.
 * @return string result
 */
char *gtdialog(GTDialogType type, int narg, const char *args[]) {
#if (CURSES && LIBRARY && !_WIN32)
  struct termios term;
  tcgetattr(0, &term); // store initial terminal settings
#endif

  // Dialog options.
  int editable = FALSE, exit_onchange = FALSE, floating = FALSE,
      focus_textbox = FALSE, height = -1, no_create_dirs = FALSE,
      no_newline = FALSE, no_show = FALSE, percent = 0, select_multiple = FALSE,
      select_only_dirs = FALSE, select = 0, selected = FALSE, timeout_len = 0,
      width = -1;
  indeterminate = FALSE, stoppable = FALSE, string_output = FALSE;
  output_col = 1, search_col = 1;
  const char *buttons[3] = {NULL, NULL, NULL}, **cols = NULL, *icon = NULL,
             *icon_file = NULL, *info_text = NULL, **info_texts = NULL,
             **items = NULL, *scroll_to = "top", **selects = NULL, *text = NULL,
             **texts = NULL, *text_file = NULL, *title = "gtdialog",
             *with_dir = NULL, *with_file = NULL;
  // Other variables.
  int ncols = 0, nrows = 0, len = 0;
#if GTK
  PangoFontDescription *font = NULL;
  GtkFileFilter *filter = NULL;
#endif

  // Dialog defaults.
#if CURSES
  height = 10, width = 40;
#endif
  if (type == GTDIALOG_MSGBOX)
    buttons[0] = STR_OK;
  else if (type == GTDIALOG_OK_MSGBOX)
    buttons[0] = STR_OK, buttons[1] = STR_CANCEL;
  else if (type == GTDIALOG_YESNO_MSGBOX)
    buttons[0] = STR_YES, buttons[1] = STR_NO, buttons[2] = STR_CANCEL;
  else if (type == GTDIALOG_INPUTBOX || type == GTDIALOG_SECURE_INPUTBOX)
    buttons[0] = STR_OK;
  else if (type == GTDIALOG_STANDARD_INPUTBOX ||
           type == GTDIALOG_SECURE_STANDARD_INPUTBOX)
    buttons[0] = STR_OK, buttons[1] = STR_CANCEL;
#if CURSES
  else if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE)
    height = 20;
#endif
  else if (type == GTDIALOG_TEXTBOX)
#if GTK
    height = 300, width = 400, buttons[0] = STR_OK;
#elif CURSES
    height = 20, buttons[0] = STR_OK;
#endif
  else if (type == GTDIALOG_PROGRESSBAR)
    buttons[0] = STR_STOP;
  else if (type == GTDIALOG_DROPDOWN)
    buttons[0] = STR_OK;
  else if (type == GTDIALOG_STANDARD_DROPDOWN)
    buttons[0] = STR_OK, buttons[1] = STR_CANCEL;
  else if (type == GTDIALOG_FILTEREDLIST)
#if GTK
    height = 360, width = 500, buttons[0] = STR_OK;
#elif CURSES
    height = 20, buttons[0] = STR_OK;
#endif
  else if (type == GTDIALOG_OPTIONSELECT)
    buttons[0] = STR_OK;

  // Parse arguments.
  int i = 0;
  const char *arg = args[i++];
  while (arg && i <= narg) {
    if (strcmp(arg, "--button1") == 0) {
      if (type == GTDIALOG_MSGBOX || type == GTDIALOG_INPUTBOX ||
          type == GTDIALOG_TEXTBOX || type == GTDIALOG_DROPDOWN ||
          type == GTDIALOG_FILTEREDLIST || type == GTDIALOG_OPTIONSELECT)
        buttons[0] = args[i++];
    } else if (strcmp(arg, "--button2") == 0) {
      if (type == GTDIALOG_MSGBOX || type == GTDIALOG_INPUTBOX ||
          type == GTDIALOG_TEXTBOX || type == GTDIALOG_DROPDOWN ||
          type == GTDIALOG_FILTEREDLIST || type == GTDIALOG_OPTIONSELECT)
        buttons[1] = args[i++];
    } else if (strcmp(arg, "--button3") == 0) {
      if (type == GTDIALOG_MSGBOX || type == GTDIALOG_INPUTBOX ||
          type == GTDIALOG_TEXTBOX || type == GTDIALOG_DROPDOWN ||
          type == GTDIALOG_FILTEREDLIST || type == GTDIALOG_OPTIONSELECT)
        buttons[2] = args[i++];
    } else if (strcmp(arg, "--columns") == 0) {
      if (type == GTDIALOG_FILTEREDLIST) {
        cols = &args[i], ncols = 0;
        while (i < narg && strncmp(args[i], "--", 2) != 0) ncols++, i++;
      }
    } else if (strcmp(arg, "--editable") == 0) {
      if (type == GTDIALOG_TEXTBOX) editable = TRUE;
    } else if (strcmp(arg, "--exit-onchange") == 0) {
      if (type == GTDIALOG_DROPDOWN || type == GTDIALOG_STANDARD_DROPDOWN)
        exit_onchange = TRUE;
    } else if (strcmp(arg, "--float") == 0) {
      if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE &&
          type != GTDIALOG_PROGRESSBAR) floating = TRUE;
    } else if (strcmp(arg, "--focus-textbox") == 0) {
      if (type == GTDIALOG_TEXTBOX) focus_textbox = TRUE;
    } else if (strcmp(arg, "--height") == 0) {
      int h = atoi(args[i++]);
      if (h > 0) height = h;
    } else if (strcmp(arg, "--icon") == 0) {
      if (type >= GTDIALOG_MSGBOX && type <= GTDIALOG_YESNO_MSGBOX)
        icon = args[i++];
    } else if (strcmp(arg, "--icon-file") == 0) {
      if (type >= GTDIALOG_MSGBOX && type <= GTDIALOG_YESNO_MSGBOX)
        icon_file = args[i++];
    } else if (strcmp(arg, "--indeterminate") == 0) {
      if (type == GTDIALOG_PROGRESSBAR) indeterminate = TRUE;
    } else if (strcmp(arg, "--informative-text") == 0) {
      if (type < GTDIALOG_FILESELECT || type == GTDIALOG_TEXTBOX ||
          type == GTDIALOG_FILTEREDLIST || type == GTDIALOG_OPTIONSELECT) {
        info_text = args[i++];
        if (type >= GTDIALOG_INPUTBOX ||
            type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
          info_texts = &args[i], nrows = 0; // for multiple inputboxes, if any
          while (i < narg && strncmp(args[i], "--", 2) != 0) nrows++, i++;
        }
      }
    } else if (strcmp(arg, "--items") == 0) {
      items = &args[i], len = 0;
      while (i < narg && strncmp(args[i], "--", 2) != 0) len++, i++;
    } else if (strcmp(arg, "--monospaced-font") == 0) {
#if GTK
      if (type == GTDIALOG_TEXTBOX) {
        font = pango_font_description_new();
        pango_font_description_set_family(font, "monospace");
      }
#endif
    } else if (strcmp(arg, "--no-cancel") == 0) {
      if (type == GTDIALOG_OK_MSGBOX || type == GTDIALOG_STANDARD_INPUTBOX ||
          type == GTDIALOG_SECURE_STANDARD_INPUTBOX ||
          type == GTDIALOG_STANDARD_DROPDOWN)
        buttons[1] = NULL;
      else if (type == GTDIALOG_YESNO_MSGBOX)
        buttons[2] = NULL;
    } else if (strcmp(arg, "--no-create-directories") == 0) {
      if (type == GTDIALOG_FILESAVE) no_create_dirs = TRUE;
    } else if (strcmp(arg, "--no-newline") == 0) {
      no_newline = TRUE;
    } else if (strcmp(arg, "--no-show") == 0) {
      if (type >= GTDIALOG_INPUTBOX &&
          type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) no_show = TRUE;
    } else if (strcmp(arg, "--output-column") == 0) {
      if (type == GTDIALOG_FILTEREDLIST) {
        output_col = atoi(args[i++]);
        if (output_col < 1) output_col = 1;
      }
    } else if (strcmp(arg, "--percent") == 0) {
      if (type == GTDIALOG_PROGRESSBAR) percent = atoi(args[i++]);
    } else if (strcmp(arg, "--scroll-to") == 0) {
      if (type == GTDIALOG_TEXTBOX) focus_textbox = TRUE, scroll_to = args[i++];
    } else if (strcmp(arg, "--search-column") == 0) {
      if (type == GTDIALOG_FILTEREDLIST) {
        search_col = atoi(args[i++]);
        if (search_col < 1) search_col = 1;
      }
    } else if (strcmp(arg, "--select-multiple") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILTEREDLIST)
        select_multiple = TRUE;
    } else if (strcmp(arg, "--select-only-directories") == 0) {
      if (type == GTDIALOG_FILESELECT) select_only_dirs = TRUE;
    } else if (strcmp(arg, "--select") == 0) {
      if (type == GTDIALOG_DROPDOWN || type == GTDIALOG_STANDARD_DROPDOWN)
        select = atoi(args[i++]);
      else if (type == GTDIALOG_OPTIONSELECT) {
        selects = &args[i], select = 0;
        while (i < narg && strncmp(args[i], "--", 2) != 0) select++, i++;
      }
    } else if (strcmp(arg, "--selected") == 0) {
      if (type == GTDIALOG_TEXTBOX) selected = TRUE;
    } else if (strcmp(arg, "--stoppable") == 0) {
      if (type == GTDIALOG_PROGRESSBAR) stoppable = TRUE;
    } else if (strcmp(arg, "--string-output") == 0) {
      string_output = TRUE;
    } else if (strcmp(arg, "--text") == 0) {
      text = args[i++];
      if (type >= GTDIALOG_INPUTBOX &&
          type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
        texts = &args[i - 1], len = 1; // for multiple inputboxes, if any
        while (i < narg && strncmp(args[i], "--", 2) != 0) i++, len++;
      }
    } else if (strcmp(arg, "--text-from-file") == 0) {
      if (type == GTDIALOG_TEXTBOX) text_file = args[i++];
    } else if (strcmp(arg, "--timeout") == 0) {
      if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE &&
          type != GTDIALOG_PROGRESSBAR) timeout_len = atoi(args[i++]);
    } else if (strcmp(arg, "--title") == 0) {
      title = args[i++];
    } else if (strcmp(arg, "--width") == 0) {
      int w = atoi(args[i++]);
      if (w > 0) width = w;
    } else if (strcmp(arg, "--with-directory") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE)
        with_dir = args[i++];
    } else if (strcmp(arg, "--with-extension") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE) {
#if GTK
        filter = gtk_file_filter_new();
#endif
        while (i < narg && strncmp(args[i], "--", 2) != 0) {
#if GTK
          const char *ext = args[i];
          char *glob = g_strconcat((*ext == '.') ? "*" : "*.", ext, NULL);
          gtk_file_filter_add_pattern(filter, glob);
          g_free(glob);
#elif CURSES
          // TODO:
#endif
          i++;
        }
      }
    } else if (strcmp(arg, "--with-file") == 0) {
      if (type == GTDIALOG_FILESELECT || type == GTDIALOG_FILESAVE)
        with_file = args[i++];
    }
    arg = args[i++];
  }
  if (output_col > ncols) output_col = ncols;
  if (search_col > ncols) search_col = ncols;

  // Create dialog.
#if GTK
  GtkWidget *dialog, *entry, *entries[nrows], *textview, *progressbar,
            *combobox, *treeview, *options[nrows];
  GtkListStore *list;
#elif CURSES
  int cursor = curs_set(1); // enable cursor
  CDKSCREEN *dialog;
  CDKLABEL *labelt, *labeli;
  CDKENTRY *entry, *entries[nrows + 1];
  CDKMENTRY *textview;
//  CDKSLIDER *progressbar;
  CDKITEMLIST *combobox;
  CDKBUTTONBOX *buttonbox;
  CDKSCROLL *scrolled;
  Model model = {ncols, search_col, (char **)items, len, NULL, 0, NULL, NULL};
  CDKSELECTION *options;
  CDKFSELECT *fileselect;
  char cwd[FILENAME_MAX];
  if (getcwd(cwd, FILENAME_MAX)); // 'if' prevents compiler warning
#endif
  if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE) {
#if GTK
    dialog = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    if (parent) gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
    if (floating) gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_widget_set_size_request(GTK_WIDGET(dialog), width, height);
#elif CURSES
    // There will be a border drawn later, but account for it now.
    dialog = initCDKScreen(newwin(height - 2, width - 2, 2, 2));
#if (LIBRARY && !_WIN32)
    tcsetattr(0, TCSANOW, &term); // restore initial terminal settings
#endif
#endif
    // Create buttons.
    if (type != GTDIALOG_PROGRESSBAR || stoppable) {
#if GTK
      for (i = 3; i > 0; i--)
        if (buttons[i - 1])
          gtk_dialog_add_button(GTK_DIALOG(dialog), buttons[i - 1], i);
      gtk_dialog_set_default_response(GTK_DIALOG(dialog), 1);
#elif CURSES
      int nbuttons = 0;
      for (i = 0; i < 3; i++) if (buttons[i]) nbuttons++;
      buttonbox = newCDKButtonbox(dialog, 0, BOTTOM, 1, 0, "", 1, nbuttons,
                                  (char **)buttons, nbuttons, A_REVERSE, TRUE,
                                  FALSE);
      setCDKButtonboxCurrentButton(buttonbox, 0);
#endif
    }
    // Create dialog content.
#if GTK
    GtkWidget *vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    if (icon || icon_file) {
      // Spacing, border width, and alignment match GtkMessageDialogs'.
      GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
      gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
      GtkWidget *image = NULL;
      if (icon)
        image = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_DIALOG);
      else
        image = gtk_image_new_from_file(icon_file);
#if GTK_CHECK_VERSION(3,0,0)
      gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
      gtk_widget_set_valign(image, GTK_ALIGN_START);
#else
      gtk_misc_set_alignment(GTK_MISC(image), 0.5, 0.0);
#endif
      gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
      GtkWidget *vbox2 = gtk_vbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);
      vbox = vbox2;
    }
    if (text && (type < GTDIALOG_INPUTBOX ||
                 type > GTDIALOG_SECURE_STANDARD_INPUTBOX) &&
        type != GTDIALOG_TEXTBOX && type != GTDIALOG_PROGRESSBAR &&
        type != GTDIALOG_FILTEREDLIST)
      gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(text), FALSE, TRUE, 5);
    if (info_text && type != GTDIALOG_PROGRESSBAR)
      gtk_box_pack_start(GTK_BOX(vbox), gtk_label_new(info_text), FALSE, TRUE,
                         5);
#endif
    if (type <= GTDIALOG_YESNO_MSGBOX) {
#if CURSES
      char **lines;
      int nlines;
      if (text) {
        nlines = wrap((char *)text, width - 2, &lines);
        labelt = newCDKLabel(dialog, LEFT, TOP, lines, nlines, FALSE, FALSE);
        free(lines);
      }
      if (info_text) {
        nlines = wrap((char *)info_text, width - 2, &lines);
        labeli = newCDKLabel(dialog, LEFT, CENTER, lines, nlines, FALSE, FALSE);
        free(lines);
      }
#endif
    } else if (type >= GTDIALOG_INPUTBOX &&
               type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
#if GTK
      if (nrows < 2) {
        // Single entry inputbox.
        entry = gtk_entry_new();
        gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
        gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, TRUE, 5);
        if (type >= GTDIALOG_SECURE_INPUTBOX || no_show)
          gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
        if (text) gtk_entry_set_text(GTK_ENTRY(entry), text);
      } else {
        // Multiple entry inputbox.
        GtkWidget *table;
        table = gtk_table_new(nrows, 2, FALSE); // macro in GTK 3.4
        gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 5);
        for (i = 0; i < nrows; i++) {
          GtkWidget *label = gtk_label_new(info_texts[i]);
          entries[i] = gtk_entry_new();
          if (i == nrows - 1)
            gtk_entry_set_activates_default(GTK_ENTRY(entries[i]), TRUE);
          if (type >= GTDIALOG_SECURE_INPUTBOX || no_show)
            gtk_entry_set_visibility(GTK_ENTRY(entries[i]), FALSE);
          if (i < len) gtk_entry_set_text(GTK_ENTRY(entries[i]), texts[i]);
          attach(label, 0, 1, i, i + 1, FILL(SHRINK), FILL(SHRINK), 5, 0);
          attach(entries[i], 1, 2, i, i + 1, FILL(EXPAND), FILL(SHRINK), 5, 0);
        }
      }
#elif CURSES
      EDisplayType display = vMIXED;
      if (type >= GTDIALOG_SECURE_INPUTBOX || no_show) display = vHMIXED;
      if (nrows < 2) {
        // Single entry inputbox.
        entry = newCDKEntry(dialog, LEFT, TOP, (char *)title, (char *)info_text,
                            A_NORMAL, '_', display, 0, 0, 100, FALSE, FALSE);
        bindCDKObject(vENTRY, entry, KEY_TAB, buttonbox_tab, buttonbox);
        bindCDKObject(vENTRY, entry, KEY_BTAB, buttonbox_tab, buttonbox);
        if (text) setCDKEntryValue(entry, (char *)text);
      } else {
        // Multiple entry inputbox.
        for (i = 0; i < nrows; i++) {
          entries[i] = newCDKEntry(dialog, LEFT, (i == 0) ? TOP : i + 3,
                                   (i == 0) ? (char *)title : NULL,
                                   (char *)info_texts[i], A_NORMAL, '_',
                                   display, 0, 0, 100, FALSE, FALSE);
          BINDFN function = (i < nrows - 1) ? entries_tab : buttonbox_tab;
          void *data = (i < nrows - 1) ? entries : (void *)buttonbox;
          bindCDKObject(vENTRY, entries[i], KEY_TAB, function, data);
          bindCDKObject(vENTRY, entries[i], KEY_DOWN, function, data);
          bindCDKObject(vENTRY, entries[i], KEY_BTAB, entries_tab, entries);
          bindCDKObject(vENTRY, entries[i], KEY_UP, entries_tab, entries);
          if (i < len) setCDKEntryValue(entries[i], (char *)texts[i]);
        }
        entries[nrows] = NULL;
      }
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
      if (font) gtk_widget_modify_font(textview, font);
      GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
      if (text) gtk_text_buffer_set_text(buffer, text, strlen(text));
#elif CURSES
      EDisplayType display = editable ? vVIEWONLY : vMIXED;
      textview = newCDKMentry(dialog, LEFT, TOP, (char *)title,
                              (char *)info_text, A_NORMAL, '_', display, 0,
                              height - 8, height - 8, 0, FALSE, FALSE);
      if (text) setCDKMentryValue(textview, (char *)text);
#endif
      if (text_file) {
        FILE *f = fopen(text_file, "r");
        if (f) {
          fseek(f, 0, SEEK_END);
          int len = ftell(f);
          char *buf = malloc(len + 1);
          rewind(f), buf[fread(buf, 1, len, f)] = '\0';
#if GTK
          gtk_text_buffer_set_text(buffer, buf, len);
#elif CURSES
          setCDKMentryValue(textview, buf);
#endif
          free(buf);
          fclose(f);
        }
      }
#if GTK
      if (strcmp(scroll_to, "bottom") == 0) {
        GtkTextView *view = GTK_TEXT_VIEW(textview);
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
        gtk_text_view_scroll_mark_onscreen(view,
                                           gtk_text_buffer_get_insert(buffer));
      } else g_signal_emit_by_name(G_OBJECT(textview), "move-cursor",
                                   GTK_MOVEMENT_BUFFER_ENDS, -1, 0);
      if (selected)
        g_signal_emit_by_name(G_OBJECT(textview), "select-all", TRUE);
#elif CURSES
      if (strcmp(scroll_to, "top") == 0)
        injectCDKMentry(textview, KEY_HOME);
      else
        injectCDKMentry(textview, KEY_END);
#endif
    } else if (type == GTDIALOG_PROGRESSBAR) {
#if GTK
      progressbar = gtk_progress_bar_new();
      gtk_box_pack_start(GTK_BOX(vbox), progressbar, FALSE, TRUE, 5);
      if (!indeterminate && percent)
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressbar),
                                      0.01 * percent);
      else if (indeterminate)
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
      if (text) gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressbar), text);
#elif CURSES
#endif
    } else if (type == GTDIALOG_DROPDOWN ||
               type == GTDIALOG_STANDARD_DROPDOWN) {
#if GTK
      combobox = gtk_combo_box_new_text();
      gtk_box_pack_start(GTK_BOX(vbox), combobox, FALSE, TRUE, 5);
      if (exit_onchange)
        g_signal_connect(G_OBJECT(combobox), "changed",
                         G_CALLBACK(close_dropdown), (gpointer)dialog);
      for (i = 0; i < len; i++)
        gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), items[i]);
      gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), select);
#elif CURSES
      combobox = newCDKItemlist(dialog, LEFT, TOP, (char *)title,
                                (char *)info_text, (char **)items, len, 0,
                                FALSE, FALSE);
#endif
    } else if (type == GTDIALOG_FILTEREDLIST) {
      if (ncols == 0) return copy("Error: --columns not given.\n");
#if GTK
      entry = gtk_entry_new();
      gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE);
      gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 5);
      GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
      gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
      treeview = gtk_tree_view_new();
      gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), TRUE);
      g_signal_connect(G_OBJECT(treeview), "key-press-event",
                       G_CALLBACK(list_keypress), (gpointer)dialog);
      g_signal_connect(G_OBJECT(treeview), "row-activated",
                       G_CALLBACK(list_select), (gpointer)dialog);
      for (i = 0; i < ncols; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *treecol = NULL;
        treecol = gtk_tree_view_column_new_with_attributes(cols[i], renderer,
                                                           "text", i, NULL);
        gtk_tree_view_column_set_sizing(treecol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), treecol);
      }
      GType *cols = g_new0(GType, ncols);
      for (i = 0; i < ncols; i++) cols[i] = G_TYPE_STRING;
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
      if (select_multiple)
        gtk_tree_selection_set_mode(
          gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)),
                                      GTK_SELECTION_MULTIPLE);
      if (text) gtk_entry_set_text(GTK_ENTRY(entry), text);
      int col = 0;
      GtkTreeIter iter;
      for (i = 0; i < len; i++) {
        if (col == 0) gtk_list_store_append(list, &iter);
        gtk_list_store_set(list, &iter, col++, items[i], -1);
        if (col == ncols) col = 0; // new row
      }
#elif CURSES
      entry = newCDKEntry(dialog, LEFT, TOP, (char *)title, (char *)info_text,
                          A_NORMAL, '_', vMIXED, 0, 0, 100, FALSE, FALSE);
      char **rows = item_rows((char **)cols, ncols, (char **)items, len);
      int num_rows = (len + ncols - 1) / ncols; // account for non-full rows
      scrolled = newCDKScroll(dialog, LEFT, CENTER, RIGHT, -6, 0, rows[-1],
                              rows, num_rows, FALSE, A_REVERSE, TRUE, FALSE);
      model.rows = rows, model.num_rows = num_rows;
      model.filtered_rows = malloc(sizeof(char *) * num_rows);
      for (i = 0; i < num_rows; i++) model.filtered_rows[i] = model.rows[i];
      model.entry = entry, model.scrolled = scrolled;
      bindCDKObject(vENTRY, entry, KEY_TAB, buttonbox_tab, buttonbox);
      bindCDKObject(vENTRY, entry, KEY_BTAB, buttonbox_tab, buttonbox);
      bindCDKObject(vENTRY, entry, KEY_UP, scrolled_key, scrolled);
      bindCDKObject(vENTRY, entry, KEY_DOWN, scrolled_key, scrolled);
      bindCDKObject(vENTRY, entry, KEY_PPAGE, scrolled_key, scrolled);
      bindCDKObject(vENTRY, entry, KEY_NPAGE, scrolled_key, scrolled);
      setCDKEntryPostProcess(entry, entry_keypress, &model);
      // TODO: commands to scroll the list to the right and left.
      if (text) setCDKEntryValue(entry, (char *)text);
#endif
    } else if (type == GTDIALOG_OPTIONSELECT) {
#if GTK
      GtkWidget *table;
      table = gtk_table_new(len, 1, FALSE); // macro in GTK 3.4
      gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 5);
      for (i = 0; i < len; i++) {
        options[i] = gtk_check_button_new_with_mnemonic(items[i]);
        attach(options[i], 0, 1, i, i + 1, FILL(SHRINK), FILL(SHRINK), 5, 0);
      }
#elif CURSES
      const char *choices[] = {"[ ]", "[x]"};
      options = newCDKSelection(dialog, LEFT, TOP, NONE, height - 5, 0,
                                (char *)info_text, (char **)items, len,
                                (char **)choices, 2, A_REVERSE, FALSE, FALSE);
#endif
      for (i = 0; i < select; i++) {
        int j = atoi(selects[i]);
        if (j >= 0 && j < len)
#if GTK
          gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(options[j]), TRUE);
#elif CURSES
          setCDKSelectionChoice(options, j, 1);
#endif
      }
    }
  } else {
    if (type == GTDIALOG_FILESELECT) {
#if GTK
      dialog = gtk_file_chooser_dialog_new(title, parent,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           GTK_STOCK_CANCEL,
                                           GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN,
                                           GTK_RESPONSE_ACCEPT, NULL);
      if (select_only_dirs)
          gtk_file_chooser_set_action(GTK_FILE_CHOOSER(dialog),
                                      GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
#elif CURSES
      dialog = initCDKScreen(newwin(height, width, 1, 1));
#if (LIBRARY && !_WIN32)
      tcsetattr(0, TCSANOW, &term); // restore initial terminal settings
#endif
      fileselect = newCDKFselect(dialog, LEFT, TOP, height, width,
                                 (char *)title, (char *)text, A_NORMAL, '_',
                                 A_REVERSE, "</B>", "</N>", "</N>", "</N>",
                                 TRUE, FALSE);
#endif
    } else {
#if GTK
      dialog = gtk_file_chooser_dialog_new(title, parent,
                                           GTK_FILE_CHOOSER_ACTION_SAVE,
                                           GTK_STOCK_CANCEL,
                                           GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE,
                                           GTK_RESPONSE_ACCEPT, NULL);
      gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog),
                                                     TRUE);
      if (no_create_dirs)
        gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), FALSE);
#elif CURSES
      dialog = initCDKScreen(newwin(height, width, 1, 1));
#if (LIBRARY && !_WIN32)
      tcsetattr(0, TCSANOW, &term); // restore initial terminal settings
#endif
      fileselect = newCDKFselect(dialog, LEFT, TOP, height, width,
                                 (char *)title, (char *)text, A_NORMAL, '_',
                                 A_REVERSE, "</B>", "</N>", "</N>", "</N>",
                                 TRUE, FALSE);
#endif
    }
#if GTK
    gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog),
                                         select_multiple);
    if (with_dir)
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), with_dir);
    if (filter) gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    if (with_dir && with_file && type == GTDIALOG_FILESELECT) {
      char *path = g_strconcat(with_dir, G_DIR_SEPARATOR_S, with_file, NULL);
      gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(dialog), path);
      g_free(path);
    } else if (with_file && type == GTDIALOG_FILESAVE)
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), with_file);
#elif CURSES
    if (with_dir) setCDKFselectDirectory(fileselect, (char *)with_dir);
    if (with_file) {
      char *dir = dirName((char *)with_file);
      setCDKFselectDirectory(fileselect, (char *)dir);
      // TODO: select file in the list.
      free(dir);
    }
#endif
  }
#if GTK
  gtk_window_set_wmclass(GTK_WINDOW(dialog), "gtdialog", "gtdialog");
#endif

  // Run dialog, storing output in 'out'.
  char *out = NULL;
  if (type != GTDIALOG_FILESELECT && type != GTDIALOG_FILESAVE &&
      type != GTDIALOG_PROGRESSBAR) {
#if GTK
    gtk_widget_show_all(dialog);
    if (timeout_len)
      g_timeout_add_seconds(timeout_len, timeout_dialog, (gpointer)dialog);
    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_DELETE_EVENT) response = RESPONSE_DELETE;
#elif CURSES
    WINDOW *border = newwin(height, width, 1, 1);
    box(border, 0, 0), wrefresh(border);
    refreshCDKScreen(dialog);
    int response;
    if (type >= GTDIALOG_INPUTBOX &&
        type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
      if (nrows > 1) {
        // Handle cycling through the multiple entries.
        activateCDKEntry(focused_entry = entries[0], NULL);
        while (focused_entry->exitType == vNORMAL ||
               focused_entry->exitType == vNEVER_ACTIVATED) {
          if (focused_entry->exitType == vNORMAL &&
              focused_entry == entries[nrows - 1]) break; // ENTER in last entry
          for (i = 0; i < nrows; i++) entries[i]->exitType = vNEVER_ACTIVATED;
          activateCDKEntry(focused_entry, NULL);
        }
        entry = focused_entry;
      } else activateCDKEntry(entry, NULL);
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
    } else if (type == GTDIALOG_OPTIONSELECT) {
      activateCDKSelection(options, NULL);
      response = (options->exitType == vNORMAL) ? 1 + buttonbox->currentButton
                                                : RESPONSE_DELETE;
    } else {
      response = 1 + activateCDKButtonbox(buttonbox, NULL);
      // activateCDKButtonbox returns -1 on escape so check for response == 0.
      if (response == 0) response = RESPONSE_DELETE;
    }
    wborder(border, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '), wrefresh(border);
    delwin(border);
    destroyCDKButtonbox(buttonbox);
#endif
    if (string_output && response > 0 && response <= 3)
      out = copy(buttons[response - 1]);
    else
      out = malloc(3), sprintf(out, "%i", response);
    if (type <= GTDIALOG_YESNO_MSGBOX) {
#if CURSES
      if (text) destroyCDKLabel(labelt);
      if (info_text) destroyCDKLabel(labeli);
#endif
    } else if (type >= GTDIALOG_INPUTBOX && type != GTDIALOG_FILESELECT &&
               type != GTDIALOG_FILESAVE && type != GTDIALOG_PROGRESSBAR) {
      if (response > RESPONSE_TIMEOUT) {
        char *txt = "";
        int created = FALSE;
        if (type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
#if GTK
          if (nrows > 1) {
            // Combine multiple entries into a '\n' separated string.
            GString *gstr = g_string_new("");
            for (i = 0; i < nrows; i++) {
              g_string_append(gstr, gtk_entry_get_text(GTK_ENTRY(entries[i])));
              g_string_append_c(gstr, '\n');
            }
            txt = copy(gstr->str), created = TRUE;
            if (strlen(txt) > 0) txt[strlen(txt) - 1] = '\0'; // chomp '\n'
            g_string_free(gstr, TRUE);
          } else txt = (char *)gtk_entry_get_text(GTK_ENTRY(entry));
#elif CURSES
          if (nrows > 1) {
            // Combine multiple entries into a '\n' separated string.
            int len = 1;
            for (i = 0; i < nrows; i++)
              len += strlen(getCDKEntryValue(entries[i])) + 1;
            txt = malloc(len), created = TRUE;
            char *p = txt;
            for (i = 0; i < nrows; i++)
              p = stpcpy(p, getCDKEntryValue(entries[i])), *p++ = '\n';
            if (p - txt > 0) *p = '\0'; // chomp '\n'
          } else txt = copy(getCDKEntryValue(entry)), created = TRUE;
#endif
        } else if (type == GTDIALOG_TEXTBOX && editable) {
#if GTK
          GtkTextView *view = GTK_TEXT_VIEW(textview);
          GtkTextBuffer *buffer = gtk_text_view_get_buffer(view);
          GtkTextIter s, e;
          gtk_text_buffer_get_start_iter(buffer, &s);
          gtk_text_buffer_get_end_iter(buffer, &e);
          txt = gtk_text_buffer_get_text(buffer, &s, &e, TRUE);
          if (font) {
            gtk_widget_modify_font(textview, NULL);
            pango_font_description_free(font);
          }
#elif CURSES
          txt = copy(getCDKMentryValue(textview)), created = TRUE;
#endif
        } else if (type == GTDIALOG_DROPDOWN ||
                   type == GTDIALOG_STANDARD_DROPDOWN) {
          if (string_output) {
#if GTK
            txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(combobox));
#elif CURSES
            if (len > 0)
              txt = (char *)items[getCDKItemlistCurrentItem(combobox)];
#endif
          } else {
            txt = malloc(4), created = TRUE;
            sprintf(txt, "%i",
#if GTK
                    gtk_combo_box_get_active(GTK_COMBO_BOX(combobox)));
#elif CURSES
                    getCDKItemlistCurrentItem(combobox));
#endif
          }
        } else if (type == GTDIALOG_FILTEREDLIST) {
#if GTK
          GString *gstr = g_string_new("");
          gtk_tree_selection_selected_foreach(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), list_foreach,
                                        gstr);
          txt = copy(gstr->str), created = TRUE;
          if (strlen(txt) > 0) txt[strlen(txt) - 1] = '\0'; // chomp '\n'
          g_string_free(gstr, TRUE);
#elif CURSES
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
            } else txt = malloc(4), sprintf(txt, "%i", i), created = TRUE;
          }
#endif
        } else if (type == GTDIALOG_OPTIONSELECT) {
#if GTK
          GString *gstr = g_string_new("");
          for (i = 0; i < len; i++) {
            GtkWidget *opt = options[i];
            if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(opt))) continue;
            if (string_output) {
              g_string_append(gstr, gtk_button_get_label(GTK_BUTTON(opt)));
              g_string_append_c(gstr, '\n');
            } else g_string_append_printf(gstr, "%i\n", i);
          }
          txt = copy(gstr->str), created = TRUE;
          if (strlen(txt) > 0) txt[strlen(txt) - 1] = '\0'; // chomp '\n'
          g_string_free(gstr, TRUE);
#elif CURSES
          int txt_len = 0;
          for (i = 0; i < len; i++)
            if (options->selections[i]) txt_len += strlen(items[i]) + 1;
          if (txt_len > 0) {
            txt = malloc(txt_len + 1), created = TRUE;
            char *p = txt;
            for (i = 0; i < len; i++)
              if (options->selections[i]) {
                if (string_output)
                  p = stpcpy(p, items[i]), *p++ = '\n';
                else
                  p += sprintf(p, "%i\n", i);
              }
            if (p > txt) *(p - 1) = '\0'; // chomp '\n'
          }
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
      GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
      if (type == GTDIALOG_FILESELECT &&
          gtk_file_chooser_get_select_multiple(chooser)) {
        out = copy("");
        GSList *filenames = gtk_file_chooser_get_filenames(chooser), *i = NULL;
        for (i = filenames; i; i = i->next) {
          char *new_out = g_strconcat(out, "\n", (char *)i->data, NULL);
          free(out), g_free(i->data);
          out = new_out;
        }
        g_slist_free(filenames);
      } else out = copy(gtk_file_chooser_get_filename(chooser));
    } else out = copy("");
#elif CURSES
    const char *charset = NULL;
#if (CURSES && !_WIN32)
    charset = getenv("CHARSET");
    if (!charset || !*charset) {
      char *locale = getenv("LC_ALL");
      if (!locale || !*locale) locale = getenv("LANG");
      if (locale && (charset = strchr(locale, '.'))) charset++;
    }
#elif (CURSES && WIN32)
    char codepage[8];
    sprintf(codepage, "CP%d", GetACP()), charset = codepage;
#endif
    char *txt = activateCDKFselect(fileselect, NULL);
    out = txt ? copy(txt) : copy("");
    destroyCDKFselect(fileselect);
    if (chdir(cwd)); // 'if' prevents compiler warning
#endif
  } else if (type == GTDIALOG_PROGRESSBAR) {
#if GTK
    gtk_widget_show_all(GTK_WIDGET(dialog));
#if !_WIN32
    GIOChannel *ch = g_io_channel_unix_new(0);
#else
    GIOChannel *ch = g_io_channel_win32_new_fd(0); // TODO: test
#endif
    g_io_channel_set_encoding(ch, NULL, NULL);
    int source = g_io_add_watch(ch, G_IO_IN | G_IO_HUP, read_stdin, dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != 1)
      out = copy("");
    else
      out = copy("stopped");
    g_source_remove(source), g_io_channel_unref(ch), g_io_channel_unref(ch);
#endif
  }
  if (strcmp(out, "0") == 0 && string_output)
    out = copy("timeout");
  else if (strcmp(out, "-1") == 0 && string_output)
    out = copy("delete");
#if GTK
  gtk_widget_destroy(dialog);
#elif CURSES
  if (type <= GTDIALOG_SECURE_STANDARD_INPUTBOX) {
    if (nrows < 2)
      destroyCDKEntry(entry);
    else
      for (i = 0; i < nrows; i++) destroyCDKEntry(entries[i]);
  } else if (type == GTDIALOG_TEXTBOX)
    destroyCDKMentry(textview);
  else if (type == GTDIALOG_DROPDOWN || type == GTDIALOG_STANDARD_DROPDOWN)
    destroyCDKItemlist(combobox);
  else if (type == GTDIALOG_FILTEREDLIST) {
    destroyCDKEntry(entry), destroyCDKScroll(scrolled);
    if (model.rows) {
      for (i = -1; i < model.num_rows; i++) free(model.rows[i]);
      free(&model.rows[-1]);
    }
    if (model.filtered_rows) free(model.filtered_rows);
  } else if (type == GTDIALOG_OPTIONSELECT)
    destroyCDKSelection(options);
  delwin(dialog->window), destroyCDKScreen(dialog);
  curs_set(cursor); // restore cursor
  timeout(0), getch(), timeout(-1); // flush input
#endif
  if (!no_newline) {
    char *new_out = malloc(strlen(out) + 2);
    sprintf(new_out, "%s\n", out);
    free(out);
    out = new_out;
  }
  return out;
}

// Help on dialog types.
#define HELP_MSGBOX \
"gtdialog msgbox [args]\n" \
"  A generic message box with custom button labels.\n" \
"gtdialog ok-msgbox [args]\n" \
"  Identical to msgbox, but with localized “Ok” and “Cancel” buttons.\n" \
"gtdialog yesno-msgbox [args]\n" \
"  Identical to msgbox, but with localized “Yes”, “No”, and “Cancel” buttons.\n"
#define HELP_INPUTBOX \
"gtdialog inputbox [args]\n" \
"  A one line input box with custom button labels.\n" \
"  An input box may have multiple entry boxes.\n" \
"gtdialog standard-inputbox [args]\n" \
"  Identical to inputbox, but with localized “Ok” and “Cancel” buttons.\n" \
"gtdialog secure-inputbox [args]\n" \
"  Identical to inputbox, but input is masked.\n" \
"gtdialog secure-standard-inputbox [args]\n" \
"  Identical to standard-inputbox, but input is masked.\n"
#define HELP_FILE \
"gtdialog fileselect [args]\n" \
"  A file selection dialog for opening files.\n" \
"gtdialog filesave [args]\n" \
"  A file selection dialog for saving a file.\n"
#define HELP_TEXTBOX \
"gtdialog textbox [args]\n" \
"  A multiple line text box with custom button labels.\n"
#define HELP_PROGRESSBAR \
"gtdialog progressbar [args]\n" \
"  A progressbar dialog with updates from stdin.\n"
#define HELP_DROPDOWN \
"gtdialog dropdown [args]\n" \
"  A drop down list of items to select from with custom button labels.\n" \
"gtdialog standard-dropdown [args]\n" \
"  Identical to drop down, but with localized “Ok” and “Cancel” buttons.\n"
#define HELP_FILTEREDLIST \
"gtdialog filteredlist [args]\n" \
"  A list of items to filter through and select from with custom button\n" \
"  labels.\n" \
"  Spaces in the filter text are treated as wildcards.\n"
#define HELP_OPTIONSELECT \
"gtdialog optionselect [args]\n" \
"  A group of options to select from.\n"
#define HELP_ALL \
"Usage: gtdialog type [args]\n" \
"\n" \
HELP_MSGBOX HELP_INPUTBOX HELP_FILE HELP_TEXTBOX HELP_PROGRESSBAR \
HELP_DROPDOWN HELP_FILTEREDLIST HELP_OPTIONSELECT \
"\n" \
"gtdialog help type\n" \
"   Shows detailed documentation on gtdialog type\n"

// Help on dialog arguments.
#define HELP_DEFAULT_ARGS \
"  --title str\n" \
"      The dialog's title text.\n" \
"  --string-output\n" \
"      Output the names of selected buttons/items or exit codes instead of\n" \
"      button/item indexes or exit code numbers.\n" \
"  --no-newline\n" \
"      Do not output the default trailing newline.\n" \
"  --width int\n" \
"      Manually set the width of the dialog in pixels if possible.\n" \
"  --height int\n" \
"      Manually set the height of the dialog in pixels if possible.\n"
#define HELP_TEXT_MAIN \
"  --text str\n" \
"      The main message text.\n"
#define HELP_INFORMATIVE_TEXT_EXTRA \
"  --informative-text str\n" \
"      Extra informative text.\n"
#define HELP_ICON \
"  --icon str\n" \
"      The name of the GTK stock icon to display. No icon is displayed\n" \
"      by default.\n" \
"      Examples are “gtk-dialog-error”, “gtk-dialog-info”,\n" \
"      “gtk-dialog-question”, and “gtk-dialog-warning”.\n"
#define HELP_ICON_FILE \
"  --icon-file str\n" \
"      The path to the icon to display.\n" \
"      Has no effect when --icon is present.\n"
#define HELP_BUTTON1_MSGBOX \
"  --button1 str\n" \
"      The right-most button's label. The default is “Ok” for ok-msgbox and\n" \
"      “Yes” for yesno-msgbox.\n"
#define HELP_BUTTON2_MSGBOX \
"  --button2 str\n" \
"      The middle button's label. The default is “Cancel” for ok-msgbox and\n" \
"      “No” for yesno-msgbox.\n"
#define HELP_BUTTON3_MSGBOX \
"  --button3 str\n" \
"      The left-most button's label. The default is “Cancel” for\n" \
"      yesno-msgbox.\n" \
"      Requires --button2.\n"
#define HELP_NO_CANCEL_MSGBOX \
"  --no-cancel\n" \
"      Only show “Ok” button for ok-msgbox and yesno-msgbox.\n"
#define HELP_FLOAT \
"  --float\n" \
"      Show the dialog on top of all windows.\n"
#define HELP_TIMEOUT \
"  --timeout int\n" \
"      The number of seconds the dialog waits for a button click before\n" \
"      timing out. Dialogs do not time out by default.\n"
#define HELP_INFORMATIVE_TEXT_INPUTBOX \
"  --informative-text str [labels]\n" \
"      The main message text.\n" \
"      Create multiple, labeled entry boxes by specifying one label for\n" \
"      each box. Each label must be a separate argument. Providing a single\n" \
"      label has no effect.\n"
#define HELP_TEXT_INPUTBOX \
"  --text str|list\n" \
"      The initial input text.\n" \
"      Fill multiple entry boxes in order using a list of input strings.\n" \
"      Each string must be a separate argument. Use '' for blanks.\n" \
"      Requires more than one label in --informative-text.\n"
#define HELP_NO_SHOW \
"  --no-show\n" \
"      Mask the user input by showing typed characters as password\n" \
"      characters.\n"
#define HELP_BUTTON1_INPUTBOX \
"  --button1 str\n" \
"      The right-most button's label. The default is “Ok” for\n" \
"      standard-inputbox.\n"
#define HELP_BUTTON2_INPUTBOX \
"  --button2 str\n" \
"      The middle button's label. The default is “Cancel” for\n" \
"      standard-inputbox.\n"
#define HELP_BUTTON3_INPUTBOX \
"  --button3 str\n" \
"      The left-most button's label.\n" \
"      Requires --button2.\n"
#define HELP_NO_CANCEL_INPUTBOX \
"  --no-cancel\n" \
"      Only show “Ok” button for standard-inputbox.\n"
#define HELP_WITH_DIRECTORY \
"  --with-directory str\n" \
"      The initial directory. The system determines the default directory.\n"
#define HELP_WITH_FILE \
"  --with-file str\n" \
"      The initially selected filename. The first filename in the list is\n" \
"      selected by default.\n" \
"      Requires --with-directory.\n"
#define HELP_WITH_EXTENSION \
"  --with-extension list\n" \
"      The set of extensions to limit selectable files to. Each extension\n" \
"      must be a separate argument with the ‘.’ being optional.\n"
#define HELP_SELECT_MULTIPLE_FILESELECT \
"  --select-multiple\n" \
"      Enable multiple file selection in fileselect.\n"
#define HELP_SELECT_ONLY_DIRECTORIES \
"  --select-only-directories\n" \
"      Prompt for directory selection in fileselect.\n"
#define HELP_NO_CREATE_DIRECTORIES \
"  --no-create-directories\n" \
"      Prevent the user from creating new directories in filesave.\n"
#define HELP_INFORMATIVE_TEXT_TEXTBOX \
"  --informative-text str\n" \
"      Informative message text.\n"
#define HELP_TEXT_TEXTBOX \
"  --text str\n" \
"      The initial text in the textbox.\n"
#define HELP_TEXT_FROM_FILE \
"  --text-from-file str\n" \
"      The filename whose contents are loaded into the textbox.\n" \
"      Has no effect when --text is present.\n"
#define HELP_BUTTON1 \
"  --button1 str\n" \
"      The right-most button's label.\n"
#define HELP_BUTTON2 \
"  --button2 str\n" \
"      The middle button's label.\n"
#define HELP_BUTTON3 \
"  --button3 str\n" \
"      The left-most button's label.\n" \
"      Requires --button2.\n"
#define HELP_EDITABLE \
"  --editable\n" \
"      Allow textbox editing.\n"
#define HELP_FOCUS_TEXTBOX \
"  --focus-textbox\n" \
"      Focus on the textbox instead of the dialog buttons.\n"
#define HELP_SCROLL_TO \
"  --scroll-to bottom|top\n" \
"      Scroll to the “bottom” or “top” of the textbox when not all text is\n" \
"      visible. The default is “top”.\n"
#define HELP_SELECTED \
"  --selected\n" \
"      Select all textbox text.\n"
#define HELP_MONOSPACED_FONT \
"  --monospaced-font\n" \
"      Use a monospaced font instead of a proportional one.\n"
#define HELP_PERCENT \
"  --percent int\n" \
"      The initial progressbar percentage between 0 and 100.\n"
#define HELP_TEXT_PROGRESSBAR \
"  --text str\n" \
"      The initial progressbar display text.\n"
#define HELP_INDETERMINATE \
"  --indeterminate\n" \
"      Show the progressbar as “busy” with no percentage updates.\n"
#define HELP_STOPPABLE \
"  --stoppable\n" \
"      Show the Stop button.\n"
#define HELP_ITEMS_DROPDOWN \
"  --items list\n" \
"      The list of items to show in the drop down. Each item must be a\n" \
"      separate argument.\n"
#define HELP_NO_CANCEL_DROPDOWN \
"  --no-cancel\n" \
"      Only show “Ok” button for standard-dropdown.\n"
#define HELP_EXIT_ONCHANGE \
"  --exit-onchange\n" \
"      Selecting a new item closes the dialog.\n"
#define HELP_SELECT \
"  --select int\n" \
"      The zero-based index of the item in the list to select. The first\n" \
"      item in the list is selected by default.\n"
#define HELP_INFORMATIVE_TEXT_FILTEREDLIST \
"  --informative-text str\n" \
"      The main message text.\n"
#define HELP_TEXT_FILTEREDLIST \
"  --text str\n" \
"      The initial input text.\n"
#define HELP_COLUMNS \
"  --columns list\n" \
"      The column names for a list row. Each name must be a separate\n" \
"      argument.\n"
#define HELP_ITEMS_FILTEREDLIST \
"  --items list\n" \
"      The items to show in the list. Each item must be a separate argument\n" \
"      and is inserted into the first empty column in the current list row.\n" \
"      Requires --columns.\n"
#define HELP_SELECT_MULTIPLE_FILTEREDLIST \
"  --select-multiple\n" \
"      Enable multiple item selection.\n"
#define HELP_SEARCH_COLUMN \
"  --search-column int\n" \
"      The column number to filter the input text against. The default is\n" \
"      1.\n" \
"      Requires --columns.\n"
#define HELP_OUTPUT_COLUMN \
"  --output-column int\n" \
"      The column number to use for --string-output. The default is 1.\n"
#define HELP_INFORMATIVE_TEXT_OPTIONSELECT \
"  --informative-text str\n" \
"      The main message text.\n"
#define HELP_ITEMS_OPTIONSELECT \
"  --items list\n" \
"      The options to show in the option group. Each item must be a\n" \
"      separate argument.\n"
#define HELP_SELECT_OPTIONSELECT \
"  --select indices\n" \
"      The zero-based indices of the options in the option group to select.\n" \
"      Each index must be a separate argument.\n"

// Help on dialog returns.
#define HELP_MSGBOX_RETURN \
"The message box dialogs return a string containing the number of the\n" \
"button pressed, ‘0’ if the dialog timed out, or “-1” if the user canceled\n" \
"the dialog. If --string-output was given, the return string contains the\n" \
"label of the button pressed, “timeout” if the dialog timed out, or\n" \
"“delete” if the user canceled the dialog.\n"
#define HELP_LOCALIZED_BUTTONS \
"\nFor GTK only, button labels with GTK stock item labels are automatically\n" \
"localized. However, when --string-output is given, the stock item label is\n" \
"returned, not the actual, localized label. The default button labels are\n" \
"all stock item labels (“gtk-ok”, “gtk-cancel”, “gtk-yes”, and “gtk-no”).\n"
#define HELP_INPUTBOX_RETURN \
"The input dialogs return a string containing the number of the button\n" \
"pressed followed by a newline character (‘\\n’) and the input text\n" \
"(with any multiple entries separated by newline characters), ‘0’ if the\n" \
"dialog timed out, or “-1” if the user canceled the dialog. If\n" \
"--string-output was given, the return string contains the label of the\n" \
"button pressed followed by a newline and the input text, “timeout” if the\n" \
"dialog timed out, or “delete” if the user canceled the dialog.\n"
#define HELP_FILE_RETURN \
"The file dialogs return a string containing the file(s) selected or the\n" \
"empty string if the user canceled the dialog.\n"
#define HELP_TEXTBOX_RETURN \
"The textbox dialog returns a string containing the number of the button\n" \
"pressed and, if --editable was given, a newline character (‘\\n’) followed\n" \
"by the textbox text; otherwise ‘0’ if the dialog timed out or “-1” if the\n" \
"user canceled the dialog. If --string-output was given, the return string\n" \
"contains the label of the button pressed and, if --editable was given, a\n" \
"newline followed by the textbox text; otherwise “timeout” if the dialog\n" \
"timed out or “delete” if the user canceled the dialog.\n"
#define HELP_PROGRESSBAR_RETURN \
"The progressbar dialog reads lines from standard input (stdin) and updates\n" \
"the progressbar until the dialog receives an EOF. Input lines are of the\n" \
"form “num str\\n” where “num” is a progress percentage between 0 and 100\n" \
"and “str” is optional progress display text. The newline character (‘\\n’)\n" \
"is required. If “str” is empty, the current progress display text is\n" \
"retained. If --stoppable is given and “str” is either “stop disable” or\n" \
"“stop enable”, the Stop button is disabled or enabled, respectively.\n" \
"The dialog returns the string “stopped” only if --stopped was given and\n" \
"the Stop button was pressed. Otherwise it returns nothing.\n"
#define HELP_DROPDOWN_RETURN \
"The dropdown dialogs return a string containing the number of the button\n" \
"pressed (or ‘4’ if --exit-onchange was responsible) followed by a newline\n" \
"character (‘\\n’) and the index of the selected item starting from 0, ‘0’\n" \
"if the dialog timed out, or “-1” if the user canceled the dialog. If\n" \
"--string-output was given, the return string contains the label of the\n" \
"button pressed (or ‘4’, yes ‘4’, if --exit-onchange was responsible)\n" \
"followed by a newline and the selected item, “timeout” if the dialog timed\n" \
"out, or “delete” if the user canceled the dialog.\n"
#define HELP_FILTEREDLIST_RETURN \
"The filteredlist dialog returns a string containing the number of the\n" \
"button pressed followed by a newline character (‘\\n’) and the index of\n" \
"the selected item(s) starting from 0, ‘0’ if the dialog timed out, or “-1”\n" \
"if the user canceled the dialog. If --string-output was given, the return\n" \
"string contains the label of the button pressed followed by a newline and\n" \
"the selected item(s) (based on --output-column, if applicable), “timeout”\n" \
"if the dialog timed out, or “delete” if the user canceled the dialog.\n"
#define HELP_OPTIONSELECT_RETURN \
"The optionselect dialog returns a string containing the number of the\n" \
"button pressed followed by a newline character (‘\\n’) and the index of\n" \
"the selected options starting from 0, ‘0’ if the dialog timed out, or “-1”\n" \
"if the user canceled the dialog. If --string-output was given, the return\n" \
"string contains the label of the button pressed followed by a newline and\n" \
"the selected option(s), “timeout” if the dialog timed out, or “delete” if\n" \
"the user canceled the dialog.\n"

// Help with dialog examples.
#define HELP_MSGBOX_EXAMPLE \
"  gtdialog msgbox --title 'EOL Mode' --text 'Which EOL?' \\\n" \
"    --icon gtk-dialog-question \\\n" \
"    --button1 CRLF --button2 CR --button3 LF\n"
#define HELP_INPUTBOX_EXAMPLE \
"  gtdialog standard-inputbox --title 'Goto Line' \\\n" \
"    --informative-text 'Line:' --text 1 --no-newline\n"
#define HELP_FILE_EXAMPLE \
"  gtdialog fileselect --title 'Open C File' --with-directory $HOME \\\n" \
"    --with-extension c h --select-multiple --no-newline\n"
#define HELP_TEXTBOX_EXAMPLE \
"  gtdialog textbox --title 'License Agreement' \\\n" \
"    --informative-text 'You agree to:' --text-from-file LICENSE --button1 Ok\n"
#define HELP_PROGRESSBAR_EXAMPLE \
"  for i in 25 50 75 100; do echo $i $i% done; sleep 1; done | \\\n" \
"    gtdialog progressbar --title 'Status' --width 200 --stoppable\n"
#define HELP_DROPDOWN_EXAMPLE \
"  gtdialog dropdown --title 'Select Encoding' --width 200 \\\n" \
"    --items UTF-8 ASCII ISO-8859-1 MacRoman --no-cancel --string-output \\\n" \
"    --no-newline\n"
#define HELP_FILTEREDLIST_EXAMPLE \
"  gtdialog filteredlist --title Title --columns Foo Bar \\\n" \
"    --items a b c d --no-newline\n"
#define HELP_OPTIONSELECT_EXAMPLE \
"  gtdialog optionselect --title Languages \\\n" \
"    --informative-text 'Check the languages you understand' \\\n" \
"    --items English French German Romanian Russian Spanish Swedish \\\n" \
"    --select 0 2 --string-output --no-newline\n"

// Help template.
#define HELP(type, args, returns, example) \
type "\nArguments:\n" HELP_DEFAULT_ARGS args "\n" returns "\nExample:\n" example

/**
 * Prints help for the gtdialog to the command line.
 * @param argc The number of command line parameters.
 * @param argv The set of command line parameters for the dialog.
 */
int help(int argc, char *argv[]) {
#ifndef NOHELP
  switch ((argc == 3) ? gtdialog_type(argv[2]) : -1) {
  case GTDIALOG_MSGBOX:
  case GTDIALOG_OK_MSGBOX:
  case GTDIALOG_YESNO_MSGBOX:
    puts(HELP(HELP_MSGBOX,
              HELP_TEXT_MAIN
              HELP_INFORMATIVE_TEXT_EXTRA
              HELP_ICON
              HELP_ICON_FILE
              HELP_BUTTON1_MSGBOX
              HELP_BUTTON2_MSGBOX
              HELP_BUTTON3_MSGBOX
              HELP_NO_CANCEL_MSGBOX
              HELP_FLOAT HELP_TIMEOUT,
              HELP_MSGBOX_RETURN
              HELP_LOCALIZED_BUTTONS,
              HELP_MSGBOX_EXAMPLE));
    break;
  case GTDIALOG_INPUTBOX:
  case GTDIALOG_STANDARD_INPUTBOX:
  case GTDIALOG_SECURE_INPUTBOX:
  case GTDIALOG_SECURE_STANDARD_INPUTBOX:
    puts(HELP(HELP_INPUTBOX,
              HELP_INFORMATIVE_TEXT_INPUTBOX
              HELP_TEXT_INPUTBOX
              HELP_NO_SHOW
              HELP_BUTTON1_INPUTBOX
              HELP_BUTTON2_INPUTBOX
              HELP_BUTTON3_INPUTBOX
              HELP_NO_CANCEL_INPUTBOX
              HELP_FLOAT HELP_TIMEOUT,
              HELP_INPUTBOX_RETURN
              HELP_LOCALIZED_BUTTONS,
              HELP_INPUTBOX_EXAMPLE));
    break;
  case GTDIALOG_FILESELECT:
  case GTDIALOG_FILESAVE:
    puts(HELP(HELP_FILE,
              HELP_WITH_DIRECTORY
              HELP_WITH_FILE
              HELP_WITH_EXTENSION
              HELP_SELECT_MULTIPLE_FILESELECT
              HELP_SELECT_ONLY_DIRECTORIES
              HELP_NO_CREATE_DIRECTORIES,
              HELP_FILE_RETURN,
              HELP_FILE_EXAMPLE));
    break;
  case GTDIALOG_TEXTBOX:
    puts(HELP(HELP_TEXTBOX,
              HELP_INFORMATIVE_TEXT_TEXTBOX
              HELP_TEXT_TEXTBOX
              HELP_TEXT_FROM_FILE
              HELP_BUTTON1
              HELP_BUTTON2
              HELP_BUTTON3
              HELP_EDITABLE
              HELP_FOCUS_TEXTBOX
              HELP_SCROLL_TO
              HELP_SELECTED
              HELP_MONOSPACED_FONT
              HELP_FLOAT HELP_TIMEOUT,
              HELP_TEXTBOX_RETURN
              HELP_LOCALIZED_BUTTONS,
              HELP_TEXTBOX_EXAMPLE));
    break;
  case GTDIALOG_PROGRESSBAR:
    puts(HELP(HELP_PROGRESSBAR,
              HELP_PERCENT
              HELP_TEXT_PROGRESSBAR
              HELP_INDETERMINATE
              HELP_STOPPABLE
              HELP_FLOAT,
              HELP_PROGRESSBAR_RETURN,
              HELP_PROGRESSBAR_EXAMPLE));
    break;
  case GTDIALOG_DROPDOWN:
  case GTDIALOG_STANDARD_DROPDOWN:
    puts(HELP(HELP_DROPDOWN,
              HELP_TEXT_MAIN
              HELP_ITEMS_DROPDOWN
              HELP_BUTTON1
              HELP_BUTTON2
              HELP_BUTTON3
              HELP_NO_CANCEL_DROPDOWN
              HELP_EXIT_ONCHANGE
              HELP_SELECT
              HELP_FLOAT HELP_TIMEOUT,
              HELP_DROPDOWN_RETURN
              HELP_LOCALIZED_BUTTONS,
              HELP_DROPDOWN_EXAMPLE));
    break;
  case GTDIALOG_FILTEREDLIST:
    puts(HELP(HELP_FILTEREDLIST,
              HELP_INFORMATIVE_TEXT_FILTEREDLIST
              HELP_TEXT_FILTEREDLIST
              HELP_COLUMNS
              HELP_ITEMS_FILTEREDLIST
              HELP_BUTTON1
              HELP_BUTTON2
              HELP_BUTTON3
              HELP_SELECT_MULTIPLE_FILTEREDLIST
              HELP_SEARCH_COLUMN
              HELP_OUTPUT_COLUMN
              HELP_FLOAT HELP_TIMEOUT,
              HELP_FILTEREDLIST_RETURN
              HELP_LOCALIZED_BUTTONS,
              HELP_FILTEREDLIST_EXAMPLE));
    break;
  case GTDIALOG_OPTIONSELECT:
    puts(HELP(HELP_OPTIONSELECT,
              HELP_INFORMATIVE_TEXT_OPTIONSELECT
              HELP_ITEMS_OPTIONSELECT
              HELP_SELECT_OPTIONSELECT
              HELP_BUTTON1
              HELP_BUTTON2
              HELP_BUTTON3
              HELP_FLOAT HELP_TIMEOUT,
              HELP_OPTIONSELECT_RETURN
              HELP_LOCALIZED_BUTTONS,
              HELP_OPTIONSELECT_EXAMPLE));
    break;
  default:
    puts(HELP_ALL);
  }
#endif
  return 1;
}

#ifndef LIBRARY
/**
 * Runs gtdialog from the command line and prints its output to stdout.
 * @param argc The number of command line parameters.
 * @param argv The set of command line parameters for the dialog.
 */
int main(int argc, char *argv[]) {
  if (argc == 1 || strcmp(argv[1], "help") == 0) return help(argc, argv);
  int type = gtdialog_type(argv[1]);
  if (type < 0) return help(argc, argv);
#if GTK
  gtk_init(&argc, &argv);
#elif CURSES
  initscr();
#endif
  char *out = gtdialog(type, argc - 2, (const char **)&argv[2]);
#if CURSES
  endCDK();
#endif
  puts(out);
  free(out);
  return 0;
}
#endif
