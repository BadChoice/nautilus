#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk.h>

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

gboolean
key_press_callback_clutter(ClutterStage *stage, ClutterKeyEvent *event, gpointer callback_data)
{
	int key_code;
	gboolean handled;
	ClutterCoverFlow *cf;

	cf = CLUTTER_COVER_FLOW(callback_data);
	key_code = clutter_key_event_code (event);
	g_message("Key Pressed %d",key_code);
	
	if ( 114 == key_code )
		clutter_cover_flow_left(cf);
	if ( 113 == key_code )
		clutter_cover_flow_right(cf);

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
	actorpressed = clutter_stage_get_actor_at_pos(stage,event->x,event->y);
	
    clutter_cover_flow_scroll_to_actor(cf, actorpressed);
    return TRUE;
}
gboolean
scroll_callback_clutter(ClutterStage *stage, ClutterScrollEvent *event, gpointer callback_data)
{
	ClutterCoverFlow *cf;
	
	g_debug("MOUSE SCROLL [%i]\n",(int)event->direction);
	
	cf = CLUTTER_COVER_FLOW(callback_data);
	
	if(event->direction == CLUTTER_SCROLL_DOWN || event->direction == CLUTTER_SCROLL_RIGHT)
		clutter_cover_flow_left(cf);		/*Oposite direction that the scroll*/
		
	if(event->direction == CLUTTER_SCROLL_UP || event->direction == CLUTTER_SCROLL_LEFT)
		clutter_cover_flow_right(cf);		/*Oposite direction that the scroll*/

	return TRUE;
}


int
main (int argc, char *argv[])
{
    GtkWidget *vbox;
    GtkWidget *bbox;
    GtkWidget *leftbutton;
    GtkWidget *rightbutton;
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

    /* Ignore the leaks for the test..... */
    int i;
    for (i = 0; i < 50; i ++)
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));

    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/home"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/tmp"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/var"));
#if 0
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/home"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/tmp"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/var"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/home"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/tmp"));
    clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/var"));
#endif

    gtk_main();

    return 0;
}
