/* vim: set et ts=4 sw=4: */

#include <glib.h>
#include "clutter-cover-flow.h"
#include "clutter-cover-flow-internal.h"
#include "clutter-black-texture.h"

G_DEFINE_TYPE (ClutterCoverFlow, clutter_cover_flow, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLUTTER_TYPE_COVER_FLOW, ClutterCoverFlowPrivate))

static void
clutter_cover_flow_dispose (GObject *object)
{
	ClutterCoverFlow *self = CLUTTER_COVER_FLOW(object); 

	if (self->priv)
	{
	  //if (self->priv->trans != NULL)
	  //g_object_unref(self->priv->trans);
	  //self->priv->trans = NULL;
	}

	G_OBJECT_CLASS (clutter_cover_flow_parent_class)->dispose (object);
}

static void
clutter_cover_flow_finalize (GObject *object)
{
  ClutterCoverFlow *self = CLUTTER_COVER_FLOW(object); 

  if (self->priv)
  {
    g_free(self->priv);
    self->priv = NULL;
  }

  G_OBJECT_CLASS (clutter_cover_flow_parent_class)->finalize (object);
}

static void
clutter_cover_flow_class_init (ClutterCoverFlowClass *klass)
{
  GObjectClass *object_class;
  //ClutterActorClass *actor_class;

  object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = clutter_cover_flow_dispose;
  object_class->finalize = clutter_cover_flow_finalize;
}

static void
clutter_cover_flow_init (ClutterCoverFlow *self)
{
    self->priv  = g_new0 (ClutterCoverFlowPrivate, 1);

    self->priv->m_timeline = clutter_timeline_new(FRAMES * FPS);
    self->priv->m_alpha = clutter_alpha_new_full(self->priv->m_timeline,CLUTTER_EASE_OUT_EXPO);
    self->priv->_items = g_sequence_new((GDestroyNotify)item_free_invisible);

    /* Maps uris to iters in the GSequence. The GSequence cleans up the iters,
     * we must free the keys */
    self->priv->uri_to_item_map = g_hash_table_new_full(
                                    g_str_hash,
                                    g_str_equal,
                                    g_free, /* KeyDestroyFunc, keys are uri strings */
                                    NULL);

    self->priv->view_mode = COVERFLOW_MODE;
}

static gboolean
on_stage_resized(ClutterStage *stage, ClutterButtonEvent *event, gpointer user_data)
{
    ClutterCoverFlow *self = CLUTTER_COVER_FLOW(user_data); 
    guint h = clutter_actor_get_height(CLUTTER_ACTOR(stage));
    guint w = clutter_actor_get_width(CLUTTER_ACTOR(stage));
    float relation = (float)500/(float)h;

    clutter_actor_set_position( 
                        self->priv->m_container, 
                        w/2,
                        h/2);

    clutter_actor_set_position (
                    self->priv->item_name,
                   0 - clutter_actor_get_width(self->priv->item_name)/2, 
                    (MAX_ITEM_HEIGHT/2) + TEXT_PAD_BELOW_ITEM);
    clutter_actor_set_position (
                    self->priv->item_type, 
                    0 - clutter_actor_get_width(self->priv->item_type)/2, 
                    (MAX_ITEM_HEIGHT/2) + TEXT_PAD_BELOW_ITEM + clutter_actor_get_height(self->priv->item_name) + 5);

    clutter_actor_set_scale(
                    self->priv->m_container,
                    1/relation ,
                    1/relation);

    g_debug("Stage Resized: %dx%d", w, h);
    return TRUE;
}


ClutterCoverFlow*
clutter_cover_flow_new (ClutterActor *stage)
{
    ClutterCoverFlow *self;
    ClutterColor color = { 255, 255, 255, 255 }; /* white */

    g_return_val_if_fail(CLUTTER_IS_STAGE(stage), NULL);

    self = g_object_new (CLUTTER_TYPE_COVER_FLOW, NULL);
    self->priv->m_stage = stage;

    /* Add ourselves to the stage */
    clutter_container_add_actor ( CLUTTER_CONTAINER (stage), CLUTTER_ACTOR(self) );

    /* Add a container, that will hold all covers, as our child */
    self->priv->m_container = clutter_group_new();
    clutter_container_add_actor ( CLUTTER_CONTAINER (self), self->priv->m_container );

    /* Add some text as our child */
    //amtest
    //self->priv->item_name = clutter_text_new_full ("Lucida Grande bold 13", NULL, &color);
    self->priv->item_name = clutter_text_new_full ("Lucida Grande bold 13", "toto", &color);
    clutter_container_add_actor (CLUTTER_CONTAINER (self->priv->m_container), self->priv->item_name);

    //self->priv->item_type = clutter_text_new_full ("Lucida Grande 10", NULL, &color);
    self->priv->item_type = clutter_text_new_full ("Lucida Grande 10", "toto2", &color);
    clutter_container_add_actor (CLUTTER_CONTAINER (self->priv->m_container), self->priv->item_type);

    /* Track stage resizes. */
    g_signal_connect (
            stage,
            "notify::allocation",
            G_CALLBACK (on_stage_resized),
            self);

    /* Fake resize event to set item initial position */
    on_stage_resized(CLUTTER_STAGE(stage), NULL, self);

    return self;
}

void clutter_cover_flow_add_gfile(ClutterCoverFlow *self, GFile *file)
{
    add_file_internal(self, file, get_info);
}

void clutter_cover_flow_add_gfile_with_info_callback(ClutterCoverFlow *self, GFile *file, ClutterCoverFlowGetInfoCallback cb)
{
    add_file_internal(self, file, cb);
}

void clutter_cover_flow_left(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv = coverflow->priv;

    g_debug("MOVE: Left requested");

    if ( g_sequence_get_length(priv->_items) ) {
        stop(coverflow);
        clear_behaviours(coverflow);
        move_iters(coverflow, MOVE_LEFT, TRUE);
        //start(coverflow);
    }
}

void clutter_cover_flow_right(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv = coverflow->priv;

    g_debug("MOVE: Right requested");

    if ( g_sequence_get_length(priv->_items) ) {
        stop(coverflow);
        clear_behaviours(coverflow);
        move_iters(coverflow, MOVE_RIGHT, TRUE);
        //start(coverflow); 
    }
}

void clutter_cover_flow_scroll_to_actor(ClutterCoverFlow *coverflow, ClutterActor *actor)
{
    GSequenceIter *iter;
    ClutterCoverFlowPrivate *priv = coverflow->priv;

    g_return_if_fail( CLUTTER_IS_COVER_FLOW(coverflow) );
    g_return_if_fail( CLUTTER_IS_ACTOR(actor) );

    iter = get_actor_iter(priv, actor);
    if (iter) {
        move_t dir;
        int i, me, front;
        GSequenceIter *look;

        /* did we click on the front iter ? */
        if (iter == priv->iter_visible_front)
            return;

        /* search all iters and find our index, and the index of the front */
        for (i = 0, me = 0, front = 0, look = priv->iter_visible_start;
             TRUE;
             i++, look = g_sequence_iter_next(look))
        {
            if (look == iter)
                me = i;

            if (look == priv->iter_visible_front)
                front = i;

            if (look == priv->iter_visible_end)
                break;
        }

        dir = ( me > front ? MOVE_LEFT : MOVE_RIGHT);

        stop(coverflow);
        clear_behaviours(coverflow);
        for (i = ABS(me-front); i > 0; i--)
            move_iters(coverflow, dir, FALSE);
        start(coverflow);
    }

}

GFile *
clutter_cover_flow_get_gfile_at_front(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv;

    g_return_val_if_fail( CLUTTER_IS_COVER_FLOW(coverflow), NULL );

    priv = coverflow->priv;
    if (priv->n_visible_items > 0 && priv->iter_visible_front);
        return g_sequence_get(priv->iter_visible_front);

    return NULL;
}

void clutter_cover_flow_clear(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv;

    g_return_if_fail( CLUTTER_IS_COVER_FLOW(coverflow) );

    priv = coverflow->priv;
    if ( g_sequence_get_length(priv->_items) )
        knock_down_items(priv);

    //g_sequence_free(priv->_items);
    //CoverFlowItem *item = g_sequence_get(priv->iter_visible_front);
    //g_object_unref(item->container);
}

void clutter_cover_flow_select(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv;

    g_return_if_fail( CLUTTER_IS_COVER_FLOW(coverflow) );

    priv = coverflow->priv;
    if ( g_sequence_get_length(priv->_items) )
        zoom_items(priv, 2.0);
}

void clutter_cover_flow_default_view(ClutterCoverFlow *coverflow)
{
    ;
}

void clutter_cover_flow_grid_view(ClutterCoverFlow *coverflow)
{
    ;
}

void clutter_cover_flow_film_view(ClutterCoverFlow *coverflow)
{
    ;
}


