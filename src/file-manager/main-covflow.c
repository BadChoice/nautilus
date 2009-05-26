#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk.h>

#include "clutter-cover-flow.h"

#define YUCK_DEFAULT_WIDTH 1200
#define YUCK_DEFAULT_HEIGHT 500

int
main (int argc, char *argv[])
{
  ClutterActor    *stage;
  ClutterColor     stage_color = { 0x00, 0x00, 0x00, 0xff };
  GtkWidget       *window, *clutter;
  ClutterCoverFlow *cf;

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    g_error ("Unable to initialize GtkClutter");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  clutter = gtk_clutter_embed_new ();
  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter));

  gtk_container_add (GTK_CONTAINER (window), clutter);
  gtk_widget_set_size_request (clutter, YUCK_DEFAULT_WIDTH, YUCK_DEFAULT_HEIGHT);
  clutter_actor_set_size(stage, YUCK_DEFAULT_WIDTH, YUCK_DEFAULT_HEIGHT);

  /* and its background color */
  clutter_stage_set_color (CLUTTER_STAGE (stage), &stage_color);

  cf = clutter_cover_flow_new (stage);

  gtk_widget_show_all (window);

  /* Only show the actors after parent show otherwise it will just be
   * unrealized when the clutter foreign window is set. widget_show
   * will call show on the stage.
   */
   clutter_actor_show_all (CLUTTER_ACTOR (cf));

  /* Ignore the leaks for the test..... */
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));
  clutter_cover_flow_add_gfile(cf, g_file_new_for_path("/"));

  gtk_main();

  return 0;
}
