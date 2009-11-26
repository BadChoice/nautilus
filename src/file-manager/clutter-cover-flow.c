/* vim: set et ts=4 sw=4: */

#include <gtk/gtk.h>
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

    /* default info callback */
    self->priv->get_info_callback = get_info;

    self->priv->info_quark = g_quark_from_string("INFO_CALLBACK");
    self->priv->item_quark = g_quark_from_string("ITEM");

    //self->priv->iter_visible_front = NULL;
    //self->priv->iter_visible_start = NULL;
    //self->priv->iter_visible_end = NULL;

    /* Maps uris to iters in the GSequence. The GSequence cleans up the iters,
     * we must free the keys */
    self->priv->uri_to_item_map = g_hash_table_new_full(
                                    g_str_hash,
                                    g_str_equal,
                                    g_free, /* KeyDestroyFunc, keys are uri strings */
                                    NULL);

    self->priv->iter_added = g_hash_table_new(
                                    g_direct_hash,
                                    g_direct_equal);

    self->priv->view_mode = COVERFLOW_MODE;
}

static gboolean
on_stage_resized(ClutterStage *stage, ClutterButtonEvent *event, gpointer user_data)
{
    ClutterCoverFlow *self = CLUTTER_COVER_FLOW(user_data);
    reset(self->priv);
    return TRUE;
}


void 
clutter_cover_flow_set_model(ClutterCoverFlow *self, GtkTreeModel *store, int file_column)
{
    g_return_if_fail( CLUTTER_IS_COVER_FLOW(self) );

    g_return_if_fail( GTK_IS_TREE_MODEL(store) );
    self->priv->model = store;
    self->priv->file_column = file_column;

    g_signal_connect (store, "row-inserted",
				  G_CALLBACK (model_row_inserted), (gpointer)(self->priv));
    g_signal_connect (store, "row-changed",
				  G_CALLBACK (model_row_changed), (gpointer)(self->priv));
    g_signal_connect (store, "rows-reordered",
				  G_CALLBACK (model_row_reordered), (gpointer)(self->priv));
    g_signal_connect (store, "row-deleted",
				  G_CALLBACK (model_row_deleted), (gpointer)(self->priv));

}

GtkTreeModel *
clutter_cover_flow_get_model(ClutterCoverFlow *self, int *file_column)
{
    g_return_val_if_fail( CLUTTER_IS_COVER_FLOW(self), NULL );
    *file_column = self->priv->file_column;
    return self->priv->model;
}

void 
clutter_cover_flow_set_info_callback(ClutterCoverFlow *self, ClutterCoverFlowGetInfoCallback cb)
{
    g_return_if_fail( CLUTTER_IS_COVER_FLOW(self) );
    self->priv->get_info_callback = cb;
}

ClutterCoverFlow*
clutter_cover_flow_new_with_model (ClutterActor *stage, GtkTreeModel *store, int file_column)
{
    ClutterCoverFlow *self;
    ClutterColor color = { 255, 255, 255, 255 }; /* white */

    g_return_val_if_fail(CLUTTER_IS_STAGE(stage), NULL);
    g_return_val_if_fail(GTK_IS_TREE_MODEL(store), NULL);

    self = g_object_new (CLUTTER_TYPE_COVER_FLOW, NULL);
    clutter_cover_flow_set_model(self, store, 0);

    /* Add ourselves to the stage */
    self->priv->m_stage = stage;
    clutter_container_add_actor ( CLUTTER_CONTAINER (stage), CLUTTER_ACTOR(self) );

    /* Add a container, that will hold all covers, as our child */
    self->priv->m_container = clutter_group_new();
    clutter_container_add_actor ( CLUTTER_CONTAINER (self), self->priv->m_container );

    /* Add some text as our child */
    self->priv->item_name = clutter_text_new_full ("Lucida Grande bold 13", NULL, &color);
    clutter_container_add_actor (CLUTTER_CONTAINER (self->priv->m_container), self->priv->item_name);
    self->priv->item_type = clutter_text_new_full ("Lucida Grande 10", NULL, &color);
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

ClutterCoverFlow*
clutter_cover_flow_new (ClutterActor *stage)
{
    ClutterCoverFlow* cf;

    cf = clutter_cover_flow_new_with_model(
                stage,
                GTK_TREE_MODEL( gtk_list_store_new (1, G_TYPE_FILE) ),
                0);
    cf->priv->model_is_list_store = TRUE;
    return cf;
}

void clutter_cover_flow_add_gfile(ClutterCoverFlow *self, GFile *file)
{
    clutter_cover_flow_add_gfile_with_info_callback(self, file, get_info);
}

void clutter_cover_flow_add_gfile_with_info_callback(ClutterCoverFlow *self, GFile *file, ClutterCoverFlowGetInfoCallback cb)
{
    g_return_if_fail( CLUTTER_IS_COVER_FLOW(self) );
    model_add_file(self->priv, file, cb);
}

static void
clutter_cover_flow_move(ClutterCoverFlow *coverflow, move_t dir)
{
    ClutterCoverFlowPrivate *priv;

    g_return_if_fail( CLUTTER_IS_COVER_FLOW(coverflow) );

    priv = coverflow->priv;
    if ( ! model_is_empty(priv) ) {
        stop(coverflow);
        clear_behaviours(coverflow);
        view_move(priv, dir, TRUE);
        start(coverflow); 
    }

}

void clutter_cover_flow_left(ClutterCoverFlow *coverflow)
{
    clutter_cover_flow_move(coverflow, MOVE_LEFT);
}

void clutter_cover_flow_right(ClutterCoverFlow *coverflow)
{
    clutter_cover_flow_move(coverflow, MOVE_RIGHT);
}

ClutterActor* clutter_cover_flow_get_actor_at_pos(ClutterCoverFlow *coverflow, guint x, guint y)
{
    ClutterCoverFlowPrivate *priv = coverflow->priv;
    return clutter_stage_get_actor_at_pos(CLUTTER_STAGE(priv->m_stage),CLUTTER_PICK_ALL,x,y);
}


void clutter_cover_flow_scroll_to_actor(ClutterCoverFlow *coverflow, ClutterActor *actor)
{
//    GSequenceIter *iter;
    ClutterCoverFlowPrivate *priv;

    g_return_if_fail( CLUTTER_IS_COVER_FLOW(coverflow) );
    g_return_if_fail( CLUTTER_IS_ACTOR(actor) );

    priv = coverflow->priv;

    g_critical("TODO: %s", G_STRFUNC);
#if 0
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
            view_move(coverflow, dir, FALSE);
        start(coverflow);
    }
#endif

}

GFile *
clutter_cover_flow_get_gfile_at_front(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv;

    g_return_val_if_fail( CLUTTER_IS_COVER_FLOW(coverflow), NULL );

    priv = coverflow->priv;
    if (  ! model_is_empty(priv) );
        return model_get_front_file(priv);

    return NULL;
}

void clutter_cover_flow_clear(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv;

    g_return_if_fail( CLUTTER_IS_COVER_FLOW(coverflow) );

    stop(coverflow);
    clear_behaviours(coverflow);

    priv = coverflow->priv;
    if ( ! model_is_empty(priv) )
        knock_down_items(priv, TRUE);

}

void clutter_cover_flow_select(ClutterCoverFlow *coverflow, gboolean should_clear)
{
    ClutterCoverFlowPrivate *priv;

    g_return_if_fail( CLUTTER_IS_COVER_FLOW(coverflow) );

    stop(coverflow);
    clear_behaviours(coverflow);

    priv = coverflow->priv;
    if ( ! model_is_empty(priv) )
        zoom_items(priv, 2.0, should_clear);
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


