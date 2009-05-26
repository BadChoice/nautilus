#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk.h>

#include "clutter-cover-flow.h"

int
main (int argc, char *argv[])
{
  ClutterActor    *stage;
  ClutterColor     stage_color = { 0x00, 0x00, 0x00, 0xff };
  GtkWidget       *window, *clutter;
  GFile           *testf;
  ClutterCoverFlow *cf;

  if (gtk_clutter_init (&argc, &argv) != CLUTTER_INIT_SUCCESS)
    g_error ("Unable to initialize GtkClutter");


//  pixbuf = gdk_pixbuf_new_from_file ("redhand.png", NULL);

//  if (!pixbuf)
//    g_error("pixbuf load failed");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

//  vbox = gtk_vbox_new (FALSE, 6);
//  gtk_container_add (GTK_CONTAINER (window), vbox);

  clutter = gtk_clutter_embed_new ();
  gtk_widget_set_size_request (clutter, 1200, 500);

  gtk_container_add (GTK_CONTAINER (window), clutter);

  stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (clutter));

//  label = gtk_label_new ("This is a label");
//  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

//  button = gtk_button_new_with_label ("This is a button...clicky");
//  g_signal_connect (button, "clicked",
//                    G_CALLBACK (clickity), NULL);
//  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

//  button = gtk_button_new_with_label ("Fullscreen");
//  gtk_button_set_image (GTK_BUTTON (button),
//                        gtk_image_new_from_stock (GTK_STOCK_FULLSCREEN,
//                                                  GTK_ICON_SIZE_BUTTON));
//  g_signal_connect (button, "clicked",
//                    G_CALLBACK (on_fullscreen),
//                    window);
//  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

//  button = gtk_button_new_from_stock (GTK_STOCK_QUIT);
//  g_signal_connect_swapped (button, "clicked",
//                            G_CALLBACK (gtk_widget_destroy),
//                            window);
//  gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  
  /* and its background color */

  clutter_stage_set_color (CLUTTER_STAGE (stage),
		           &stage_color);

  cf = clutter_cover_flow_new (stage);

//  oh = g_new(SuperOH, 1);

  /* create a new group to hold multiple actors in a group */
//  oh->group = CLUTTER_GROUP (clutter_group_new ());
  
//  for (i = 0; i < NHANDS; i++)
//    {
//      gint x, y, w, h;

      /* Create a texture from pixbuf, then clone in to same resources */
//      if (i == 0)
//       oh->hand[i] = gtk_clutter_texture_new_from_pixbuf (pixbuf);
//     else
//       oh->hand[i] = clutter_clone_new (oh->hand[0]);
      /* Place around a circle */
//      w = clutter_actor_get_width (oh->hand[0]);
//      h = clutter_actor_get_height (oh->hand[0]);

//      x = WINWIDTH/2  + RADIUS * cos (i * M_PI / (NHANDS/2)) - w/2;
//      y = WINHEIGHT/2 + RADIUS * sin (i * M_PI / (NHANDS/2)) - h/2;

//      clutter_actor_set_position (oh->hand[i], x, y);

      /* Add to our group group */
//      clutter_group_add (oh->group, oh->hand[i]);
//    }

  /* Add the group to the stage */
//  clutter_container_add_actor (CLUTTER_CONTAINER (stage),
//                               CLUTTER_ACTOR (oh->group));

//  g_signal_connect (stage, "button-press-event",
//		    G_CALLBACK (input_cb), 
//		    oh);
//  g_signal_connect (stage, "key-release-event",
//		    G_CALLBACK (input_cb),
//		    oh);

  gtk_widget_show_all (window);

  /* Only show the actors after parent show otherwise it will just be
   * unrealized when the clutter foreign window is set. widget_show
   * will call show on the stage.
   */
   clutter_actor_show_all (CLUTTER_ACTOR (cf));

  /* Create a timeline to manage animation */
//  timeline = clutter_timeline_new (360, 60); /* num frames, fps */
//  g_object_set(timeline, "loop", TRUE, NULL);   /* have it loop */

  /* fire a callback for frame change */
//  g_signal_connect(timeline, "new-frame",  G_CALLBACK (frame_cb), oh);

  /* and start it */
//  clutter_timeline_start (timeline);

//#endif

  testf = g_file_new_for_path("/");
  clutter_cover_flow_add_gfile(cf, testf);

  gtk_main();

  return 0;
}
