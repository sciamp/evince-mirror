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

#include "ev-view-presenter-note.h"
#include "ev-view-presenter.h"

struct _EvViewPresenterNote {
        GtkTextView parent_instance;
};

struct _EvViewPresenterNoteClass {
        GtkTextViewClass parent_class;
};

G_DEFINE_TYPE (EvViewPresenterNote, ev_view_presenter_note, GTK_TYPE_TEXT_VIEW)

static void
ev_view_presenter_note_init (EvViewPresenterNote *self)
{
        GtkTextBuffer *buffer;
        const gchar *text = "Testing presenter note!\0";

        buffer = gtk_text_buffer_new (NULL);
        gtk_text_buffer_set_text (buffer,
                                  text,
                                  -1);

        gtk_text_view_set_buffer (GTK_TEXT_VIEW (self),
                                  buffer);
}

static void
ev_view_presenter_note_class_init (EvViewPresenterNoteClass *klass)
{
        GtkCssProvider *provider;

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
ev_view_presenter_note_new ()
{
        return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER_NOTE,
                                         NULL));
}
