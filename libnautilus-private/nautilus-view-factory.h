/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*-

   nautilus-view-factory.h: register and create NautilusViews
 
   Copyright (C) 2004 Red Hat Inc.
  
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
  
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
  
   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
  
   Author: Alexander Larsson <alexl@redhat.com>
*/

#ifndef NAUTILUS_VIEW_FACTORY_H
#define NAUTILUS_VIEW_FACTORY_H

#include <string.h>

#include <libnautilus-private/nautilus-view.h>
#include <libnautilus-private/nautilus-window-info.h>
#include <libgnomevfs/gnome-vfs-file-info.h>

G_BEGIN_DECLS

typedef struct _NautilusViewInfo NautilusViewInfo;

struct _NautilusViewInfo {
	char *id;
	char *label;
	char *label_with_mnemonic;
	NautilusView * (*create) (NautilusWindowInfo *window);
	/* BONOBOTODO: More args here */
	gboolean (*supports_uri) (const char *uri,
				  GnomeVFSFileType file_type,
				  const char *mime_type);
};


void                    nautilus_view_factory_register          (NautilusViewInfo   *view_info);
const NautilusViewInfo *nautilus_view_factory_lookup            (const char         *id);
NautilusView *          nautilus_view_factory_create            (const char         *id,
								 NautilusWindowInfo *window);
gboolean                nautilus_view_factory_view_supports_uri (const char         *id,
								 const char         *uri,
								 GnomeVFSFileType    file_type,
								 const char         *mime_type);
GList *                 nautilus_view_factory_get_views_for_uri (const char         *uri,
								 GnomeVFSFileType    file_type,
								 const char         *mime_type);




G_END_DECLS

#endif /* NAUTILUS_VIEW_FACTORY_H */