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
#include "ev-jobs.h"
#include "ev-job-scheduler.h"

enum {
        PROP_0,
        PROP_DOCUMENT,
        PROP_CURRENT_PAGE,
        PROP_ROTATION,
};

enum {
        CHANGE_PAGE,
        FINISHED,
        EIGNAL_EXTERNAL_LINK,
        N_SIGNALS
};

typedef enum {
        CURR_SLIDE,
        NEXT_SLIDE,
        N_SLIDES
} EvSlideCount;

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
        cairo_surface_t    *curr_slide_surface;
        cairo_surface_t    *next_slide_surface;

        EvJob              *prev_job[N_SLIDES];
        EvJob              *curr_job[N_SLIDES];
        EvJob              *next_job[N_SLIDES];
};

struct _EvViewPresenterClass
{
        GtkWidgetClass        base_class;

        void (* change_page) (EvViewPresenter *self,
                              GtkScrollType    scroll);
        void (* finished)    (EvViewPresenter *self);
};

static guint signals[N_SIGNALS] = { 0 }; 

static gdouble ev_view_presenter_get_scale_for_page (EvViewPresenter *self,
                                                     gint             page);


G_DEFINE_TYPE (EvViewPresenter, ev_view_presenter, GTK_TYPE_WIDGET)

static void
ev_view_presenter_init (EvViewPresenter *self)
{
        gtk_widget_set_can_focus (GTK_WIDGET (self), TRUE);
        self->is_constructing = TRUE;
}

static gboolean
ev_view_presenter_job_is_current (EvViewPresenter *self,
                                  EvJob           *job)
{
        gboolean     current = FALSE;
        EvSlideCount slide;

        for (slide = 0; slide < N_SLIDES; slide++)
                if (self->curr_job[slide] == job)
                        current = TRUE;

        return current;
}

static void
job_finished_cb (EvJob           *job,
                 EvViewPresenter *presenter)
{
        if (ev_view_presenter_job_is_current (presenter, job))
                gtk_widget_queue_draw (GTK_WIDGET (presenter));
}

static EvJob *
ev_view_presenter_schedule_new_job (EvViewPresenter *self,
                                    gint             page,
                                    EvJobPriority    priority,
                                    EvSlideCount     slide)
{
        EvJob  *job;
        gdouble scale;

        if (page < 0 || page >= ev_document_get_n_pages (self->document))
                return FALSE;

        scale = ev_view_presenter_get_scale_for_page (self, page);
        job = ev_job_render_new (self->document,
                                 page + slide,
                                 self->rotation,
                                 scale, 0, 0);
        g_signal_connect (job, "finished",
                          G_CALLBACK (job_finished_cb),
                          self);
        ev_job_scheduler_push_job (job, priority);

        return job;
}

static void
ev_view_presenter_delete_job (EvViewPresenter *self,
                              EvJob           *job)
{
        if (!job)
                return;

        /* when done with signals about finished jobs we should
         * disconnect it here */
        ev_job_cancel (job);
        g_object_unref (job);
}
 
static gboolean
ev_view_presenter_surfaces_ready (EvViewPresenter  *self,
                                  EvJob            *jobs[])
{
        gboolean     ready = TRUE;
        EvSlideCount slide;

        for (slide = 0; slide < N_SLIDES; slide++)
                if (!EV_JOB_RENDER (jobs[slide])->surface)
                        ready = FALSE;

        return ready;
}

static void
ev_view_presenter_update_current_page (EvViewPresenter *self,
                                       gint             page)
{
        gint         jump;
        EvSlideCount slide;

        if (page < 0 || page >= ev_document_get_n_pages (self->document))
                return;

        jump = page - self->current_page;

        switch (jump) {
        case -1:
                /* for (slide = 0; slide < N_SLIDES; slide ++) { */
                        ev_view_presenter_delete_job (self, 
                                                      self->next_job[CURR_SLIDE]);
                        
                        self->next_job[CURR_SLIDE] = self->curr_job[CURR_SLIDE];
                        self->curr_job[CURR_SLIDE] = self->prev_job[CURR_SLIDE];

                        if (!self->curr_job[CURR_SLIDE])
                                self->curr_job[CURR_SLIDE] = 
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_URGENT,
                                                                            CURR_SLIDE);
                        else
                                ev_job_scheduler_update_job (self->curr_job[CURR_SLIDE],
                                                            EV_JOB_PRIORITY_URGENT);

                        self->prev_job[CURR_SLIDE] =
                                ev_view_presenter_schedule_new_job (self,
                                                                    page - 1,
                                                                    EV_JOB_PRIORITY_HIGH,
                                                                    CURR_SLIDE);

                        ev_job_scheduler_update_job (self->next_job[CURR_SLIDE],
                                                     EV_JOB_PRIORITY_LOW);
                /* } */
                        ev_view_presenter_delete_job (self, 
                                                      self->next_job[NEXT_SLIDE]);
                        
                        self->next_job[NEXT_SLIDE] = self->curr_job[NEXT_SLIDE];
                        self->curr_job[NEXT_SLIDE] = self->prev_job[NEXT_SLIDE];

                        if (!self->curr_job[NEXT_SLIDE])
                                self->curr_job[NEXT_SLIDE] = 
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_URGENT,
                                                                            NEXT_SLIDE);
                        else
                                ev_job_scheduler_update_job (self->curr_job[NEXT_SLIDE],
                                                            EV_JOB_PRIORITY_URGENT);

                        self->prev_job[NEXT_SLIDE] =
                                ev_view_presenter_schedule_new_job (self,
                                                                    page - 1,
                                                                    EV_JOB_PRIORITY_HIGH,
                                                                    NEXT_SLIDE);

                        ev_job_scheduler_update_job (self->next_job[NEXT_SLIDE],
                                                     EV_JOB_PRIORITY_LOW);

                break;
        case 0:
                /* for (slide = 0; slide < N_SLIDES; slide++) { */
                        if (!self->curr_job[CURR_SLIDE])
                                self->curr_job[CURR_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_URGENT,
                                                                            CURR_SLIDE);

                        if (!self->next_job[CURR_SLIDE])
                                self->next_job[CURR_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_HIGH,
                                                                            CURR_SLIDE);

                        if (!self->prev_job[CURR_SLIDE])
                                self->prev_job[CURR_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_LOW,
                                                                            CURR_SLIDE);
                /* } */
                        if (!self->curr_job[NEXT_SLIDE])
                                self->curr_job[NEXT_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_URGENT,
                                                                            NEXT_SLIDE);

                        if (!self->next_job[NEXT_SLIDE])
                                self->next_job[NEXT_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_HIGH,
                                                                            NEXT_SLIDE);

                        if (!self->prev_job[NEXT_SLIDE])
                                self->prev_job[NEXT_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page,
                                                                            EV_JOB_PRIORITY_LOW,
                                                                            NEXT_SLIDE);

                break;
        case 1:
                /* for (slide = 0; slide < N_SLIDES; slide++) { */
                        ev_view_presenter_delete_job (self, self->prev_job[CURR_SLIDE]);
                        self->prev_job[CURR_SLIDE] = self->curr_job[CURR_SLIDE];
                        self->curr_job[CURR_SLIDE] = self->next_job[CURR_SLIDE];

                        if (!self->curr_job[CURR_SLIDE])
                                self->curr_job[CURR_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page + 1,
                                                                            EV_JOB_PRIORITY_URGENT,
                                                                            CURR_SLIDE);
                        else
                                ev_job_scheduler_update_job (self->curr_job[CURR_SLIDE],
                                                             EV_JOB_PRIORITY_URGENT);
                        self->next_job[CURR_SLIDE] =
                                ev_view_presenter_schedule_new_job (self,
                                                                    page,
                                                                    EV_JOB_PRIORITY_HIGH,
                                                                    CURR_SLIDE);
                        ev_job_scheduler_update_job (self->prev_job[CURR_SLIDE], EV_JOB_PRIORITY_LOW);
                /* } */
                        ev_view_presenter_delete_job (self, self->prev_job[NEXT_SLIDE]);
                        self->prev_job[NEXT_SLIDE] = self->curr_job[NEXT_SLIDE];
                        self->curr_job[NEXT_SLIDE] = self->next_job[NEXT_SLIDE];

                        if (!self->curr_job[NEXT_SLIDE])
                                self->curr_job[NEXT_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page + 1,
                                                                            EV_JOB_PRIORITY_URGENT,
                                                                            NEXT_SLIDE);
                        else
                                ev_job_scheduler_update_job (self->curr_job[NEXT_SLIDE],
                                                             EV_JOB_PRIORITY_URGENT);
                        self->next_job[NEXT_SLIDE] =
                                ev_view_presenter_schedule_new_job (self,
                                                                    page,
                                                                    EV_JOB_PRIORITY_HIGH,
                                                                    NEXT_SLIDE);
                        ev_job_scheduler_update_job (self->prev_job[NEXT_SLIDE], EV_JOB_PRIORITY_LOW);

                break;
        default:
                /* for (slide = 0; slide < N_SLIDES; slide++) { */
                        ev_view_presenter_delete_job (self, self->prev_job[NEXT_SLIDE]);
                        ev_view_presenter_delete_job (self, self->curr_job[NEXT_SLIDE]);
                        ev_view_presenter_delete_job (self, self->next_job[NEXT_SLIDE]);

                        self->curr_job[NEXT_SLIDE] = 
                                ev_view_presenter_schedule_new_job (self,
                                                                    page,
                                                                    EV_JOB_PRIORITY_URGENT,
                                                                    NEXT_SLIDE);

                        if (jump > 0) {
                                self->next_job[NEXT_SLIDE] = 
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page + 1,
                                                                            EV_JOB_PRIORITY_HIGH,
                                                                            NEXT_SLIDE);
                                self->prev_job[NEXT_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page - 1,
                                                                            EV_JOB_PRIORITY_LOW,
                                                                            NEXT_SLIDE);
                        } else {
                                self->next_job[NEXT_SLIDE] = 
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page - 1,
                                                                            EV_JOB_PRIORITY_HIGH,
                                                                            NEXT_SLIDE);
                                self->prev_job[NEXT_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page + 1,
                                                                            EV_JOB_PRIORITY_LOW,
                                                                            NEXT_SLIDE);
                        }
                /* } */
                        ev_view_presenter_delete_job (self, self->prev_job[CURR_SLIDE]);
                        ev_view_presenter_delete_job (self, self->curr_job[CURR_SLIDE]);
                        ev_view_presenter_delete_job (self, self->next_job[CURR_SLIDE]);

                        self->curr_job[CURR_SLIDE] = 
                                ev_view_presenter_schedule_new_job (self,
                                                                    page,
                                                                    EV_JOB_PRIORITY_URGENT,
                                                                    CURR_SLIDE);

                        if (jump > 0) {
                                self->next_job[CURR_SLIDE] = 
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page + 1,
                                                                            EV_JOB_PRIORITY_HIGH,
                                                                            CURR_SLIDE);
                                self->prev_job[CURR_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page - 1,
                                                                            EV_JOB_PRIORITY_LOW,
                                                                            CURR_SLIDE);
                        } else {
                                self->next_job[CURR_SLIDE] = 
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page - 1,
                                                                            EV_JOB_PRIORITY_HIGH,
                                                                            CURR_SLIDE);
                                self->prev_job[CURR_SLIDE] =
                                        ev_view_presenter_schedule_new_job (self,
                                                                            page + 1,
                                                                            EV_JOB_PRIORITY_LOW,
                                                                            CURR_SLIDE);
                        }

        }

        if (self->current_page != page) {
                self->current_page = page;
                g_object_notify (G_OBJECT (self), "current-paege");
        }

        if (ev_view_presenter_surfaces_ready (self, self->curr_job))
                gtk_widget_queue_draw (GTK_WIDGET (self));
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
ev_view_presenter_dispose (GObject *obj)
{
        EvViewPresenter *presenter = EV_VIEW_PRESENTER (obj);

        if (presenter->document)
        {
                g_object_unref (presenter->document);
                presenter->document = NULL;
        }

        G_OBJECT_CLASS (ev_view_presenter_parent_class)->dispose (obj);
}

static GObject *
ev_view_presenter_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_params)
{
        GObject         *obj;
 
        obj = G_OBJECT_CLASS (ev_view_presenter_parent_class)->constructor (type,
                                                                            n_construct_properties,
                                                                            construct_params);
 
       return obj;
}

static void
ev_view_presenter_set_current_page (EvViewPresenter *self,
                                    guint            new_page)
{
        if (self->current_page == new_page)
                return;

        ev_view_presenter_update_current_page (self, new_page);
}

void
ev_view_presenter_set_rotation (EvViewPresenter *self,
                                gint             rotation)
{
        if (rotation >= 360)
                rotation %= 360;
        else if (rotation < 0)
                rotation = (rotation % 360) + 360;

        if (self->rotation == rotation)
                return;

        self->rotation = rotation;

        if (self->is_constructing)
                return;

        self->scale = 0;
        ev_view_presenter_update_current_page (self,
                                               self->current_page);
}

static void
ev_view_presenter_set_property (GObject      *obj,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{

        EvViewPresenter *presenter = EV_VIEW_PRESENTER (obj);

        switch (prop_id) {
        case PROP_DOCUMENT:
                presenter->document = g_value_dup_object (value);
                break;
        case PROP_CURRENT_PAGE:
                ev_view_presenter_set_current_page (presenter,
                                                    g_value_get_uint (value));
                break;
        case PROP_ROTATION:
                ev_view_presenter_set_rotation (presenter,
                                           g_value_get_int (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
        }
}

static void
ev_view_presenter_get_property (GObject    *obj,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
        EvViewPresenter *presenter = EV_VIEW_PRESENTER (obj);

        switch (prop_id) {
        case PROP_CURRENT_PAGE:
                g_value_set_uint (value, presenter->current_page);
                break;
        case PROP_ROTATION:
                g_value_set_int (value, presenter->rotation);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
        }
}

static void
ev_view_presenter_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
        *minimum = *natural = 0;
}

static void
ev_view_presenter_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum,
                                        gint      *natural)
{
        *minimum = *natural = 0;
}

static gboolean
init_presenter (GtkWidget *widget)
{
        EvViewPresenter *presenter = EV_VIEW_PRESENTER (widget);
        GdkScreen       *screen = gtk_widget_get_screen (widget);
        GdkRectangle     monitor;
        gint             monitor_num;

        monitor_num = gdk_screen_get_monitor_at_window (screen,
                                                        gtk_widget_get_window (widget));
        gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);
        presenter->monitor_width = monitor.width;
        presenter->monitor_height = monitor.height;

        ev_view_presenter_update_current_page (presenter, presenter->current_page);

        return FALSE;
}

static void
ev_view_presenter_realize (GtkWidget *widget)
{
        /*cut and paste from ev-view-presentation.c */

	GdkWindow     *window;
	GdkWindowAttr  attributes;
	GtkAllocation  allocation;

	gtk_widget_set_realized (widget, TRUE);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);

	gtk_widget_get_allocation (widget, &allocation);
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.event_mask = GDK_EXPOSURE_MASK |
		GDK_BUTTON_PRESS_MASK |
		GDK_BUTTON_RELEASE_MASK |
		GDK_SCROLL_MASK |
		GDK_KEY_PRESS_MASK |
		GDK_POINTER_MOTION_MASK |
		GDK_POINTER_MOTION_HINT_MASK |
		GDK_ENTER_NOTIFY_MASK |
		GDK_LEAVE_NOTIFY_MASK;

	window = gdk_window_new (gtk_widget_get_parent_window (widget),
				 &attributes,
				 GDK_WA_X | GDK_WA_Y |
				 GDK_WA_VISUAL);

	gdk_window_set_user_data (window, widget);
	gtk_widget_set_window (widget, window);
        gtk_style_context_set_background (gtk_widget_get_style_context (widget),
                                          window);

	g_idle_add ((GSourceFunc)init_presenter, widget);
}

static void
ev_view_presenter_update_curr_slide_surface (EvViewPresenter *self,
                                             cairo_surface_t *surface)
{
        if (!surface || self->curr_slide_surface == surface)
                return;

        /* ref count ++ */
        cairo_surface_reference (surface);
        if (self->curr_slide_surface)
                /* ref count -- */
                cairo_surface_destroy (self->curr_slide_surface);
        self->curr_slide_surface = surface;
}

static void
ev_view_presenter_update_next_slide_surface (EvViewPresenter *self,
                                             cairo_surface_t *surface)
{
        if (!surface || self->next_slide_surface == surface)
                return;

        /* ref count ++ */
        cairo_surface_reference (surface);
        if (self->next_slide_surface)
                /* ref count -- */
                cairo_surface_destroy (self->next_slide_surface);
        self->next_slide_surface = surface;
}

static gdouble
ev_view_presenter_get_scale_for_page (EvViewPresenter *self,
                                      gint             page)
{
	if (!ev_document_is_page_size_uniform (self->document) || self->scale == 0) {
		gdouble width, height;

		ev_document_get_page_size (self->document, page, &width, &height);
		if (self->rotation == 90 || self->rotation == 270) {
			gdouble tmp;

			tmp = width;
			width = height;
			height = tmp;
		}
		self->scale = MIN ((self->monitor_width / 2) / width, (self->monitor_height / 2) / height);
	}

	return self->scale;
}


static void
ev_view_presenter_get_page_area_curr_slide (EvViewPresenter *self,
                                            GdkRectangle    *page_area)
{
        GtkWidget    *widget = GTK_WIDGET (self);
        GtkAllocation allocation;
        gdouble       doc_width, doc_height;
        gint          view_width, view_height;
        gdouble       scale;

        ev_document_get_page_size (self->document,
                                   self->current_page,
                                   &doc_width, &doc_height);
        scale = ev_view_presenter_get_scale_for_page (self,
                                                      self->current_page);


	if (self->rotation == 90 || self->rotation == 270) {
		view_width = (gint)((doc_height * scale) + 0.5);
		view_height = (gint)((doc_width * scale) + 0.5);
	} else {
		view_width = (gint)((doc_width * scale) + 0.5);
		view_height = (gint)((doc_height * scale) + 0.5);
	}

	gtk_widget_get_allocation (widget, &allocation);

	page_area->x = 0;//(MAX (0, allocation.width - view_width)) / 2;
	page_area->y = 0;//(MAX (0, allocation.height - view_height)) / 2;
	page_area->width = view_width;
	page_area->height = view_height;
}

static void
ev_view_presenter_get_page_area_next_slide (EvViewPresenter *self,
                                            GdkRectangle    *page_area)
{
        GtkWidget    *widget = GTK_WIDGET (self);
        GtkAllocation allocation;
        gdouble       doc_width, doc_height;
        gint          view_width, view_height;
        gdouble       scale;

        ev_document_get_page_size (self->document,
                                   self->current_page + 1,
                                   &doc_width, &doc_height);
        scale = ev_view_presenter_get_scale_for_page (self,
                                                      self->current_page + 1);


	if (self->rotation == 90 || self->rotation == 270) {
		view_width = (gint)((doc_height * scale) + 0.5);
		view_height = (gint)((doc_width * scale) + 0.5);
	} else {
		view_width = (gint)((doc_width * scale) + 0.5);
		view_height = (gint)((doc_height * scale) + 0.5);
	}

	gtk_widget_get_allocation (widget, &allocation);

	page_area->x = 0;//(MAX (0, allocation.width - view_width)) / 2;
	page_area->y = allocation.height / 2;//(MAX (0, allocation.height - view_height)) / 2;
	page_area->width = view_width;
	page_area->height = view_height;
}

static void
ev_view_presenter_reset_jobs (EvViewPresenter *self)
{
        EvSlideCount slide;

        for (slide=0; slide < N_SLIDES; slide++) {
                if (self->curr_job[slide]) {
                        ev_view_presenter_delete_job (self, self->curr_job[slide]);
                        self->curr_job[slide] = NULL;
                }

                if (self->prev_job[slide]) {
                        ev_view_presenter_delete_job (self, self->prev_job[slide]);
                        self->prev_job[slide] = NULL;
                }

                if (self->next_job[slide]) {
                        ev_view_presenter_delete_job (self, self->next_job[slide]);
                        self->next_job[slide] = NULL;
                }
        }
}

static gboolean
ev_view_presenter_draw (GtkWidget *widget,
                        cairo_t   *cr)
{
        EvViewPresenter *presenter = EV_VIEW_PRESENTER (widget);
        GdkRectangle     page_area;
        GdkRectangle     overlap;
        cairo_surface_t *curr_slide_s;
        cairo_surface_t *next_slide_s;
        GdkRectangle     clip_rect;

        if (!gdk_cairo_get_clip_rectangle (cr, &clip_rect))
                return FALSE;

        curr_slide_s = presenter->curr_job[CURR_SLIDE] ?
                EV_JOB_RENDER (presenter->curr_job[CURR_SLIDE])->surface : NULL;

        next_slide_s = presenter->curr_job[NEXT_SLIDE] ?
                EV_JOB_RENDER (presenter->curr_job[NEXT_SLIDE])->surface : NULL;

        if (curr_slide_s)
                ev_view_presenter_update_curr_slide_surface (presenter,
                                                             curr_slide_s);
        else if (presenter->curr_slide_surface)
                curr_slide_s = presenter->curr_slide_surface;

        if (next_slide_s)
                ev_view_presenter_update_next_slide_surface (presenter,
                                                             next_slide_s);
        else if (presenter->next_slide_surface)
                next_slide_s = presenter->next_slide_surface;

        if (!curr_slide_s && !next_slide_s)
                return FALSE;

        if (curr_slide_s) {
                ev_view_presenter_get_page_area_curr_slide (presenter,
                                                                &page_area);
                if (gdk_rectangle_intersect (&page_area, &clip_rect, &overlap)) {
                        cairo_save (cr);

                        if (overlap.width == page_area.width)
                                overlap.width--;

                        cairo_rectangle (cr, overlap.x, overlap.y, overlap.width, overlap.height);
                        cairo_set_source_surface (cr, curr_slide_s, page_area.x, page_area.y);
                        cairo_fill (cr);
                        cairo_restore (cr);
                }
        }

        if (next_slide_s) {
                ev_view_presenter_get_page_area_next_slide (presenter,
                                                                &page_area);
                if (gdk_rectangle_intersect (&page_area, &clip_rect, &overlap)) {
                        cairo_save (cr);

                        if (overlap.width == page_area.width)
                                overlap.width--;

                        cairo_rectangle (cr, overlap.x, overlap.y, overlap.width, overlap.height);
                        cairo_set_source_surface (cr, next_slide_s, page_area.x, page_area.y);
                        cairo_fill (cr);
                        cairo_restore (cr);
                }
        }

        return FALSE;
}

static gboolean
ev_view_presenter_key_press_event (GtkWidget   *widget,
                                   GdkEventKey *event)
{
        EvViewPresenter *presenter = EV_VIEW_PRESENTER (widget);

        switch (event->keyval) {
        case GDK_KEY_Home:
                ev_view_presenter_update_current_page (presenter, 0);
                return TRUE;
        case GDK_KEY_End:
                ev_view_presenter_update_current_page (
                        presenter,
                        ev_document_get_n_pages (
                                presenter->document) - 1);
                return TRUE;
        default:
                break;
        }

        return gtk_bindings_activate_event (G_OBJECT (widget),
                                            event);
}

static gboolean
ev_view_presenter_scroll_event (GtkWidget      *widget,
                                GdkEventScroll *scroll)
{
        EvViewPresenter *presenter = EV_VIEW_PRESENTER (widget);
        guint            state;

        state = scroll->state & gtk_accelerator_get_default_mod_mask ();
        if (state != 0)
                return FALSE;

        switch (scroll->direction) {
        case GDK_SCROLL_DOWN:
        case GDK_SCROLL_RIGHT:
                ev_view_presenter_change_page (presenter,
                                               GTK_SCROLL_PAGE_FORWARD);
                break;
        case GDK_SCROLL_UP:
        case GDK_SCROLL_LEFT:
                ev_view_presenter_change_page (presenter,
                                               GTK_SCROLL_PAGE_BACKWARD);
                break;
        case GDK_SCROLL_SMOOTH:
                return FALSE;
        }

        return TRUE;
}

static void
add_change_page_binding_keypad (GtkBindingSet  *binding_set,
                                guint           keyval,
                                GdkModifierType modifiers,
                                GtkScrollType   scroll)
{
	guint keypad_keyval = keyval - GDK_KEY_Left + GDK_KEY_KP_Left;

	gtk_binding_entry_add_signal (binding_set, keyval, modifiers,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, scroll);
	gtk_binding_entry_add_signal (binding_set, keypad_keyval, modifiers,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, scroll);
}

static void
ev_view_presenter_class_init (EvViewPresenterClass *klass)
{
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
        GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
        GtkBindingSet  *binding_set;
        GtkCssProvider *provider;

        klass->change_page = ev_view_presenter_change_page;

        gobject_class->dispose = ev_view_presenter_dispose;
        gobject_class->constructor = ev_view_presenter_constructor;
        gobject_class->set_property = ev_view_presenter_set_property;
        gobject_class->get_property = ev_view_presenter_get_property;

        widget_class->get_preferred_width = ev_view_presenter_get_preferred_width;
        widget_class->get_preferred_height = ev_view_presenter_get_preferred_height; 
        widget_class->realize = ev_view_presenter_realize;
        widget_class->draw = ev_view_presenter_draw;
        widget_class->key_press_event = ev_view_presenter_key_press_event;
        widget_class->scroll_event = ev_view_presenter_scroll_event;

        g_object_class_install_property (gobject_class,
                                         PROP_DOCUMENT,
                                         g_param_spec_object ("document",
                                                              "Document",
                                                              "Document",
                                                              EV_TYPE_DOCUMENT,
                                                              G_PARAM_WRITABLE |
                                                              G_PARAM_CONSTRUCT_ONLY |
                                                              G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (gobject_class,
                                         PROP_CURRENT_PAGE,
                                         g_param_spec_uint ("current-page",
                                                            "Current Page",
                                                            "The current page",
                                                            0, G_MAXUINT, 0,
                                                            G_PARAM_READWRITE |
                                                            G_PARAM_CONSTRUCT |
                                                            G_PARAM_STATIC_STRINGS));
        g_object_class_install_property (gobject_class,
                                         PROP_ROTATION,
                                         g_param_spec_int ("rotation",
                                                           "Rotation",
                                                           "Current rotation angle",
                                                           0, 360, 0,
                                                           G_PARAM_READWRITE |
                                                           G_PARAM_CONSTRUCT |
                                                           G_PARAM_STATIC_STRINGS));
	signals[CHANGE_PAGE] =
		g_signal_new ("change_page",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvViewPresenterClass, change_page),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__ENUM,
			      G_TYPE_NONE, 1,
			      GTK_TYPE_SCROLL_TYPE);
	signals[FINISHED] =
		g_signal_new ("finished",
			      G_OBJECT_CLASS_TYPE (gobject_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (EvViewPresenterClass, finished),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0,
			      G_TYPE_NONE);

	binding_set = gtk_binding_set_by_class (klass);
	add_change_page_binding_keypad (binding_set, GDK_KEY_Left,  0, GTK_SCROLL_PAGE_BACKWARD);
	add_change_page_binding_keypad (binding_set, GDK_KEY_Right, 0, GTK_SCROLL_PAGE_FORWARD);
	add_change_page_binding_keypad (binding_set, GDK_KEY_Up,    0, GTK_SCROLL_PAGE_BACKWARD);
	add_change_page_binding_keypad (binding_set, GDK_KEY_Down,  0, GTK_SCROLL_PAGE_FORWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_space, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_FORWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_BackSpace, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_BACKWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Page_Down, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_FORWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_Page_Up, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_BACKWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_J, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_FORWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_H, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_BACKWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_L, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_FORWARD);
	gtk_binding_entry_add_signal (binding_set, GDK_KEY_K, 0,
				      "change_page", 1,
				      GTK_TYPE_SCROLL_TYPE, GTK_SCROLL_PAGE_BACKWARD);

        provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_data (provider,
                                         "EvViewPresenter {\n"
                                         " background-color: black; }",
                                         -1, NULL);
        gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                                   GTK_STYLE_PROVIDER (provider),
                                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref (provider);
}

GtkWidget *
ev_view_presenter_new (EvDocument *document,
                       guint       current_page,
                       guint       rotation)
{
        g_return_val_if_fail (EV_IS_DOCUMENT (document), NULL);
        g_return_val_if_fail (current_page < ev_document_get_n_pages (document), NULL);

        return GTK_WIDGET (g_object_new (EV_TYPE_VIEW_PRESENTER,
                                         "document", document,
                                         "current_page", current_page,
                                         "rotation", rotation, NULL));
}
