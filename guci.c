/* guci - The Gtk UniCode inspector
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "unicode_names.h"

#define UNICODE_LRO "\342\200\255"
#define VERSION "0.0.1"
#define _(x) x

// Global variables
GtkWidget *w_text, *w_main_window, *w_text_inspector, *w_statusbar;
GtkWidget *w_selected_sub_char = NULL, *w_prev_focus;
gchar *guci_last_directory = NULL;

void create_widgets();
void rc_setup ();
void cb_text_changed (GtkTextBuffer *textbuffer,
		      gpointer user_data);
void cb_mark_set (GtkTextBuffer *textbuffer,
		  GtkTextIter *arg1,
		  GtkTextMark *arg2,
		  gpointer user_data);
void cb_text_insertion (GtkTextBuffer *textbuffer,
			GtkTextIter *arg1,
			gchar *arg2,
			gint arg3,
			gpointer user_data);
void cb_cursor_moved (GtkTextView *textview,
		      GtkMovementStep arg1,
		      gint arg2,
		      gboolean arg3,
		      gpointer user_data);
static void cb_menu_tbd (gpointer   callback_data,
			 guint      callback_action,
			 GtkWidget *widget);
static void cb_menu_quit(gpointer   callback_data,
			 guint      callback_action,
			 GtkWidget *widget);
static void cb_menu_about(gpointer   callback_data,
			  guint      callback_action,
			  GtkWidget *widget);
static void cb_menu_open   (gpointer   callback_data,
			    guint      callback_action,
			    GtkWidget *widget);
static void cb_menu_save   (gpointer   callback_data,
			    guint      callback_action,
			    GtkWidget *widget);
static void cb_menu_new   (gpointer   callback_data,
			    guint      callback_action,
			    GtkWidget *widget);
static void cb_menu_edit_paste(gpointer   callback_data,
			       guint      callback_action,
			       GtkWidget *widget);
static void cb_menu_edit_cut(gpointer   callback_data,
			     guint      callback_action,
			     GtkWidget *widget);
static void cb_menu_edit_copy(gpointer   callback_data,
			     guint      callback_action,
			     GtkWidget *widget);
static void cb_menu_edit_delete(gpointer   callback_data,
				guint      callback_action,
				GtkWidget *widget);
static void cb_menu_edit_select_all(gpointer   callback_data,
				    guint      callback_action,
				    GtkWidget *widget);

static void set_last_directory_from_filename(const gchar *filename);
static void file_to_textview(const gchar *filename,
			     GError **error);
static void textview_to_file(const gchar *filename,
			     GError **error);
static void pack_label_list_in_hbox(GSList *label_list,
				    GtkWidget *hbox);
void guci_error(GtkWidget *parent, const char *fmt, ...);
GtkWidget*guci_button_new_with_stock_image (const gchar* text, const gchar* stock_id);
GtkWidget *guci_dialog_add_button (GtkDialog *dialog, const gchar* text, const gchar* stock_id, gint response_id);
void display_char_name(gunichar ch);

static GtkItemFactoryEntry menu_items[] =
{
  { "/_File",		 NULL,	       0,		      0, "<Branch>" },
  { "/File/_Open",	 "<control>O", cb_menu_open,	0, "<StockItem>", GTK_STOCK_OPEN },
  { "/File/_Save",	 "<control>S", cb_menu_save,	0, "<StockItem>", GTK_STOCK_SAVE },
  { "/File/Save _As",	 "<control><shift>S", cb_menu_save, 0, "<StockItem>", GTK_STOCK_SAVE },
  { "/File/_New",	 NULL, cb_menu_new, 0, "<StockItem>", GTK_STOCK_NEW },
  { "/File/sep1",        NULL,         0,       0, "<Separator>" },
  { "/File/_Quit",	 "<control>Q", cb_menu_quit,	      0, "<StockItem>", GTK_STOCK_QUIT },

  { "/_Edit",		 NULL,	       0,		      0, "<Branch>" },
  { "/Edit/sep1",        NULL,         0,       0, "<Separator>" },
  { "/Edit/Cut",         NULL, cb_menu_edit_cut, 0, "<StockItem>", GTK_STOCK_CUT },
  { "/Edit/Copy",        NULL, cb_menu_edit_copy, 0, "<StockItem>", GTK_STOCK_COPY },
  { "/Edit/Paste",       NULL, cb_menu_edit_paste, 0, "<StockItem>", GTK_STOCK_PASTE },
  { "/Edit/Delete",      NULL, cb_menu_edit_delete, 0, "<StockItem>", GTK_STOCK_DELETE },
  { "/Edit/sep2",        NULL,         0,       0, "<Separator>" },
  { "/Edit/Select All",   "<control>A", cb_menu_edit_select_all, 0, "<CheckItem>" },
  { "/Edit/sep3",        NULL,         0,       0, "<Separator>" },
  { "/Edit/Preferences",   NULL, cb_menu_tbd, 0, "<StockItem>", GTK_STOCK_PREFERENCES },
  { "/_Help",		 NULL,	       0,		      0, "<LastBranch>" },
  { "/Help/_About",	 NULL,	       cb_menu_about,	      0, "<StockItem>", GTK_STOCK_HELP },
};
static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

int main(int argc, char **argv)
{
  GError *error = NULL;
  gchar *filename;
  int argp=1;

  gtk_init (&argc, &argv);
  create_uninames ();
  rc_setup ();
  create_widgets ();

  if (argc > 1)
    {
      filename = argv[argp++];
      file_to_textview(filename,
		       &error);
    }
  
  gtk_main ();
  exit (0);
  return (0);
}

void rc_setup()
{
  gtk_rc_parse_string(
    "style \"inspector\" {\n"
    "  font_name =\"Serif 40\"\n"
    "}\n"
    "style \"selected-char\" {\n"
    "  bg[NORMAL] =\"#c0c0f0\"\n"
    "}\n"
    "widget \"*char_box*\" style \"inspector\""
    "widget \"*selected-char*\" style \"selected-char\""
    );
}

void create_widgets ()
{
  GtkWidget *vbox, *vpaned, *scrolledwin;
  GtkWidget *alignment;
  GtkItemFactory *item_factory;
  GtkTextBuffer *text_buffer;
  GtkAccelGroup *accel_group;

  accel_group = gtk_accel_group_new ();

  // Top window
  w_main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (w_main_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  //  A vbox for the rest of the widgets 
  vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (w_main_window), vbox);

  // menubar bar
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
  gtk_box_pack_start (GTK_BOX (vbox),
		      gtk_item_factory_get_widget (item_factory, "<main>"),
		      FALSE, FALSE, 0);

  // A vertical pane for the two text windows
  vpaned = gtk_vpaned_new();
  gtk_box_pack_start( GTK_BOX (vbox), vpaned, 1,1,0);

  // And finally a statusbar
  w_statusbar = gtk_statusbar_new();
  gtk_box_pack_start( GTK_BOX (vbox), w_statusbar, 0,0,0);
  
  // Fill the vpane with a textview within a scrolledwindow
  scrolledwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_add1 (GTK_PANED (vpaned), scrolledwin);
  w_text = gtk_text_view_new ();
  g_object_set (G_OBJECT (w_text), "wrap-mode", GTK_WRAP_WORD, NULL);
  gtk_widget_set_size_request (w_text, 640, 300);
  gtk_container_add (GTK_CONTAINER (scrolledwin), w_text);

  // setup callback on text insertion
  g_signal_connect_after (G_OBJECT (w_text), "move-cursor",
			 G_CALLBACK (cb_cursor_moved), NULL);
  g_signal_connect_after (G_OBJECT (w_text), "set-anchor",
			 G_CALLBACK (cb_cursor_moved), NULL);
  text_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
  g_signal_connect_after (G_OBJECT (text_buffer), "changed",
			 G_CALLBACK (cb_text_changed), NULL);
  g_signal_connect_after (G_OBJECT (text_buffer), "mark-set",
			 G_CALLBACK (cb_mark_set), NULL);

  // Use an alignment to get the w_text_inspector centered
  alignment = gtk_alignment_new (0.5, 0.5, 0,0);
  gtk_paned_add2 (GTK_PANED (vpaned), alignment);
  
  // Another text view without a scrolled window at the bottom
  w_text_inspector = gtk_hbox_new (0,0);

  gtk_container_add (GTK_CONTAINER (alignment), w_text_inspector);

  // show it all
  gtk_widget_show_all(w_main_window);
}

void cb_text_insertion (GtkTextBuffer *textbuffer,
			GtkTextIter *iter,
			gchar *chars,
			gint len,
			gpointer user_data)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text_inspector));
  GtkTextIter end_iter;

  gtk_text_buffer_get_end_iter (buf,
				&end_iter);

  gtk_text_buffer_insert (buf, &end_iter, chars, len);

  if (len > 0)
    display_char_name(chars[0]);
}

gboolean
cb_label_button_press(GtkWidget *widget,
		      GdkEventButton *event,
		      gpointer user_data)
{
  gunichar ch = GPOINTER_TO_INT (user_data);
  
  if (w_selected_sub_char)
    gtk_widget_set_name (w_selected_sub_char, "normal");

  gtk_widget_set_name (widget, "selected-char");
  w_prev_focus = gtk_window_get_focus (GTK_WINDOW (w_main_window));
  gtk_widget_grab_focus (widget);
  w_selected_sub_char = widget;
  if (ch > 0)
    display_char_name(ch);
    
  return TRUE;
}

gboolean
cb_label_key_press(GtkWidget *widget,
		   GdkEventKey *event,
		   gpointer user_data)
{
  gint k = event->keyval;

  switch(k) {
  case GDK_Escape:
    if (w_selected_sub_char)
      {
	gtk_widget_set_name (w_selected_sub_char, "normal");
	w_selected_sub_char = NULL;
      }
    
    gtk_window_set_focus (GTK_WINDOW (w_main_window),
			    w_prev_focus);
    break;
  case GDK_Delete:
  case GDK_KP_Delete:
    {
      GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
      GtkTextIter cursor_iter, start_iter, end_iter;
      int glyph_idx = GPOINTER_TO_INT(user_data);
      
      // Get cursor position
      gtk_text_buffer_get_iter_at_mark ( buf, &cursor_iter,
					 gtk_text_buffer_get_insert (buf));
      start_iter = end_iter = cursor_iter;

      gtk_text_iter_backward_cursor_position (&start_iter);

      if (glyph_idx > 0)
	{
	  gtk_text_iter_forward_chars(&start_iter, glyph_idx);
	  end_iter = start_iter;
	  gtk_text_iter_forward_char(&end_iter);
	}
	   
      // Extract the selection
      gtk_text_buffer_delete (buf, &start_iter, &end_iter);

      // Reset focus to text edit window
      gtk_window_set_focus (GTK_WINDOW (w_main_window),
			    w_prev_focus);

    }
    break;
      
  }
  
  return TRUE;
}

GtkWidget *char_box_label_new (const char *markup,
			       gboolean may_focus,
			       int glyph_idx_index,
			       gunichar ch_idx)
{
  GtkWidget *event_box = gtk_event_box_new ();
  GtkWidget *label = gtk_label_new ("");

  gtk_label_set_justify (GTK_LABEL (label),
			 GTK_JUSTIFY_CENTER);
  gtk_label_set_markup (GTK_LABEL (label), markup);
  gtk_container_add (GTK_CONTAINER (event_box),
		     label);

  if (may_focus)
    {
      GTK_WIDGET_SET_FLAGS (event_box, GTK_CAN_FOCUS + GTK_SENSITIVE);

      g_signal_connect (G_OBJECT(event_box),
			"button-press-event",
			G_CALLBACK (cb_label_button_press),
			GINT_TO_POINTER (ch_idx));
      g_signal_connect (G_OBJECT(event_box),
			"key-press-event",
			G_CALLBACK (cb_label_key_press),
			GINT_TO_POINTER(glyph_idx_index));
    }

  return event_box;
}

/*
 * Todo: replace labels with handdrawn pango layouts in order to be able
 * to measure the bounding box...
 */

void display_surrounding_text ()
{
  GSList *list = NULL;
  GtkTextBuffer *buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
  GtkWidget *label;
  GtkTextIter cursor_iter, start_iter, end_iter;
  int i, glyph_idx;
  gchar *text, *p;
  int pos, num_chars;
  GString *s;
  gunichar ch;

  // Reset old selected character
  w_selected_sub_char = NULL;
  
  // Get cursor position
  gtk_text_buffer_get_iter_at_mark ( buf, &cursor_iter,
				     gtk_text_buffer_get_insert (buf));
  start_iter = end_iter = cursor_iter;
  pos = gtk_text_iter_get_line_index (&cursor_iter);

  for (i=0; i<1; i++)
    gtk_text_iter_backward_cursor_position (&start_iter);
  for (i=0; i<0; i++)
    gtk_text_iter_forward_cursor_position (&end_iter);
  
  // Get character at cursor
  text = gtk_text_iter_get_slice (&start_iter, &end_iter);

  // Build up the display string
  s = g_string_new ("");

  // g_string_append_printf (s, "&#8237;");
  g_string_append_printf(s, "<span size=\"64000\">");
  
  ch = g_utf8_get_char (text);

  switch (ch) {
  case '<' :
    g_string_append_printf(s, "&lt;");
    break;
  case '&' :
    g_string_append_printf(s, "&amp;");
    break;
  case 0x202A:
    g_string_append_printf(s, "<span size=\"48000\">[LRE]</span>");
    break;
  case 0x202B:
    g_string_append_printf(s, "<span size=\"48000\">[RLE]</span>");
    break;
  case 0x202C:
    g_string_append_printf(s, "<span size=\"48000\">[PDF]</span>");
    break;
  case 0x202D:
    g_string_append_printf(s, "<span size=\"48000\">[LRO]</span>");
    break;
  case 0x200C:
    g_string_append_printf(s, "<span size=\"48000\">[ZWNJ]</span>");
    break;
  case 0x200D:
    g_string_append_printf(s, "<span size=\"48000\">[ZWJ]</span>");
    break;
  case 0x200E:
    g_string_append_printf(s, "<span size=\"48000\">[LRM]</span>");
    break;
  case 0x200F:
    g_string_append_printf(s, "<span size=\"48000\">[RLM]</span>");
    break;
  default:
    g_string_append_printf(s, "%s", text);
    break;
  }
  g_string_append_printf(s, "</span>\n");

  // Examine how many unicode characters there are
  p=text;
  num_chars=0;
  while (g_utf8_get_char (p) )
    {
      p = g_utf8_next_char(p);
      num_chars++;
    }

  if (num_chars == 1)
      g_string_append_printf (s, "U+%04X", ch);

  // Display first character in cluster
  display_char_name(ch);
      
  label = char_box_label_new (s->str, TRUE, -1, 0);
  list = g_slist_prepend (list, label);

  if (num_chars > 1)
    {
      g_string_printf (s, "<span size=\"48000\" foreground=\"darkblue\"> = </span>");
      label = char_box_label_new (s->str, FALSE, -1, 0);
      list = g_slist_prepend (list, label);

      glyph_idx = 0;
      p=text;
      while (g_utf8_get_char (p) )
	{
	  gunichar ch = g_utf8_get_char (p);
	  gunichar glyph_ch = ch;
	  gchar pad_left[3];
	  gchar pad_right[3];
	  gchar *prev_p = p;
	  p = g_utf8_next_char(p);

	  pad_left[0] = 0;
	  pad_right[0] = 0;
	  
	  if (prev_p != text)
	    {
	      g_string_printf (s, "<span size=\"48000\" foreground=\"darkred\"> + </span>");
	      label = char_box_label_new (s->str, FALSE, -1, ch);
	      list = g_slist_prepend (list, label);
	    }

	  // Switch some hebrew glyphs to get around bugs in label crop...
	  if (ch == 0x05C1)
	    {
	      glyph_ch = 0x05B9; // cholam
	      sprintf(pad_left, "      ");
	    }
	  else if (ch == 0x05C2) // sin dots
	    {
	      glyph_ch = 0x05B9; // cholam
	      sprintf(pad_right, "      ");
	    }

	  // Add the character
	  g_string_printf(s, "%s<span size=\"64000\">&#%d;</span>%s\nU+%04X",
			  pad_left, glyph_ch, pad_right, ch);
	  label = char_box_label_new (s->str, TRUE, glyph_idx, ch);
	  list = g_slist_prepend (list, label);
	  glyph_idx++;
	}
    }


  list = g_slist_reverse (list);

  pack_label_list_in_hbox (list, w_text_inspector);

  g_string_free(s, TRUE);

  g_free(text);
}

void        cb_text_changed                (GtkTextBuffer *textbuffer,
                                            gpointer user_data)
{
  display_surrounding_text ();
}

void
display_char_name(gunichar ch)
{
  // Erase old contents of statusbar
  gtk_statusbar_pop (GTK_STATUSBAR(w_statusbar), 0);

  if (uniname[ch]) {
      gtk_statusbar_push (GTK_STATUSBAR(w_statusbar), 0, g_strdup(uniname[ch]));
  }
}

void        cb_mark_set (GtkTextBuffer *textbuffer,
			 GtkTextIter *arg1,
			 GtkTextMark *arg2,
			 gpointer user_data)
{
  display_surrounding_text ();
}

void        cb_cursor_moved      (GtkTextView *textview,
				  GtkMovementStep arg1,
				  gint arg2,
				  gboolean arg3,
				  gpointer user_data)
{
  display_surrounding_text();
}

static void
set_last_directory_from_filename(const gchar *filename)
{
    gchar *dir_name;
    
    if (guci_last_directory)
        g_free(guci_last_directory);
    dir_name = g_path_get_dirname(filename);
    guci_last_directory = g_strdup_printf("%s", filename);
    g_free(dir_name);
}

static void
pack_label_list_in_hbox(GSList *label_list,
			GtkWidget *hbox)
{
  GtkWidget *inner_hbox;
  
  // Get old_inner_hbox
  inner_hbox = g_object_get_data (G_OBJECT (hbox), "inner_hbox");

  if (inner_hbox)
    gtk_widget_destroy(inner_hbox);

  inner_hbox = gtk_hbox_new(0,0);
  gtk_box_pack_start (GTK_BOX (hbox), inner_hbox, 0,0,0);

  // pack new list
  while (label_list)
    {
      gtk_box_pack_start (GTK_BOX (inner_hbox), GTK_WIDGET (label_list->data),
			  0,0,0);
      label_list = label_list->next;
    }

  gtk_widget_show_all (inner_hbox);

  g_object_set_data (G_OBJECT (hbox), "inner_hbox", inner_hbox);
}

/*======================================================================
// Misc help functions.
//----------------------------------------------------------------------
*/
static void
file_to_textview(const gchar *filename,
		 GError **error)
{
  gchar *contents;
  gint len;

  if (!filename)
    {
      contents = g_strdup("");
      len = 0;
    }
  else
    g_file_get_contents(filename,
			&contents,
			&len,
			error);

  if (*error)
    return;

  // Erase old contents of text viewer
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text)),
			    contents,
			    len);

  g_free(contents);
}

static void
textview_to_file(const gchar *filename,
		 GError **error)
{
  gchar *contents;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
  GtkTextIter iter_start, iter_end;
  FILE *OUT_FILE;

  gtk_text_buffer_get_start_iter(buffer, &iter_start);
  gtk_text_buffer_get_end_iter(buffer, &iter_end);

  contents = gtk_text_buffer_get_text (buffer,
				       &iter_start,
				       &iter_end,
				       TRUE);

  if (*error)
    return;

  OUT_FILE = fopen(filename, "w");
  if (!OUT_FILE)
    {
      // should fill in error
      printf("Failed opening file %s for writing!\n", filename);
      g_free(contents);

      return;
    }
  fwrite(contents, 1, strlen(contents), OUT_FILE);
  fclose(OUT_FILE);

  g_free(contents);
}

/*======================================================================
// Menu utility functions.
//----------------------------------------------------------------------
*/
GtkWidget*
create_save_if_modified_dialog()
{
  GtkWidget *dialog = gtk_message_dialog_new (
			   GTK_WINDOW (w_main_window),
			   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   GTK_MESSAGE_QUESTION,
			   GTK_BUTTONS_NONE,
			   "Do you want to save the changed you made to the document?"
			   );
  guci_dialog_add_button (GTK_DIALOG (dialog),
			  _("Do_n't save"), GTK_STOCK_NO,
			  GTK_RESPONSE_NO);
  
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
  
  gtk_dialog_add_button (GTK_DIALOG (dialog),
			 GTK_STOCK_SAVE,
			 GTK_RESPONSE_YES);
  
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

  return dialog;
}

/*======================================================================
//  Menu callback functions.
//----------------------------------------------------------------------
*/
static void
cb_menu_tbd (gpointer   callback_data,
	     guint      callback_action,
	     GtkWidget *widget)
{
  guci_error(w_main_window, "Sorry. Not implemented yet!\n");
  
  return;
}

static void cb_open_ok(GtkWidget *this,
		       gpointer user_data)
{
  GtkWidget *file_selection = GTK_WIDGET(user_data);
  const gchar *selected_filename;
  GError *error = NULL;
    
  selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION(file_selection));
  set_last_directory_from_filename(selected_filename);

  file_to_textview(selected_filename, &error);

  if (error)
    {
      printf("error: %s\n", error->message);
    }
  
  gtk_widget_destroy (file_selection);
}

static void
cb_menu_open(gpointer   callback_data,
	     guint      callback_action,
	     GtkWidget *widget)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
  GtkWidget *file_selector;
  
  if (gtk_text_buffer_get_modified (buffer)) {
    GtkWidget *dialog = create_save_if_modified_dialog();
    int res = gtk_dialog_run (GTK_DIALOG (dialog));
    
    gtk_widget_destroy (dialog);
  
    switch (res) {
    case GTK_RESPONSE_YES:
      cb_menu_save(NULL,0,NULL);
      break;
    case GTK_RESPONSE_CANCEL:
      return;
    default:
      break;
    }
  }

  file_selector = gtk_file_selection_new("File to open");
  
  if (guci_last_directory)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), guci_last_directory);
  
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
		    "clicked", GTK_SIGNAL_FUNC (cb_open_ok), file_selector);
  
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
			    "clicked",
			    G_CALLBACK (gtk_widget_destroy),
			    (gpointer) file_selector); 
  
  /* Center file selector on parent */
  gtk_window_set_transient_for (GTK_WINDOW (file_selector),
				GTK_WINDOW (w_main_window));
  
  
  /* Display that dialog */
  gtk_widget_show (file_selector);
}
  
static void cb_save_ok(GtkWidget *this,
		       gpointer user_data)
{
  GtkWidget *file_selection = GTK_WIDGET(user_data);
  const gchar *selected_filename;
  GError *error = NULL;
    
  selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION(file_selection));
  set_last_directory_from_filename(selected_filename);

  textview_to_file(selected_filename, &error);

  if (error)
    {
      printf("error: %s\n", error->message);
    }
  gtk_widget_destroy (file_selection);
}

static void
cb_menu_save(gpointer   callback_data,
	     guint      callback_action,
	     GtkWidget *widget)
{
  GtkWidget *file_selector;

  file_selector = gtk_file_selection_new("File to save");
  
  if (guci_last_directory)
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(file_selector), guci_last_directory);
  
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION(file_selector)->ok_button),
		    "clicked", GTK_SIGNAL_FUNC (cb_save_ok), file_selector);
  
  g_signal_connect_swapped (GTK_OBJECT (GTK_FILE_SELECTION (file_selector)->cancel_button),
			    "clicked",
			    G_CALLBACK (gtk_widget_destroy),
			    (gpointer) file_selector); 
  
  /* Center file selector on parent */
  gtk_window_set_transient_for (GTK_WINDOW (file_selector),
				GTK_WINDOW (w_main_window));
  
  
  /* Display that dialog */
  gtk_widget_show (file_selector);
}
  
static void
cb_menu_about(gpointer   callback_data,
	      guint      callback_action,
	      GtkWidget *widget)
{
  GtkWidget *about_window;
  GtkWidget *vbox;
  GtkWidget *label;
  
  gchar *markup;
  
  about_window = gtk_dialog_new ();
  gtk_dialog_add_button (GTK_DIALOG (about_window),
			 GTK_STOCK_OK, GTK_RESPONSE_OK);
  gtk_dialog_set_default_response (GTK_DIALOG (about_window),
				   GTK_RESPONSE_OK);
  
  gtk_window_set_title(GTK_WINDOW (about_window), _("About Guci"));
  
  gtk_window_set_resizable (GTK_WINDOW (about_window), FALSE);

  gtk_window_set_transient_for (GTK_WINDOW (about_window),
				GTK_WINDOW (w_main_window));
  
  g_signal_connect (G_OBJECT (about_window), "delete-event",
		    G_CALLBACK (gtk_widget_destroy), NULL);
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->vbox), vbox, FALSE, FALSE, 0);
  
  label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
  markup = g_strdup_printf ("<span size=\"xx-large\" weight=\"bold\">guci "VERSION"</span>\n\n"
			    "%s\n\n"
			    "<span>%s\n%sEmail: <tt>&lt;%s&gt;</tt></span>\n",
			    _("the Gtk UniCode Inspector"),
			    _("Licensed under the GPL\n"),
			    _("Programming by: Dov Grobgeld\n"),
			    _("dov.grobgeld@weizmann.ac.il"));
  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  
  gtk_widget_show_all (about_window);
  gtk_dialog_run (GTK_DIALOG (about_window));
  gtk_widget_destroy (about_window);
}

static void
cb_menu_quit(gpointer   callback_data,
	     guint      callback_action,
	     GtkWidget *widget)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
  
  if (gtk_text_buffer_get_modified (buffer))
    {
      GtkWidget *dialog = create_save_if_modified_dialog();
      int res = gtk_dialog_run (GTK_DIALOG (dialog));
      
      gtk_widget_destroy (dialog);
      
      switch (res) {
      case GTK_RESPONSE_YES:
	cb_menu_save(NULL,0,NULL);
	break;
      case GTK_RESPONSE_CANCEL:
	return;
      default:
	break;
      }
    }
  gtk_main_quit();
}

static void
cb_menu_new(gpointer   callback_data,
	    guint      callback_action,
	    GtkWidget *widget)
{
  GError *error = NULL;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));

  if (gtk_text_buffer_get_modified (buffer)) {
    GtkWidget *dialog = create_save_if_modified_dialog();
    int res = gtk_dialog_run (GTK_DIALOG (dialog));
    
    gtk_widget_destroy (dialog);
  
    switch (res) {
    case GTK_RESPONSE_YES:
      cb_menu_save(NULL,0,NULL);
      break;
    case GTK_RESPONSE_CANCEL:
      return;
    default:
      break;
    }
  }
  file_to_textview(NULL, &error);
}
  
static void
cb_menu_edit_paste(gpointer   callback_data,
		   guint      callback_action,
		   GtkWidget *widget)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
  gtk_text_buffer_paste_clipboard (buffer,
				   gtk_clipboard_get (GDK_NONE),
				   NULL,
				   TRUE);
  
  gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (w_text),
				      gtk_text_buffer_get_mark (buffer,
								"insert"));
}

static void cb_menu_edit_cut(gpointer   callback_data,
			     guint      callback_action,
			     GtkWidget *widget)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));

  gtk_text_buffer_copy_clipboard (buffer,
				  gtk_clipboard_get (GDK_NONE));
  
  gtk_text_buffer_delete_selection (buffer, 
				    TRUE,
				    TRUE);
  
}

static void cb_menu_edit_copy(gpointer   callback_data,
			     guint      callback_action,
			     GtkWidget *widget)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));
  gtk_text_buffer_copy_clipboard (buffer,
				  gtk_clipboard_get (GDK_NONE));
  gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (w_text),
				      gtk_text_buffer_get_mark (buffer,
								"insert"));
  
}

static void cb_menu_edit_delete(gpointer   callback_data,
				guint      callback_action,
				GtkWidget *widget)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));

  gtk_text_buffer_delete_selection (buffer, 
				    TRUE,
				    TRUE);
}

static void cb_menu_edit_select_all(gpointer   callback_data,
				    guint      callback_action,
				    GtkWidget *widget)
{
  GtkTextIter start_iter;
  GtkTextIter end_iter;
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (w_text));

  gtk_text_buffer_get_start_iter (buffer, &start_iter);
  gtk_text_buffer_get_end_iter (buffer, &end_iter);
  gtk_text_buffer_place_cursor (buffer, &end_iter);
  gtk_text_buffer_move_mark_by_name (buffer, "selection_bound", &start_iter);
}

static void
guci_message_va(GtkWidget *parent,
		 GtkMessageType msg_type,
		 const char *fmt,
		 va_list args)
{
  GtkWidget *dialog;
  char *msg;

  msg = g_strdup_vprintf(fmt, args);

  dialog = gtk_message_dialog_new (GTK_WINDOW(parent),
				   GTK_DIALOG_DESTROY_WITH_PARENT,
				   msg_type,
				   GTK_BUTTONS_CLOSE,
				   msg);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);  
  
  g_free(msg);
}

void guci_error(GtkWidget *parent,
		 const char *fmt,
		 ...)
{
  va_list args;

  va_start(args, fmt);

  guci_message_va(parent, GTK_MESSAGE_ERROR, fmt, args);
}

void guci_info(GtkWidget *parent,
		 const char *fmt,
		 ...)
{
  va_list args;

  va_start(args, fmt);

  guci_message_va(parent, GTK_MESSAGE_INFO, fmt, args);
}
				

GtkWidget *
guci_dialog_add_button (GtkDialog *dialog, const gchar* text, const gchar* stock_id, gint response_id)
{
  GtkWidget *button;
  
  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (text != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  
  button = guci_button_new_with_stock_image (text, stock_id);
  g_return_val_if_fail (button != NULL, NULL);
  
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  
  gtk_widget_show (button);
  
  gtk_dialog_add_action_widget (dialog, button, response_id);     
  
  return button;
}

GtkWidget* 
guci_button_new_with_stock_image (const gchar* text, const gchar* stock_id)
{
  GtkWidget *button;
  GtkStockItem item;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *hbox;
  GtkWidget *align;
  
  button = gtk_button_new ();
  
  if (GTK_BIN (button)->child)
    gtk_container_remove (GTK_CONTAINER (button),
				GTK_BIN (button)->child);
  
  if (gtk_stock_lookup (stock_id, &item))
    {
      label = gtk_label_new_with_mnemonic (text);
      
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
      
      image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
      hbox = gtk_hbox_new (FALSE, 2);
      
      align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      
      gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      
      gtk_container_add (GTK_CONTAINER (button), align);
      gtk_container_add (GTK_CONTAINER (align), hbox);
      gtk_widget_show_all (align);
      
      return button;
    }
  
  label = gtk_label_new_with_mnemonic (text);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
  
  gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
  
  gtk_widget_show (label);
  gtk_container_add (GTK_CONTAINER (button), label);
  
  return button;
}

