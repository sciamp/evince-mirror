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
  PRESENTATION_IDLE,
  PRESENTATION_RUNNING,
  PRESENTATION_PAUSED
};

enum {
  PRESENTATION_STARTED,
  N_SIGNALS
};

struct _EvViewPresenterTimer {
  GtkBox          parent_instance;

  GTimer         *timer;
  GtkWidget      *time;
  GtkWidget      *slide_time;
  gint            last_slide_time;
  PangoAttrList  *attr_list;
  PangoAttribute *weight;
  PangoAttribute *size;
  GtkWidget      *time_box;

  GtkWidget      *toggle_button;
  gint            state;
};

struct _EvViewPresenterTimerClass {
  GtkBoxClass parent_class;
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (EvViewPresenterTimer, ev_view_presenter_timer, GTK_TYPE_BOX)

static void
ev_view_presenter_timer_init (EvViewPresenterTimer *self)
{
}

void
ev_view_presenter_timer_reset_slide_time (EvViewPresenterTimer *self)
{
  self->last_slide_time = (int) g_timer_elapsed (self->timer, NULL);
}

static gboolean
update_label_cb (GtkWidget     *widget,
                 GdkFrameClock *fc,
                 gpointer       user_data)
{
  EvViewPresenterTimer *self = EV_VIEW_PRESENTER_TIMER (user_data);
  gdouble               elapsed;
  gint                  presentation;
  gint                  slide;
  gint                  h;
  gint                  m;
  gint                  s;
  gchar                *label;

  elapsed = g_timer_elapsed (self->timer, NULL);
  presentation = (int) elapsed;
  slide = presentation - self->last_slide_time;

  h = (int) presentation / 3600;
  presentation %= 3600;
  m = (int) presentation / 60;
  presentation %= 60;
  s = (int) presentation;

  label = g_strdup_printf ("%02d:%02d:%02d",
                           h, m, s);

  gtk_label_set_text (GTK_LABEL (self->time),
                      label);

  h = (int) slide / 3600;
  slide %= 3600;
  m = (int) slide / 60;
  slide %= 60;
  s = (int) slide;

  label = g_strdup_printf ("%02d:%02d:%02d",
                           h, m, s);

  gtk_label_set_text (GTK_LABEL (self->slide_time),
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
    gtk_button_set_label (GTK_BUTTON (self->toggle_button),
                          _("Start"));
    break;
  case PRESENTATION_IDLE:
    g_signal_emit_by_name (self, "presentation-started");
  case PRESENTATION_PAUSED:
    g_timer_continue (self->timer);
    self->state = PRESENTATION_RUNNING;
    gtk_button_set_label (GTK_BUTTON (self->toggle_button),
                          _("Pause"));
    break;
  default:
    g_assert_not_reached ();
  }
}

static void
ev_view_presenter_timer_constructed (GObject *obj)
{
  EvViewPresenterTimer *self = EV_VIEW_PRESENTER_TIMER (obj);
  const gchar          *zero = "00:00:00";
  GtkCssProvider       *provider;

  /* timer buttons */
  self->toggle_button = gtk_button_new ();
  g_signal_connect (self->toggle_button, "clicked",
                    G_CALLBACK (toggle_timer_cb), self);
  gtk_widget_set_halign (self->toggle_button,
                         GTK_ALIGN_CENTER);
  gtk_widget_set_valign (self->toggle_button,
                         GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (self->toggle_button,
                               100, -1);

  /* timer display */
  self->time = gtk_label_new (zero);
  self->slide_time = gtk_label_new (zero);
  self->last_slide_time = 0;

  self->attr_list = pango_attr_list_new ();
  self->weight = pango_attr_weight_new (PANGO_WEIGHT_ULTRABOLD);
  self->size = pango_attr_size_new_absolute (100*PANGO_SCALE);

  pango_attr_list_insert (self->attr_list,
                          self->weight);
  pango_attr_list_insert (self->attr_list,
                          self->size);
  gtk_label_set_attributes (GTK_LABEL (self->time),
                            self->attr_list);

  gtk_label_set_attributes (GTK_LABEL (self->slide_time),
                            self->attr_list);

  self->time_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
                                0);
  gtk_box_pack_start (GTK_BOX (self->time_box),
                      GTK_WIDGET (self->time),
                      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (self->time_box),
                      GTK_WIDGET (self->slide_time),
                      TRUE, TRUE, 0);

  gtk_widget_add_tick_callback (GTK_WIDGET (self),
                                update_label_cb,
                                self, NULL);
  self->timer = g_timer_new ();

  /* presentation started in paused state */
  self->state = PRESENTATION_IDLE;
  gtk_button_set_label (GTK_BUTTON (self->toggle_button),
                        _("Start"));
  g_timer_stop (self->timer);
  g_timer_reset (self->timer);

  /* packing things */
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (self->toggle_button),
                      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (self), GTK_WIDGET (self->time_box),
                      TRUE, TRUE, 0);

  /* style */
  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider,
                                   "GtkWindow {\n"
                                   "  background-color: black; }\n"
                                   "GtkBox > GtkLabel {\n"
                                   "  background-color: black;\n"
                                   "  color: white; }\n"
                                   "GtkButton {\n"
                                   "  font-size: 14px; }",
                                   -1, NULL);
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             GTK_STYLE_PROVIDER (provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);
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
  g_object_unref (self->slide_time);
  g_object_unref (self->time_box);
  g_object_unref (self->toggle_button);

  G_OBJECT_CLASS (ev_view_presenter_timer_parent_class)->dispose (obj);
}

static void
ev_view_presenter_timer_class_init (EvViewPresenterTimerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = ev_view_presenter_timer_constructed;
  gobject_class->dispose = ev_view_presenter_timer_dispose;

  signals[PRESENTATION_STARTED] =
    g_signal_new ("presentation-started",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0,
                  G_TYPE_NONE);
}

GtkWidget *
ev_view_presenter_timer_new (void)
{
  return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER_TIMER, NULL));
}
