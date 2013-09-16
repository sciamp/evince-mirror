/* ev-view-presenter-widget.h
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

#if !defined (__EV_EVINCE_VIEW_H_INSIDE__ ) && !defined (EVINCE_COMPILATION)
#error "Only <evince-view.h> can be included directly."
#endif

#ifndef __EV_VIEW_PRESENTER_WIDGET__
#define __EV_VIEW_PRESENTER_WIDGET__

#include <gtk/gtk.h>

#include <evince-document.h>
#include "ev-view-presentation.h"
#include "ev-view-presenter-note.h"

G_BEGIN_DECLS

#define EV_TYPE_VIEW_PRESENTER_WIDGET (ev_view_presenter_widget_get_type ())
#define EV_VIEW_PRESENTER_WIDGET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EV_TYPE_VIEW_PRESENTER_WIDGET, EvViewPresenterWidget))
#define EV_IS_VIEW_PRESENTER_WIDGET(obj)   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EV_TYPE_VIEW_PRESENTER_WIDGET))

typedef struct _EvViewPresenterWidget      EvViewPresenterWidget;
typedef struct _EvViewPresenterWidgetClass EvViewPresenterWidgetClass;

GType           ev_view_presenter_widget_get_type         (void) G_GNUC_CONST;
GtkWidget      *ev_view_presenter_widget_new              (EvViewPresentation *presentation,
                                                           gboolean            with_notes);
guint           ev_view_presenter_widget_get_current_page (EvViewPresenterWidget *self);
void            ev_view_presenter_widget_next_page        (EvViewPresenterWidget *self);
void            ev_view_presenter_widget_previous_page    (EvViewPresenterWidget *self);
void            ev_view_presenter_widget_set_rotation     (EvViewPresenterWidget *self,
                                                           gint                   rotation);
guint           ev_view_presenter_widget_get_rotaion      (EvViewPresenterWidget *self);

G_END_DECLS

#endif /* __EV_VIEW_PRESENTER_WIDGET__ */
