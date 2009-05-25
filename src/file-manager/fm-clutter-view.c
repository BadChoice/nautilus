/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/* fm-empty-view.c - implementation of empty view of directory.

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

#include <config.h>
#include "fm-clutter-view.h"

#include <string.h>
#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-view.h>
#include <libnautilus-private/nautilus-view-factory.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-gtk-macros.h>
#include <eel/eel-vfs-extensions.h>

#include <clutter/clutter.h>

struct FMClutterViewDetails {
	int number_of_files;
};

static GList *fm_clutter_view_get_selection                   (FMDirectoryView   *view);
static GList *fm_clutter_view_get_selection_for_file_transfer (FMDirectoryView   *view);
static void   fm_clutter_view_scroll_to_file                  (NautilusView      *view,
							     const char        *uri);
static void   fm_clutter_view_iface_init                      (NautilusViewIface *iface);

G_DEFINE_TYPE_WITH_CODE (FMClutterView, fm_clutter_view, FM_TYPE_DIRECTORY_VIEW, 
			 G_IMPLEMENT_INTERFACE (NAUTILUS_TYPE_VIEW,
						fm_clutter_view_iface_init));

/* for EEL_CALL_PARENT */
#define parent_class fm_clutter_view_parent_class

static void
fm_clutter_view_add_file (FMDirectoryView *view, NautilusFile *file, NautilusDirectory *directory)
{
	static GTimer *timer = NULL;
	static gdouble cumu = 0, elaps;
	FM_CLUTTER_VIEW (view)->details->number_of_files++;
	GdkPixbuf *icon;

	if (!timer) timer = g_timer_new ();

	g_timer_start (timer);
	icon = nautilus_file_get_icon_pixbuf (file, nautilus_get_icon_size_for_zoom_level (NAUTILUS_ZOOM_LEVEL_STANDARD), TRUE, 0);

	elaps = g_timer_elapsed (timer, NULL);
	g_timer_stop (timer);

	g_object_unref (icon);
	
	cumu += elaps;
	g_message ("entire loading: %.3f, cumulative %.3f", elaps, cumu);
}


static void
fm_clutter_view_begin_loading (FMDirectoryView *view)
{
}

static void
fm_clutter_view_clear (FMDirectoryView *view)
{
}


static void
fm_clutter_view_file_changed (FMDirectoryView *view, NautilusFile *file, NautilusDirectory *directory)
{
}

static GtkWidget *
fm_clutter_view_get_background_widget (FMDirectoryView *view)
{
	return GTK_WIDGET (view);
}

static GList *
fm_clutter_view_get_selection (FMDirectoryView *view)
{
	return NULL;
}


static GList *
fm_clutter_view_get_selection_for_file_transfer (FMDirectoryView *view)
{
	return NULL;
}

static guint
fm_clutter_view_get_item_count (FMDirectoryView *view)
{
	return FM_CLUTTER_VIEW (view)->details->number_of_files;
}

static gboolean
fm_clutter_view_is_empty (FMDirectoryView *view)
{
	return FM_CLUTTER_VIEW (view)->details->number_of_files == 0;
}

static void
fm_clutter_view_end_file_changes (FMDirectoryView *view)
{
}

static void
fm_clutter_view_remove_file (FMDirectoryView *view, NautilusFile *file, NautilusDirectory *directory)
{
	FM_CLUTTER_VIEW (view)->details->number_of_files--;
	g_assert (FM_CLUTTER_VIEW (view)->details->number_of_files >= 0);
}

static void
fm_clutter_view_set_selection (FMDirectoryView *view, GList *selection)
{
	fm_directory_view_notify_selection_changed (view);
}

static void
fm_clutter_view_select_all (FMDirectoryView *view)
{
}

static void
fm_clutter_view_reveal_selection (FMDirectoryView *view)
{
}

static void
fm_clutter_view_merge_menus (FMDirectoryView *view)
{
	EEL_CALL_PARENT (FM_DIRECTORY_VIEW_CLASS, merge_menus, (view));
}

static void
fm_clutter_view_update_menus (FMDirectoryView *view)
{
	EEL_CALL_PARENT (FM_DIRECTORY_VIEW_CLASS, update_menus, (view));
}

/* Reset sort criteria and zoom level to match defaults */
static void
fm_clutter_view_reset_to_defaults (FMDirectoryView *view)
{
}

static void
fm_clutter_view_bump_zoom_level (FMDirectoryView *view, int zoom_increment)
{
}

static NautilusZoomLevel
fm_clutter_view_get_zoom_level (FMDirectoryView *view)
{
	return NAUTILUS_ZOOM_LEVEL_STANDARD;
}

static void
fm_clutter_view_zoom_to_level (FMDirectoryView *view,
			    NautilusZoomLevel zoom_level)
{
}

static void
fm_clutter_view_restore_default_zoom_level (FMDirectoryView *view)
{
}

static gboolean 
fm_clutter_view_can_zoom_in (FMDirectoryView *view) 
{
	return FALSE;
}

static gboolean 
fm_clutter_view_can_zoom_out (FMDirectoryView *view) 
{
	return FALSE;
}

static void
fm_clutter_view_start_renaming_file (FMDirectoryView *view,
				  NautilusFile *file,
				  gboolean select_all)
{
}

static void
fm_clutter_view_click_policy_changed (FMDirectoryView *directory_view)
{
}


static int
fm_clutter_view_compare_files (FMDirectoryView *view, NautilusFile *file1, NautilusFile *file2)
{
	if (file1 < file2) {
		return -1;
	}

	if (file1 > file2) {
		return +1;
	}

	return 0;
}

static gboolean
fm_clutter_view_using_manual_layout (FMDirectoryView *view)
{
	return FALSE;
}

static void
fm_clutter_view_end_loading (FMDirectoryView *view,
			   gboolean all_files_seen)
{
}

static void
fm_clutter_view_finalize (GObject *object)
{
	FMClutterView *empty_view;

	empty_view = FM_CLUTTER_VIEW (object);
	g_free (empty_view->details);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
fm_clutter_view_emblems_changed (FMDirectoryView *directory_view)
{
}

static char *
fm_clutter_view_get_first_visible_file (NautilusView *view)
{
	return NULL;
}

static void
fm_clutter_view_scroll_to_file (NautilusView *view,
			      const char *uri)
{
}

static void
fm_clutter_view_grab_focus (NautilusView *view)
{
	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
fm_clutter_view_sort_directories_first_changed (FMDirectoryView *view)
{
}

static void
fm_clutter_view_class_init (FMClutterViewClass *class)
{
	FMDirectoryViewClass *fm_directory_view_class;

	fm_directory_view_class = FM_DIRECTORY_VIEW_CLASS (class);

	G_OBJECT_CLASS (class)->finalize = fm_clutter_view_finalize;

	fm_directory_view_class->add_file = fm_clutter_view_add_file;
	fm_directory_view_class->begin_loading = fm_clutter_view_begin_loading;
	fm_directory_view_class->bump_zoom_level = fm_clutter_view_bump_zoom_level;
	fm_directory_view_class->can_zoom_in = fm_clutter_view_can_zoom_in;
	fm_directory_view_class->can_zoom_out = fm_clutter_view_can_zoom_out;
        fm_directory_view_class->click_policy_changed = fm_clutter_view_click_policy_changed;
	fm_directory_view_class->clear = fm_clutter_view_clear;
	fm_directory_view_class->file_changed = fm_clutter_view_file_changed;
	fm_directory_view_class->get_background_widget = fm_clutter_view_get_background_widget;
	fm_directory_view_class->get_selection = fm_clutter_view_get_selection;
	fm_directory_view_class->get_selection_for_file_transfer = fm_clutter_view_get_selection_for_file_transfer;
	fm_directory_view_class->get_item_count = fm_clutter_view_get_item_count;
	fm_directory_view_class->is_empty = fm_clutter_view_is_empty;
	fm_directory_view_class->remove_file = fm_clutter_view_remove_file;
	fm_directory_view_class->merge_menus = fm_clutter_view_merge_menus;
	fm_directory_view_class->update_menus = fm_clutter_view_update_menus;
	fm_directory_view_class->reset_to_defaults = fm_clutter_view_reset_to_defaults;
	fm_directory_view_class->restore_default_zoom_level = fm_clutter_view_restore_default_zoom_level;
	fm_directory_view_class->reveal_selection = fm_clutter_view_reveal_selection;
	fm_directory_view_class->select_all = fm_clutter_view_select_all;
	fm_directory_view_class->set_selection = fm_clutter_view_set_selection;
	fm_directory_view_class->compare_files = fm_clutter_view_compare_files;
	fm_directory_view_class->sort_directories_first_changed = fm_clutter_view_sort_directories_first_changed;
	fm_directory_view_class->start_renaming_file = fm_clutter_view_start_renaming_file;
	fm_directory_view_class->get_zoom_level = fm_clutter_view_get_zoom_level;
	fm_directory_view_class->zoom_to_level = fm_clutter_view_zoom_to_level;
        fm_directory_view_class->emblems_changed = fm_clutter_view_emblems_changed;
	fm_directory_view_class->end_file_changes = fm_clutter_view_end_file_changes;
	fm_directory_view_class->using_manual_layout = fm_clutter_view_using_manual_layout;
	fm_directory_view_class->end_loading = fm_clutter_view_end_loading;
}

static const char *
fm_clutter_view_get_id (NautilusView *view)
{
	return FM_CLUTTER_VIEW_ID;
}


static void
fm_clutter_view_iface_init (NautilusViewIface *iface)
{
	fm_directory_view_init_view_iface (iface);
	
	iface->get_view_id = fm_clutter_view_get_id;
	iface->get_first_visible_file = fm_clutter_view_get_first_visible_file;
	iface->scroll_to_file = fm_clutter_view_scroll_to_file;
	iface->get_title = NULL;
	iface->grab_focus = fm_clutter_view_grab_focus;
}


static void
fm_clutter_view_init (FMClutterView *empty_view)
{
	empty_view->details = g_new0 (FMClutterViewDetails, 1);
}

static NautilusView *
fm_clutter_view_create (NautilusWindowSlotInfo *slot)
{
	FMClutterView *view;

	g_assert (NAUTILUS_IS_WINDOW_SLOT_INFO (slot));

	view = g_object_new (FM_TYPE_CLUTTER_VIEW,
			     "window-slot", slot,
			     NULL);
	g_object_ref (view);

	return NAUTILUS_VIEW (view);
}

static gboolean
fm_clutter_view_supports_uri (const char *uri,
			   GFileType file_type,
			   const char *mime_type)
{
	if (file_type == G_FILE_TYPE_DIRECTORY) {
		return TRUE;
	}
	if (strcmp (mime_type, NAUTILUS_SAVED_SEARCH_MIMETYPE) == 0){
		return TRUE;
	}
	if (g_str_has_prefix (uri, "trash:")) {
		return TRUE;
	}
	if (g_str_has_prefix (uri, EEL_SEARCH_URI)) {
		return TRUE;
	}

	return FALSE;
}

static NautilusViewInfo fm_empty_view = {
	FM_CLUTTER_VIEW_ID,
	"Clutter",
	"Clutter View",
	"_Clutter View",
	"The clutter view encountered an error.",
	"Display this location with the clutter view.",
	fm_clutter_view_create,
	fm_clutter_view_supports_uri
};

void
fm_clutter_view_register (void)
{
	fm_empty_view.id = fm_empty_view.id;
	fm_empty_view.view_combo_label = fm_empty_view.view_combo_label;
	fm_empty_view.view_menu_label_with_mnemonic = fm_empty_view.view_menu_label_with_mnemonic;
	fm_empty_view.error_label = fm_empty_view.error_label;
	fm_empty_view.display_location_label = fm_empty_view.display_location_label;

	nautilus_view_factory_register (&fm_empty_view);
}
