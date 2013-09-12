/* ev-view-presenter-note.c
 *  this file is part of evince, a gnome document viewer
 *
 * Copyright (C) 2013 Alessandro Campagni <alessandro.campagni@gmail.com>
 *
 * Evince is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Evince is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <json-glib/json-glib.h>

#include "ev-view-presenter-note.h"
#include "ev-view-presenter.h"

enum {
        PROP_0,
        PROP_PRESENTATION,
        N_PROP
};

struct _EvViewPresenterNote {

        GtkTextView         parent_instance;

        GtkTextBuffer      *buffer;

        EvViewPresentation *presentation;

        JsonParser         *parser;
        JsonReader         *reader;
};

struct _EvViewPresenterNoteClass {
        GtkTextViewClass parent_class;
};

static GParamSpec *obj_properties[N_PROP] = { NULL, };

G_DEFINE_TYPE (EvViewPresenterNote, ev_view_presenter_note, GTK_TYPE_TEXT_VIEW)

static void
ev_view_presenter_note_set_page (EvViewPresenterNote *self,
                                 gint                 page)
{
        GString             *page_str;

        page_str = g_string_new ("");
        g_string_printf (page_str, "%d", page);

        json_reader_read_member (self->reader, page_str->str );
        const gchar *note = json_reader_get_string_value (self->reader);
        json_reader_end_member (self->reader);

        if (note == NULL)
                gtk_text_buffer_set_text (self->buffer, "No notes defined for current slide :(", -1);
        else
                gtk_text_buffer_set_text (self->buffer, note, -1);
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (self), self->buffer);
}

static void
update_note_page_cb (GObject    *obj,
                     GParamSpec *pspec,
                     gpointer    user_data)
{
        EvViewPresenterNote *self = EV_VIEW_PRESENTER_NOTE (user_data);
        gint                 page;

        page = ev_view_presentation_get_current_page (self->presentation);

        ev_view_presenter_note_set_page (self, page);
}

static void
ev_view_presenter_note_set_property (GObject      *obj,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
        EvViewPresenterNote *self = EV_VIEW_PRESENTER_NOTE (obj);

        switch (prop_id) {
        case PROP_PRESENTATION:
                self->presentation = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
        }
}

static void
ev_view_presenter_note_init (EvViewPresenterNote *self)
{
}

static void
ev_view_presenter_note_constructed (GObject *obj)
{
        EvViewPresenterNote *self = EV_VIEW_PRESENTER_NOTE (obj);
        gchar               *uri;
        GError              *err = NULL;
        GFile               *file;
        GtkCssProvider      *provider;

        gtk_text_view_set_left_margin (GTK_TEXT_VIEW (self), 30);
        gtk_text_view_set_right_margin (GTK_TEXT_VIEW (self), 30);
        gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self), GTK_WRAP_WORD);
        gtk_text_view_set_editable (GTK_TEXT_VIEW (self), FALSE);
        gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (self), FALSE);

        file = g_file_new_for_uri (
                g_strjoin (NULL,
                           ev_view_presentation_get_document_uri (self->presentation),
                           ".notes",
                           NULL));
        uri = g_file_get_path (file);
        g_object_unref (file);

        self->parser = json_parser_new ();
        if (json_parser_load_from_file (self->parser, uri, &err)) {
                self->reader = json_reader_new (json_parser_get_root (self->parser));
                g_signal_connect (self->presentation,
                                  "notify::current-page",
                                  G_CALLBACK (update_note_page_cb),
                                  self);

		self->buffer = gtk_text_buffer_new (NULL);
                ev_view_presenter_note_set_page (self,
                                                 ev_view_presentation_get_current_page (self->presentation));
        } else {
                self->buffer = gtk_text_buffer_new (NULL);
                gtk_text_buffer_set_text (self->buffer,
                                          g_strjoin (NULL,
                                                     "To start using notes with this presentation just create the note file ",
                                                     uri,
                                                     NULL),
                                          -1);
                gtk_text_view_set_buffer (GTK_TEXT_VIEW (self), self->buffer);
        }

        /* style */
        provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_data (provider,
                                         "EvViewPresenterNote {\n"
                                         "  background-color: black;\n"
                                         "  color: white;\n"
                                         "  font-size: 2.5em; }",
                                         -1, NULL);
        gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                   GTK_STYLE_PROVIDER (provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref (provider);

        G_OBJECT_CLASS (ev_view_presenter_note_parent_class)->constructed (obj);
}

static gboolean
ev_view_presenter_note_key_press_event (GtkWidget   *widget,
                                        GdkEventKey *event)
{
  /* this will prevent the GtkTextView from catching */
  /* keyboard inputs */
  return FALSE;
}

static void
ev_view_presenter_note_dispose (GObject *obj)
{
  EvViewPresenterNote *self = EV_VIEW_PRESENTER_NOTE (obj);

  if (self->parser) {
    g_object_unref (self->parser);
    self->parser = NULL;
  }

  if (self->reader) {
    g_object_unref (self->reader);
    self->reader = NULL;
  }

  if (self->buffer) {
    g_object_unref (self->buffer);
    self->buffer = NULL;
  }

  G_OBJECT_CLASS (ev_view_presenter_note_parent_class)->dispose (obj);
}

static void
ev_view_presenter_note_class_init (EvViewPresenterNoteClass *klass)
{
        GtkCssProvider *provider;
        GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        gobject_class->set_property = ev_view_presenter_note_set_property;
        gobject_class->constructed = ev_view_presenter_note_constructed;
	gobject_class->dispose = ev_view_presenter_note_dispose;

        widget_class->key_press_event = ev_view_presenter_note_key_press_event;

        obj_properties[PROP_PRESENTATION] =
                g_param_spec_object ("presentation",
                                     "Presentation",
                                     "The running presentation",
                                     EV_TYPE_VIEW_PRESENTATION,
                                     G_PARAM_WRITABLE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);
        g_object_class_install_properties (gobject_class,
                                           N_PROP, obj_properties);

        provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_data (provider,
                                         "EvViewPresenterNote {\n"
                                         " background-color: white; }",
                                         -1, NULL);
        gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                   GTK_STYLE_PROVIDER (provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref (provider);
}

GtkWidget *
ev_view_presenter_note_new (EvViewPresentation *presentation)
{
        return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER_NOTE,
                                         "presentation", presentation, NULL));
}
