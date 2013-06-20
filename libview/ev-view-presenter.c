/* ev-view-presenter.c
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

#include "config.h"

#include <stdlib.h>
#include <glib/gi18n-lib.h>
#include <gdk/gdkkeysyms.h>

#include "ev-view-presenter.h"
#include "ev-view-presentation.h"
#include "ev-jobs.h"
#include "ev-job-scheduler.h"
#include "ev-transition-animation.h"
#include "ev-view-cursor.h"
#include "ev-page-cache.h"

enum {
        PROP_0,
        PROP_DOCUMENT,
        PROP_DOCUMENT_PAGE,
        PROP_ROTATION,
        PROP_INVERTED_COLORS
};

enum {
        CHANGE_PAGE,
        FINISHED,
        EIGNAL_EXTERNAL_LINK,
        N_SIGNALS
};

struct _EvViewPresenter
{
        GtkWidget           base;

        guint               is_constructing : 1;

        guint               current_page;
        EvDocument         *document;
        guint               rotation;
        gdouble             scale;
        gint                monitor_width;
        gint                monitor_height;
};

struct _EvViewPresenterClass
{
        GtkWidgetClass        base_class;

        void (* change_page) (EvViewPresenter *self,
                              GtkScrollType    scroll);
        void (* finished)    (EvViewPresenter *self);
};

static guint signals[N_SIGNALS] = { 0 }; 

G_DEFINE_TYPE (EvViewPresenter, ev_view_presenter, GTK_TYPE_WIDGET)

static GdkRGBA black = { 0., 0., 0., 1. };
static GdkRGBA white = { 1., 1., 1., 1. };

static void
ev_view_presenter_init (EvViewPresenter *self)
{
        gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
        self->is_constructing = TRUE;
}

static void
ev_view_presenter_update_current_page (EvViewPresenter *self,
                                       gint             next)
{
        /* here there's some work to do */
}

static void
ev_view_presenter_presentation_end (EvViewPresenter *self)
{
        /* presentation displays a black screen with */
        /* "End of presentation */
        /* we just notify the presenter in some way */
}

void
ev_view_presenter_previous_page (EvViewPresenter *self)
{
        ev_view_presenter_update_current_page (self,
                                               self->current_page - 1);
}

void
ev_view_presenter_next_page (EvViewPresenter *self)
{
        guint n_pages;
        gint  new_page;

        n_pages = ev_document_get_n_pages (self->document);
        new_page = self->current_page + 1;

        if (new_page == n_pages)
                ev_view_presenter_presentation_end (self);
        else
                ev_view_presenter_update_current_page (self,
                                                       new_page);
}

static void
ev_view_presenter_change_page (EvViewPresenter *self,
                               GtkScrollType    scroll)
{
        switch (scroll) {
        case GTK_SCROLL_PAGE_FORWARD:
                ev_view_presenter_next_page (self);
                break;
        case GTK_SCROLL_PAGE_BACKWARD:
                ev_view_presenter_previous_page (self);
                break;
        default:
                g_assert_not_reached ();
        }
}

static void
ev_view_presenter_class_init (EvViewPresenterClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
        GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

        klass->change_page = ev_view_presenter_change_page;

        /* gobject_class->dispose = ev_view_presenter_dispose; */
        /* gobject_class->constructor = ev_view_presenter_constructor; */
}
