/* vim: set et ts=4 sw=4: */

#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>

#include "clutter-cover-flow.h"

gboolean
on_left_clicked_event (GtkWidget *widget, gpointer user_data)
{
    ClutterCoverFlow *cf = CLUTTER_COVER_FLOW(user_data);
    clutter_cover_flow_left(cf);
    return FALSE;
}

gboolean
on_right_clicked_event (GtkWidget *widget, gpointer user_data)
{
    ClutterCoverFlow *cf = CLUTTER_COVER_FLOW(user_data);
    clutter_cover_flow_right(cf);
    return FALSE;
}

static void
add_file(ClutterCoverFlow *cf, const char *path, int n, gboolean only_append)
{
    int i;
    GFile *file;

    for (i = 0; i < n; i++) {
        file = g_file_new_for_path(path);
        if (only_append) {
            clutter_cover_flow_add_gfile(cf, file);
        } else {
            int idx, file_column;
            GtkListStore *store;
            GtkTreeIter iter;

            idx = g_random_int_range(0,10);
            store = clutter_cover_flow_get_model(cf, &file_column);

            gtk_list_store_insert_with_values (store,
                                               &iter,
                                               idx,
                                               file_column, file,
                                               -1);

            //gtk_list_store_append (priv->model, &iter);
            //gtk_list_store_set (priv->model, &iter,
            //                    priv->file_column, file,
            //                   -1);
        }
        g_object_unref(file);
    }
}

#define NPLACES 3

gboolean
on_add_clicked_event (GtkWidget *widget, gpointer user_data)
{
    static char *places[NPLACES] = {
        "/home",
        "/var",
        "/tmp"
    };
    ClutterCoverFlow *cf = CLUTTER_COVER_FLOW(user_data);

    add_file(cf, places[g_random_int_range(0,NPLACES)], 1, FALSE);

    return FALSE;
}

gboolean
key_press_callback_clutter(ClutterStage *stage, ClutterEvent *event, gpointer callback_data)
{
    int key_code;
    gboolean handled;
    ClutterCoverFlow *cf;

    cf = CLUTTER_COVER_FLOW(callback_data);
    key_code = clutter_event_get_key_code (event);
    g_message("Key Pressed %d",key_code);
    
    if ( 114 == key_code )  /* right arrow */
        clutter_cover_flow_left(cf);
    if ( 113 == key_code )  /* left arrow */
        clutter_cover_flow_right(cf);
    if ( 36 == key_code || 111 == key_code )  /* enter, up arrow */
        clutter_cover_flow_select(cf, TRUE);
    if ( 116 == key_code )  /* down arrow */  
        clutter_cover_flow_clear(cf);
    if ( 41 == key_code )   /* f */
        clutter_cover_flow_film_view(cf);
    if ( 42 == key_code )   /* g */
        clutter_cover_flow_grid_view(cf);
    if ( 33 == key_code )   /* p */
    {
        GFile *f;
        char *uri;

        f = clutter_cover_flow_get_gfile_at_front(cf);
        uri = g_file_get_uri(f);
        g_debug("Front file: %s", uri);

        g_free(uri);
    }
    if ( 24 == key_code || 9 == key_code ) /* q or esc */
        gtk_main_quit();

    handled = TRUE;
    return handled;
}

gboolean
button_press_callback_clutter(ClutterStage *stage, ClutterButtonEvent *event, gpointer callback_data)
{
    ClutterCoverFlow *cf;
    ClutterActor *actorpressed;

    g_debug("Click!");

    cf = CLUTTER_COVER_FLOW(callback_data);
    actorpressed = clutter_stage_get_actor_at_pos(stage,CLUTTER_PICK_ALL,event->x,event->y);
    
    clutter_cover_flow_scroll_to_actor(cf, actorpressed);
    return TRUE;
}
gboolean
scroll_callback_clutter(ClutterStage *stage, ClutterScrollEvent *event, gpointer callback_data)
{
    ClutterCoverFlow *cf;
    
    g_debug("MOUSE SCROLL [%i]",(int)event->direction);
    
    cf = CLUTTER_COVER_FLOW(callback_data);
    
    /*Oposite direction that the scroll*/
    if(event->direction == CLUTTER_SCROLL_DOWN || event->direction == CLUTTER_SCROLL_RIGHT)
        clutter_cover_flow_left(cf);

    /*Oposite direction that the scroll*/
    if(event->direction == CLUTTER_SCROLL_UP || event->direction == CLUTTER_SCROLL_LEFT)
        clutter_cover_flow_right(cf);

    return TRUE;
}


int
main (int argc, char *argv[])
{
    GtkWidget *vbox;
    GtkWidget *bbox;
    GtkWidget *leftbutton;
    GtkWidget *rightbutton;
    GtkWidget *addbutton;
    ClutterActor *stage;
    ClutterColor stage_color = { 0x00, 0x00, 0x00, 0xff };
    GtkWidget *window, *clutter;
    ClutterCoverFlow *cf;

    if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    g_error ("Unable to initialize GtkClutter");

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 500);
    g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    /* Setup stage for gtk+ embedding */
    clutter = gtk_clutter_embed_new ();
    GTK_WIDGET_SET_FLAGS (clutter, GTK_CAN_FOCUS);
    stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter));
    clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

    /* Add the important bits */
    cf = clutter_cover_flow_new (stage);

    /* Buttons and UI */
    gtk_box_pack_start (GTK_BOX(vbox), clutter, TRUE, TRUE, 0);
    bbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
    leftbutton = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
    g_signal_connect (G_OBJECT (leftbutton), "clicked",
                    G_CALLBACK (on_left_clicked_event), cf);
    gtk_box_pack_start (GTK_BOX(bbox), leftbutton, FALSE, TRUE, 0);
    addbutton = gtk_button_new_from_stock (GTK_STOCK_ADD);
    g_signal_connect (G_OBJECT (addbutton), "clicked",
                    G_CALLBACK (on_add_clicked_event), cf);
    gtk_box_pack_start (GTK_BOX(bbox), addbutton, FALSE, TRUE, 0);
    rightbutton = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
    g_signal_connect (G_OBJECT (rightbutton), "clicked",
                    G_CALLBACK (on_right_clicked_event), cf);
    gtk_box_pack_start (GTK_BOX(bbox), rightbutton, FALSE, TRUE, 0);

    /* and signals */
    g_signal_connect (
          stage, 
          "key-press-event", 
          G_CALLBACK (key_press_callback_clutter),
          cf);
    g_signal_connect (
          stage,
          "button-press-event",
          G_CALLBACK (button_press_callback_clutter),
          cf);
    g_signal_connect (
            stage, 
          "scroll-event", 
          G_CALLBACK (scroll_callback_clutter),
          cf);
    /* Show the important bits */
    gtk_widget_show_all (window);
    /* Only show the actors after parent show otherwise it will just be
    * unrealized when the clutter foreign window is set. widget_show
    * will call show on the stage.
    */
    clutter_actor_show_all (CLUTTER_ACTOR (cf));

    /* add first batch of files */
    add_file(cf, "/", 4, TRUE);

    gtk_main();

    return 0;
}
