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
#include "fm-list-model.h"

#include <math.h>
#include <string.h>
#include <libnautilus-private/nautilus-file-utilities.h>
#include <libnautilus-private/nautilus-view.h>
#include <libnautilus-private/nautilus-view-factory.h>
#include <libnautilus-private/nautilus-column-utilities.h>
#include <libnautilus-private/nautilus-cell-renderer-pixbuf-emblem.h>
#include <libnautilus-private/nautilus-cell-renderer-text-ellipsized.h>
#include <eel/eel-glib-extensions.h>
#include <eel/eel-gtk-macros.h>
#include <eel/eel-vfs-extensions.h>

#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include "clutter-cover-flow.h"

struct FMClutterViewDetails {
	int number_of_files;
	ClutterCoverFlow *cf;
	GtkWidget *clutter;
	GtkWidget *pane;
	GtkWidget *scrolled_window;

	FMListModel *model;
	GtkTreeView *tree;

	GtkTreeViewColumn   *file_name_column;
	int file_name_column_num;

	GtkCellRendererPixbuf *pixbuf_cell;
	GtkCellRendererText   *file_name_cell;
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

#define USE_THUMBS 		1
#define MIN_COVERFLOW_WIDTH 	500
#define MIN_COVERFLOW_HEIGHT	250
#define MIN_LIST_HEIGHT		100

static gboolean
key_press_callback_clutter (GtkWidget *widget, GdkEventKey *event, gpointer callback_data)
{
    gboolean handled = FALSE;
    FMClutterView *view;
    ClutterCoverFlow *cf;
    GFile *file;

    view = FM_CLUTTER_VIEW(callback_data);
    cf = view->details->cf;

    g_message("Key Pressed %d",event->keyval);

    switch (event->keyval) {
    case GDK_Left:
        clutter_cover_flow_right(cf);
        handled = TRUE;
        break;
    case GDK_Right:
        clutter_cover_flow_left(cf);
        handled = TRUE;
        break;
    case GDK_Return:
        file = clutter_cover_flow_get_gfile_at_front(cf);
        if (file) {
            NautilusFile *nfile;

            clutter_cover_flow_select(cf);
            //amtest
            printf("hummm\n");
            nfile = nautilus_file_get(file);
            printf("hummm\n");

            fm_directory_view_activate_file (
                                             FM_DIRECTORY_VIEW(view),
                                             nfile,
                                             NAUTILUS_WINDOW_OPEN_IN_NAVIGATION,
                                             0);
        }
        handled = TRUE;
        break;
    }
    return handled;
}

static gboolean
scroll_callback_clutter(GtkWidget *widget, GdkEventScroll *event, gpointer callback_data)
{
    gboolean handled = FALSE;
    FMClutterView *view;
    ClutterCoverFlow *cf;
    
    view = FM_CLUTTER_VIEW(callback_data);
    cf = view->details->cf;


    g_message("Scroll Eventi %d\n",event->direction);
    switch(event->direction)
    {
        case GDK_SCROLL_UP:
        {
            clutter_cover_flow_right(cf);
            handled = TRUE;
            break;
        }

        case GDK_SCROLL_DOWN:
        {
            clutter_cover_flow_left(cf);
            handled = TRUE;
            break;	
        }
	case GDK_SCROLL_LEFT: break;
	case GDK_SCROLL_RIGHT: break;
    }

    return handled;
}

/*
static void 
get_info(GFile *file, char **name, char **description, GdkPixbuf **pb, guint pbsize)
{
	NautilusFile *nfile;
#if USE_THUMBS
	int thumb_flags = NAUTILUS_FILE_ICON_FLAGS_USE_THUMBNAILS | NAUTILUS_FILE_ICON_FLAGS_FORCE_THUMBNAIL_SIZE;
#else
	int thumb_flags = NAUTILUS_FILE_ICON_FLAGS_NONE;
#endif
	nfile = nautilus_file_get_existing(file);

	*pb = nautilus_file_get_icon_pixbuf (
		nfile, 
		pbsize, 
		TRUE, 
		thumb_flags);

	*name = nautilus_file_get_display_name(nfile);
*/
	/* TODO: Perhaps I should call nautils_file_get_description, but
         * it gives up if the standard::description key has not been loaded */
/*	*description = nautilus_file_get_string_attribute(nfile, "type");

	nautilus_file_unref(nfile);
}
*/

/* 
** nautilus_file_get_icon_pixbuf is for icon where the largest value is 
** nautilus-icon-info.h:#define NAUTILUS_ICON_SIZE_LARGEST     192
** this results having images scaled from small 192 sized thumb
** real thumbnail that nautilus use, use fcts defined in
** ../../libnautilus-private/nautilus-thumbnails.c
**
** so let's use gnome_desktop_thumbnail's new experimental API
**
** i don't think we need async stuff it makes no sense to load a THUMBNAIL_SIZE_NORMAL
** and then reload and replace it with a SIZE_LARGE wich is just twice as large
** than the previous one (just cpu wasting time IMO).
*/

static void 
get_info(GFile *file, char **name, char **description, GdkPixbuf **pb, guint pbsize)
{
    NautilusFile *nfile;
    GnomeDesktopThumbnailFactory *fac;
    char *uri;
    time_t mtime;
    char *thumb_loc;

    nfile = nautilus_file_get_existing(file);
    fac = gnome_desktop_thumbnail_factory_new(GNOME_DESKTOP_THUMBNAIL_SIZE_LARGE);
    uri = nautilus_file_get_uri (nfile);
    mtime = nautilus_file_get_mtime (nfile);

    *name = nautilus_file_get_display_name(nfile);
    *description = nautilus_file_get_string_attribute(nfile, "type");

    thumb_loc = gnome_desktop_thumbnail_factory_lookup (fac, uri, mtime);
    if (!thumb_loc) 
    {
        if ((*pb = gnome_desktop_thumbnail_factory_generate_thumbnail(fac, uri, *description)))
            gnome_desktop_thumbnail_factory_save_thumbnail(fac, *pb, uri, mtime);
        else
            *pb =  nautilus_file_get_icon_pixbuf (nfile, pbsize, TRUE, 0);      
            //*pb =  nautilus_file_get_icon_pixbuf (nfile, nautilus_get_icon_size_for_zoom_level (NAUTILUS_ZOOM_LEVEL_STANDARD), TRUE, 0);
    }
    else
    {
        if (!(*pb = gdk_pixbuf_new_from_file (thumb_loc, NULL)))
            g_warning ("could not load %s (%s)\n", *name, *description);
    }

    nautilus_file_unref(nfile);
    g_object_unref (fac);
    g_free (thumb_loc);
    g_free (uri);

}

static void
fm_clutter_view_add_file (FMDirectoryView *view, NautilusFile *file, NautilusDirectory *directory)
{
	GFile * gfile;
	FMListModel *model;

	gfile = nautilus_file_get_location(file);

	clutter_cover_flow_add_gfile_with_info_callback(
		FM_CLUTTER_VIEW (view)->details->cf,
		gfile,
		get_info);

	model = FM_CLUTTER_VIEW (view)->details->model;
	fm_list_model_add_file (model, file, directory);


	FM_CLUTTER_VIEW (view)->details->number_of_files++;
}


static void
fm_clutter_view_begin_loading (FMDirectoryView *view)
{
}

static void
fm_clutter_view_clear (FMDirectoryView *view)
{
    //amtest
    printf("$$ fm_clutter_view_clear\n");
	FMListModel *model;
	ClutterCoverFlow *cf;

	model = FM_CLUTTER_VIEW (view)->details->model;
	cf = FM_CLUTTER_VIEW (view)->details->cf;

	fm_list_model_clear (model);

	clutter_cover_flow_clear(cf);
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
fm_clutter_view_unmerge_menus (FMDirectoryView *view)
{
	EEL_CALL_PARENT (FM_DIRECTORY_VIEW_CLASS, unmerge_menus, (view));
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
	gtk_widget_grab_focus (FM_CLUTTER_VIEW (view)->details->clutter);
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
	fm_directory_view_class->unmerge_menus = fm_clutter_view_unmerge_menus;
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

#if 0
static void
filename_cell_data_func (GtkTreeViewColumn *column,
			 GtkCellRenderer   *renderer,
			 GtkTreeModel      *model,
			 GtkTreeIter       *iter,
			 FMClutterView     *view)
{
	char *text;

//	GtkTreePath *path;
//	PangoUnderline underline;

	gtk_tree_model_get (model, iter,
			    view->details->file_name_column_num, &text,
			    -1);
#if 0
	if (click_policy_auto_value == NAUTILUS_CLICK_POLICY_SINGLE) {
		path = gtk_tree_model_get_path (model, iter);

		if (view->details->hover_path == NULL ||
		    gtk_tree_path_compare (path, view->details->hover_path)) {
			underline = PANGO_UNDERLINE_NONE;
		} else {
			underline = PANGO_UNDERLINE_SINGLE;
		}

		gtk_tree_path_free (path);
	} else {
		underline = PANGO_UNDERLINE_NONE;
	}
#endif
	g_object_set (G_OBJECT (renderer),
		      "text", text,
		      "underline", PANGO_UNDERLINE_SINGLE,
		      NULL);
	g_free (text);
}
#endif

static void
create_and_set_up_tree_view (FMClutterView *view)
{
	GtkCellRenderer *cell;
	GtkTreeViewColumn *column;
	GList *nautilus_columns;
	GList *l;
	
	gtk_tree_view_set_enable_search (view->details->tree, TRUE);
	gtk_tree_view_set_model (view->details->tree, GTK_TREE_MODEL (view->details->model));

	nautilus_columns = nautilus_get_all_columns ();

	for (l = nautilus_columns; l != NULL; l = l->next) {
		NautilusColumn *nautilus_column;
		int column_num;		
		char *name;
		char *label;
		float xalign;

		nautilus_column = NAUTILUS_COLUMN (l->data);

		g_object_get (nautilus_column, 
			      "name", &name, 
			      "label", &label,
			      "xalign", &xalign, NULL);

		g_message("NAME: %s", name);

		/* Created the name column specially, because it
		 * has the icon in it.*/
		if (!strcmp (name, "name")) {
			column_num = fm_list_model_add_column (view->details->model,
					       nautilus_column);

			/* Create the file name column */
			cell = nautilus_cell_renderer_pixbuf_emblem_new ();
			view->details->pixbuf_cell = (GtkCellRendererPixbuf *)cell;
			
			view->details->file_name_column = gtk_tree_view_column_new ();
			g_object_ref (view->details->file_name_column);
			view->details->file_name_column_num = column_num;
			
//			g_hash_table_insert (view->details->columns,
//					     g_strdup ("name"), 
//					     view->details->file_name_column);

			gtk_tree_view_set_search_column (view->details->tree, column_num);

			gtk_tree_view_column_set_sort_column_id (view->details->file_name_column, column_num);
			gtk_tree_view_column_set_title (view->details->file_name_column, "Name");
			gtk_tree_view_column_set_resizable (view->details->file_name_column, TRUE);
			
			gtk_tree_view_column_pack_start (view->details->file_name_column, cell, FALSE);
			gtk_tree_view_column_set_attributes (view->details->file_name_column,
							     cell,
							     "pixbuf", FM_LIST_MODEL_SMALLEST_ICON_COLUMN,
							     "pixbuf_emblem", FM_LIST_MODEL_SMALLEST_EMBLEM_COLUMN,
							     NULL);
			
			cell = nautilus_cell_renderer_text_ellipsized_new ();
			view->details->file_name_cell = (GtkCellRendererText *)cell;
//			g_signal_connect (cell, "edited", G_CALLBACK (cell_renderer_edited), view);
//			g_signal_connect (cell, "editing-canceled", G_CALLBACK (cell_renderer_editing_canceled), view);
			
			gtk_tree_view_column_pack_start (view->details->file_name_column, cell, TRUE);
//			gtk_tree_view_column_set_cell_data_func (view->details->file_name_column, cell,
//								 (GtkTreeCellDataFunc) filename_cell_data_func,
//								 view, NULL);
			gtk_tree_view_column_set_attributes (view->details->file_name_column,
							     cell,
							     "text", view->details->file_name_column_num,
//							     "underline", PANGO_UNDERLINE_SINGLE,
							     NULL);

			gtk_tree_view_append_column (view->details->tree, view->details->file_name_column);
		}
		else if ( strcmp (name,"type") == 0) {
			column_num = fm_list_model_add_column (view->details->model,
					       nautilus_column);

			cell = gtk_cell_renderer_text_new ();
			g_object_set (cell, "xalign", xalign, NULL);
			//view->details->cells = g_list_append (view->details->cells,
			//				      cell);
			column = gtk_tree_view_column_new_with_attributes (label,
									   cell,
									   "text", column_num,
									   NULL);
			g_object_ref (column);
			gtk_tree_view_column_set_sort_column_id (column, column_num);
			//g_hash_table_insert (view->details->columns, 
			//		     g_strdup (name), 
			//		     column);
			
			gtk_tree_view_column_set_resizable (column, TRUE);
			gtk_tree_view_column_set_visible (column, TRUE);

			gtk_tree_view_append_column (view->details->tree, column);
		}
		g_free (name);
		g_free (label);
	}

	nautilus_column_list_free (nautilus_columns);
	

#if 0
	/* Apply the default column order and visible columns, to get it
	 * right most of the time. The metadata will be checked when a 
	 * folder is loaded */
	apply_columns_settings (view,
				default_column_order_auto_value,
				default_visible_columns_auto_value);

	gtk_widget_show (GTK_WIDGET (view->details->tree_view));
	gtk_container_add (GTK_CONTAINER (view), GTK_WIDGET (view->details->tree_view));


        atk_obj = gtk_widget_get_accessible (GTK_WIDGET (view->details->tree_view));
        atk_object_set_name (atk_obj, _("List View"));
#endif
}

static void
create_and_setup_list_model(FMClutterView *view)
{
	view->details->model = g_object_new (
					FM_TYPE_LIST_MODEL,
					"list-only", TRUE,
					NULL);
	fm_list_model_set_should_sort_directories_first(view->details->model, TRUE);
}

static void
fm_clutter_view_init (FMClutterView *empty_view)
{
	ClutterActor    *stage;
	ClutterColor     stage_color = { 0x00, 0x00, 0x00, 0xff }; /* black */

	if (gtk_clutter_init (NULL, NULL) != CLUTTER_INIT_SUCCESS) {
		g_error ("Unable to initialize GtkClutter");
		return;
	}
	
	empty_view->details = g_new0 (FMClutterViewDetails, 1);

	empty_view->details->clutter = gtk_clutter_embed_new ();
	empty_view->details->pane = gtk_vpaned_new ();
	empty_view->details->scrolled_window = gtk_scrolled_window_new(NULL, NULL);

	create_and_setup_list_model(empty_view);

	empty_view->details->tree = GTK_TREE_VIEW (gtk_tree_view_new ());
    gtk_tree_view_set_rules_hint  (GTK_TREE_VIEW(empty_view->details->tree), TRUE);          //Diferent color for each row

	/* Add the clutter widget to the top pane */
	gtk_widget_set_size_request(
		GTK_WIDGET(empty_view->details->clutter),
		MIN_COVERFLOW_WIDTH, MIN_COVERFLOW_HEIGHT);
	//FIXME: Is this the correct way to ensure key presses are delivered - it does not work
	GTK_WIDGET_SET_FLAGS (empty_view->details->clutter, GTK_CAN_FOCUS); 
	gtk_paned_pack1(
		GTK_PANED(empty_view->details->pane),
		GTK_WIDGET(empty_view->details->clutter),
		TRUE,
		FALSE);

	/* Add the sw to the bottom pane */
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(empty_view->details->scrolled_window),
		GTK_POLICY_NEVER,
		GTK_POLICY_ALWAYS);
	gtk_container_add(
		GTK_CONTAINER(empty_view->details->scrolled_window),
		GTK_WIDGET(empty_view->details->tree));

	/* Add the tree to the sw */
	gtk_widget_set_size_request(
		GTK_WIDGET(empty_view->details->tree),
		MIN_COVERFLOW_WIDTH, MIN_LIST_HEIGHT);
	gtk_paned_pack2(
		GTK_PANED(empty_view->details->pane),
		GTK_WIDGET(empty_view->details->scrolled_window),
		FALSE,
		FALSE);

	/* Setup the treeview */
	create_and_set_up_tree_view(empty_view);

	/* Add the pane to the main widget */
	gtk_scrolled_window_add_with_viewport (
		GTK_SCROLLED_WINDOW(empty_view),
		GTK_WIDGET (empty_view->details->pane));
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(empty_view),
		GTK_POLICY_NEVER,
		GTK_POLICY_NEVER);

  	/* create the cover flow widget - it adds itself to the stage */
	stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (empty_view->details->clutter));
	clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);
  	empty_view->details->cf = clutter_cover_flow_new ( CLUTTER_ACTOR (stage) );
  
	/* Connect signals */
    g_signal_connect_object (empty_view->details->clutter, "key_press_event",
                             G_CALLBACK (key_press_callback_clutter), empty_view, 0);

    g_signal_connect_object (empty_view->details->clutter, "scroll_event",
                             G_CALLBACK (scroll_callback_clutter), empty_view, 0);


	gtk_widget_show_all (empty_view->details->pane);
	/* Only show the actors after parent show otherwise it will just be
	* unrealized when the clutter foreign window is set. widget_show
	* will call show on the stage.
	*/
	clutter_actor_show_all (CLUTTER_ACTOR (empty_view->details->cf));

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
