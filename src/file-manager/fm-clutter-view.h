/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-clutter-view.h - interface for clutter view of directory.

   Copyright (C) 2006 Free Software Foundation, Inc.
   
   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Authors: Christian Neumair <chris@gnome-de.org>
*/

#ifndef FM_CLUTTER_VIEW_H
#define FM_CLUTTER_VIEW_H

#include "fm-directory-view.h"

#define GNOME_DESKTOP_USE_UNSTABLE_API

#include <libgnomeui/gnome-desktop-thumbnail.h>

#define FM_TYPE_CLUTTER_VIEW fm_clutter_view_get_type()
#define FM_CLUTTER_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), FM_TYPE_CLUTTER_VIEW, FMClutterView))
#define FM_CLUTTER_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), FM_TYPE_CLUTTER_VIEW, FMClutterViewClass))
#define FM_IS_CLUTTER_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FM_TYPE_CLUTTER_VIEW))
#define FM_IS_CLUTTER_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), FM_TYPE_CLUTTER_VIEW))
#define FM_CLUTTER_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), FM_TYPE_CLUTTER_VIEW, FMClutterViewClass))

#define FM_CLUTTER_VIEW_ID "OAFIID:Nautilus_File_Manager_Clutter_View"

typedef struct FMClutterViewDetails FMClutterViewDetails;

typedef struct {
	FMDirectoryView parent_instance;
	FMClutterViewDetails *details;
} FMClutterView;

typedef struct {
	FMDirectoryViewClass parent_class;
} FMClutterViewClass;

GType fm_clutter_view_get_type (void);
void  fm_clutter_view_register (void);

#endif /* FM_CLUTTER_VIEW_H */
