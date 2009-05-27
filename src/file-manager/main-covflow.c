#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk.h>

#include "clutter-cover-flow.h"

#define YUCK_DEFAULT_WIDTH 1200
#define YUCK_DEFAULT_HEIGHT 500

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

int
main (int argc, char *argv[])
{
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *leftbutton;
  GtkWidget *rightbutton;
  ClutterActor    *stage;
  ClutterColor     stage_color = { 0x00, 0x00, 0x00, 0xff };
  GtkWidget       *window, *clutter;
  ClutterCoverFlow *cf;

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    g_error ("Unable to initialize GtkClutter");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  /* Setup stage for gtk+ embedding */
  clutter = gtk_clutter_embed_new ();
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter));
  gtk_widget_set_size_request (clutter, YUCK_DEFAULT_WIDTH, YUCK_DEFAULT_HEIGHT);
  clutter_actor_set_size(stage, YUCK_DEFAULT_WIDTH, YUCK_DEFAULT_HEIGHT);
  /* and its background color */
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  /* Add the important bits */
  cf = clutter_cover_flow_new (stage);

  /* Buttons and UI */
  gtk_box_pack_start (GTK_BOX(vbox), clutter, TRUE, TRUE, 0);
  bbox = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX(vbox), bbox, FALSE, TRUE, 0);
  leftbutton = gtk_button_new_from_stock (GTK_STOCK_GO_BACK);
  g_signal_connect (G_OBJECT (leftbutton), "clicked",
                    G_CALLBACK (on_left_clicked_event), (gpointer) cf);
  gtk_box_pack_start (GTK_BOX(bbox), leftbutton, FALSE, TRUE, 0);
  rightbutton = gtk_button_new_from_stock (GTK_STOCK_GO_FORWARD);
  g_signal_connect (G_OBJECT (rightbutton), "clicked",
                    G_CALLBACK (on_right_clicked_event), (gpointer) cf);
  gtk_box_pack_start (GTK_BOX(bbox), rightbutton, FALSE, TRUE, 0);

  /* Show the important bits */
  gtk_widget_show_all (window);
  /* Only show the actors after parent show otherwise it will just be
   * unrealized when the clutter foreign window is set. widget_show
   * will call show on the stage.
   */
   clutter_actor_show_all (CLUTTER_ACTOR (cf));

  /* Ignore the leaks for the test..... */
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/home"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/tmp"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/var"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));

  gtk_main();

  return 0;
}
