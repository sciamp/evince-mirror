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

#include "ev-view-presenter.h"
#include "ev-view-presenter-widget.h"
#include "ev-view-presenter-note.h"
#include "ev-view-presenter-timer.h"

enum {
        PROP_0,
        PROP_PRESENTATION,
        PROP_NOTES
};

struct _EvViewPresenter
{
        GtkBox              base;

        EvViewPresentation *presentation;
        GtkWidget          *presenter_widget;
        GtkWidget          *notes;
        GtkWidget          *timer;
};

struct _EvViewPresenterClass
{
        GtkBoxClass            base_class;
};

G_DEFINE_TYPE (EvViewPresenter, ev_view_presenter, GTK_TYPE_BOX)

static void
ev_view_presenter_dispose (GObject *obj)
{
        EvViewPresenter *self = EV_VIEW_PRESENTER (obj);

        if (self->presentation) {
                g_object_unref (self->presentation);
                self->presentation = NULL;
        }

        if (self->notes) {
                g_object_unref (self->notes);
                self->notes = NULL;
        }

        G_OBJECT_CLASS (ev_view_presenter_parent_class)->dispose (obj);
}

static void
ev_view_presenter_set_property (GObject      *obj,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
        EvViewPresenter *self = EV_VIEW_PRESENTER (obj);

        switch (prop_id) {
        case PROP_PRESENTATION:
                self->presentation = g_value_dup_object (value);
                break;
        case PROP_NOTES:
                self->notes = g_value_dup_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj,
                                                   prop_id,
                                                   pspec);
        }
}

static void
ev_view_presenter_init (EvViewPresenter *self)
{
}

static void
ev_view_presenter_constructed (GObject *obj)
{
        EvViewPresenter *self = EV_VIEW_PRESENTER (obj);
        GtkWidget       *left_box;

        gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                        GTK_ORIENTATION_HORIZONTAL);

        self->presenter_widget = ev_view_presenter_widget_new (self->presentation);
        gtk_box_pack_start (GTK_BOX (self), self->presenter_widget,
                            FALSE, TRUE, 0);

        left_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start (GTK_BOX (self), left_box,
                            TRUE, TRUE, 0);

        self->timer = ev_view_presenter_timer_new ();
        self->notes = ev_view_presenter_note_new (self->presentation);
        gtk_box_pack_start (GTK_BOX (left_box), self->timer,
                            TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (left_box), self->notes,
                            TRUE, TRUE, 0);

        G_OBJECT_CLASS (ev_view_presenter_parent_class)->constructed (obj);
}

static void
ev_view_presenter_class_init (EvViewPresenterClass *klass)
{
        GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);

        gobject_class->dispose = ev_view_presenter_dispose;
        gobject_class->set_property = ev_view_presenter_set_property;
        gobject_class->constructed = ev_view_presenter_constructed;

        g_object_class_install_property (gobject_class,
                                         PROP_PRESENTATION,
                                         g_param_spec_object ("presentation",
                                                              "Presentation",
                                                              "Running presentation",
                                                              EV_TYPE_VIEW_PRESENTATION,
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (gobject_class,
                                         PROP_NOTES,
                                         g_param_spec_object ("notes",
                                                              "Notes",
                                                              "Presenter notes",
                                                              EV_TYPE_VIEW_PRESENTER_NOTE,
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_STRINGS));
}

GtkWidget *
ev_view_presenter_new (EvViewPresentation *presentation)
{
        g_return_val_if_fail (EV_IS_VIEW_PRESENTATION (presentation),
                              NULL);

        return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER,
                                         "presentation", presentation,
                                         NULL));
 }
