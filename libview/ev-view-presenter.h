/* ev-view-presenter.h
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

#ifndef __EV_VIEW_PRESENTER__
#define __EV_VIEW_PRESENTER__

#include <gtk/gtk.h>

#include <evince-document.h>

G_BEGIN_DECLS

#define EV_TYPE_VIEW_PRESENTER (ev_view_presenter_get_type ())
#define EV_VIEW_PRESENTER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EV_TYPE_VIEW_PRESENTER, EvViewPresenter))
#define EV_IS_VIEW_PRESENTER   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EV_TYPE_VIEW_PRESENTER))

typedef struct _EvViewPresenter      EvViewPresenter;
typedef struct _EvViewPresenterClass EvViewPresenterClass;

GType           ev_view_presenter_get_type         (void) G_GNUC_CONST;
GtkWidget      *ev_view_presenter_new              (EvDocument      *document,
                                                    guint            current_page,
<<<<<<< HEAD
                                                    guint            rotation);
=======
                                                    guint            rotation,
                                                    gboolean         inverted_colors);
>>>>>>> c6ced242c6b21e7d47d5c277b44ea9aa78a739ef
guint           ev_view_presenter_get_current_page (EvViewPresenter *self);
void            ev_view_presenter_next_page        (EvViewPresenter *self);
void            ev_view_presenter_previous_page    (EvViewPresenter *self);
void            ev_view_presenter_set_rotation     (EvViewPresenter *self,
                                                    gint             rotation);
<<<<<<< HEAD
guint           ev_view_presenter_get_rotaion      (EvViewPresenter *self);
=======
>>>>>>> c6ced242c6b21e7d47d5c277b44ea9aa78a739ef

G_END_DECLS

#endif /* __EV_VIEW_PRESENTER__ */
