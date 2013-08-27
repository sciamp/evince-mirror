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

enum {
  PRESENTATION_RUNNING,
  PRESENTATION_PAUSED
};

struct _EvViewPresenterTimer {
  GtkBox          parent_instance;

  GTimer         *timer;
  GtkWidget      *time;
  PangoAttrList  *attr_list;
  PangoAttribute *weight;
  PangoAttribute *size;

  GtkWidget      *button;
  gint            state;
};

struct _EvViewPresenterTimerClass {
  GtkBoxClass parent_class;
};

G_DEFINE_TYPE (EvViewPresenterTimer, ev_view_presenter_timer, GTK_TYPE_BOX)

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

  gtk_label_set_text (GTK_LABEL (self->time),
                      label);

  g_free (label);

  return TRUE;
}

static void
toggle_timer_cb (GtkButton *button,
                 gpointer   data)
{
  EvViewPresenterTimer *self = EV_VIEW_PRESENTER_TIMER (data);

  switch (self->state) {
  case PRESENTATION_RUNNING:
    g_timer_stop (self->timer);
    self->state = PRESENTATION_PAUSED;
    gtk_button_set_label (GTK_BUTTON (self->button),
                          _("Start"));
    break;
  case PRESENTATION_PAUSED:
    g_timer_continue (self->timer);
    self->state = PRESENTATION_RUNNING;
    gtk_button_set_label (GTK_BUTTON (self->button),
                          _("Pause"));
    break;
  default:
    g_error ("You reached an unknown presentation state, this should never happen!\nPlease report it as a bug.");
  }
}

static void
ev_view_presenter_timer_constructed (GObject *obj)
{
  EvViewPresenterTimer *self = EV_VIEW_PRESENTER_TIMER (obj);
  const gchar          *zero = "00:00:00";

  /* timer button */
  self->button = gtk_button_new ();
  g_signal_connect (self->button, "clicked",
                    G_CALLBACK (toggle_timer_cb), self);

  /* timer display */
  self->time = gtk_label_new (zero);

  self->attr_list = pango_attr_list_new ();
  self->weight = pango_attr_weight_new (PANGO_WEIGHT_ULTRABOLD);
  self->size = pango_attr_size_new_absolute (100*PANGO_SCALE);

  pango_attr_list_insert (self->attr_list,
                          self->weight);
  pango_attr_list_insert (self->attr_list,
                          self->size);
  gtk_label_set_attributes (GTK_LABEL (self->time),
                            self->attr_list);

  gtk_widget_add_tick_callback (GTK_WIDGET (self),
                                update_label_cb,
                                self, NULL);
  self->timer = g_timer_new ();

  /* presentation started in paused state */
  self->state = PRESENTATION_PAUSED;
  gtk_button_set_label (GTK_BUTTON (self->button),
                        _("Start"));
  g_timer_stop (self->timer);
  g_timer_reset (self->timer);

  /* packing things */
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (self->button),
                      FALSE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (self->time),
                      TRUE, TRUE, 0);
}

static void
ev_view_presenter_timer_dispose (GObject *obj)
{
  EvViewPresenterTimer *self = EV_VIEW_PRESENTER_TIMER (obj);

  /* calling destroy after gtk_label_set_attributes crashes */
  pango_attribute_destroy (self->weight);
  pango_attribute_destroy (self->size);
  pango_attr_list_unref (self->attr_list);

  g_timer_destroy (self->timer);

  g_object_unref (self->time);
  g_object_unref (self->button);

  G_OBJECT_CLASS (ev_view_presenter_timer_parent_class)->dispose (obj);
}

static void
ev_view_presenter_timer_class_init (EvViewPresenterTimerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = ev_view_presenter_timer_constructed;
  gobject_class->dispose = ev_view_presenter_timer_dispose;
}

GtkWidget *
ev_view_presenter_timer_new (void)
{
  return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER_TIMER, NULL));
}
