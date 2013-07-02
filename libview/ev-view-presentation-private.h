/* ev-view-presentation-private.h
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

#ifndef __EV_VIEW_PRESENTATION_PRIVATE_H__
#define __EV_VIEW_PRESENTATION_PRIVATE_H__

#include "ev-view-presentation.h"

typedef enum {
	EV_PRESENTATION_NORMAL,
	EV_PRESENTATION_BLACK,
	EV_PRESENTATION_WHITE,
	EV_PRESENTATION_END
} EvPresentationState;

EvDocument         *ev_view_presentation_get_document        (EvViewPresentation *pview);
EvPresentationState ev_view_presentation_get_state           (EvViewPresentation *pview);
gdouble             ev_view_presentation_get_scale           (EvViewPresentation *pview);
void                ev_view_presentation_update_current_page (EvViewPresentation *pview,
                                                              guint               page);



#endif /* __EV_VIEW_PRESENTATION_PRIVATE_H__ */
