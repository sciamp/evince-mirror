/* ev-view-presenter-timer.c
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

#include "ev-view-presenter-timer.h"

struct _EvViewPresenterTimer {
  GtkLabel parent_instance;

  GTimer  *timer;
};

struct _EvViewPresenterTimerClass {
  GtkLabelClass parent_class;
};

G_DEFINE_TYPE (EvViewPresenterTimer, ev_view_presenter_timer, GTK_TYPE_LABEL)

static void
ev_view_presenter_timer_init (EvViewPresenterTimer *self)
{
}

static gboolean
update_label_cb (GtkWidget     *widget,
                 GdkFrameClock *fc,
                 gpointer       user_data)
{
  EvViewPresenterTimer *self = EV_VIEW_PRESENTER_TIMER (user_data);
  gint                  elapsed;
  gint                  h;
  gint                  m;
  gint                  s;
  gchar                *label;

  elapsed = (int) g_timer_elapsed (self->timer, NULL);

  h = (int) elapsed / 3600;
  elapsed %= 3600;
  m = (int) elapsed / 60;
  elapsed %= 60;
  s = (int) elapsed;

  label = g_strdup_printf ("%02d:%02d:%02d",
                           h, m, s);

  gtk_label_set_text (GTK_LABEL (self),
                      label);

  g_free (label);

  return TRUE;
}

static void
ev_view_presenter_timer_constructed (GObject *obj)
{
  EvViewPresenterTimer *self = EV_VIEW_PRESENTER_TIMER (obj);
  const gchar *label = "00:00:00";

  gtk_label_set_text (GTK_LABEL (self), label);
  gtk_widget_add_tick_callback (GTK_WIDGET (self),
                                update_label_cb,
                                self, NULL);
  self->timer = g_timer_new ();
}

static void
ev_view_presenter_timer_class_init (EvViewPresenterTimerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = ev_view_presenter_timer_constructed;
}

GtkWidget *
ev_view_presenter_timer_new (void)
{
  return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER_TIMER, NULL));
}
