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
        PROP_NOTES_FILE,
        N_PROP
};

struct _EvViewPresenterNote {
        GtkTextView parent_instance;

        GFile      *file;
        JsonParser *parser;
        JsonReader *reader;
};

struct _EvViewPresenterNoteClass {
        GtkTextViewClass parent_class;
};

static GParamSpec *obj_properties[N_PROP] = { NULL, };

G_DEFINE_TYPE (EvViewPresenterNote, ev_view_presenter_note, GTK_TYPE_TEXT_VIEW)

void
ev_view_presenter_note_for_page (EvViewPresenterNote *self,
                                 gint                 page)
{
        GString       *page_str;
        GtkTextBuffer *buffer;

        page_str = g_string_new ("");
        g_string_printf (page_str, "%d", page);

        json_reader_read_member (self->reader, page_str->str );
        const gchar *note = json_reader_get_string_value (self->reader);
        json_reader_end_member (self->reader);

        buffer = gtk_text_buffer_new (NULL);
        if (note == NULL)
                gtk_text_buffer_set_text (buffer, "No notes defined for current slide :(", -1);
        else
                gtk_text_buffer_set_text (buffer, note, -1);
        gtk_text_view_set_buffer (GTK_TEXT_VIEW (self), buffer);
}

static void
ev_view_presenter_note_set_property (GObject      *obj,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
        EvViewPresenterNote *self = EV_VIEW_PRESENTER_NOTE (obj);

        switch (prop_id) {
        case PROP_NOTES_FILE:
                self->file = g_value_get_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
        }
}

static void
ev_view_presenter_note_init (EvViewPresenterNote *self)
{
}

static GObject *
ev_view_presenter_note_constructor (GType                  gtype,
                                    guint                  n_prop,
                                    GObjectConstructParam *prop)
{
        GObject             *obj;
        EvViewPresenterNote *self;
        GError              *err = NULL;

        obj = G_OBJECT_CLASS (ev_view_presenter_note_parent_class)->constructor (gtype,
                                                                                 n_prop,
                                                                                 prop);

        /* g_print ("object build with path %s \n", */
        /*          g_file_get_uri (EV_VIEW_PRESENTER_NOTE (obj)->file)); */
        self = EV_VIEW_PRESENTER_NOTE (obj);
        self->parser = json_parser_new ();
        if (json_parser_load_from_file (self->parser,
                                        g_file_get_uri (self->file),
                                        &err))
                self->reader = json_reader_new (json_parser_get_root (self->parser));
        else
                fprintf (stderr, "Unable to read file: %s\n", err->message);

        return obj;
}

static void
ev_view_presenter_note_class_init (EvViewPresenterNoteClass *klass)
{
        GtkCssProvider *provider;
        GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

        gobject_class->set_property = ev_view_presenter_note_set_property;
        gobject_class->constructor = ev_view_presenter_note_constructor;

        obj_properties[PROP_NOTES_FILE] =
                g_param_spec_object ("file",
                                     "File",
                                     "JSON file containing presenter notes",
                                     G_TYPE_FILE,
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
ev_view_presenter_note_new (const gchar *uri)
{
        GFile *file = g_file_new_for_uri (uri);

        /* g_print ("ev_view_presenter_note_new: got %s \n", g_file_get_uri (file)); */

        return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER_NOTE,
                                         "file", file, NULL));
}