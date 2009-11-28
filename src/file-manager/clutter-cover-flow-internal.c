#include "clutter-cover-flow-internal.h"

static gboolean view_is_path_in_visible_range(ClutterCoverFlowPrivate *priv, int pos);

void
item_clear_behavior (CoverFlowItem *item, gpointer user_data)
{
    g_return_if_fail(item != NULL);

    if (    item->rotateBehaviour && 
            CLUTTER_IS_BEHAVIOUR(item->rotateBehaviour) && 
            clutter_behaviour_is_applied(item->rotateBehaviour, item->container) )
        {
            clutter_behaviour_remove(item->rotateBehaviour,item->container);
        }
}

void
item_free_visible(CoverFlowItem *item)
{
    /* Remove all item resources except the GFile, in case the
     * item is paged back in future */
    g_free(item->display_name);
        item->display_name = NULL;
    g_free(item->display_type);
        item->display_type = NULL;

    /* Remove and free any pending rotation behaviors */
    if (item->rotateBehaviour) {
        item_clear_behavior(item, NULL);
        item->rotateBehaviour = NULL;
    }

    if (item->container) {
        ClutterActor *parent;

        /* Remove the children actors first, they are unrefed automatically,
         * so should be collected */
        clutter_container_remove(
                    CLUTTER_CONTAINER(item->container),
                    item->texture,
                    item->reflection,
                    NULL);

        /* Remove ourselves from our parent */
        parent = clutter_actor_get_parent(item->container);
        clutter_container_remove_actor(
                    CLUTTER_CONTAINER(parent),
                    item->container);

        item->container = NULL;
    }
}

void 
item_free_invisible(CoverFlowItem *item)
{
    item_free_visible(item);
    g_object_unref(item->file);
    g_free(item);
}

#if 0
static void
items_free_all(ClutterTimeline *timeline, ClutterCoverFlowPrivate *priv)
{
    g_critical("TODO: %s", G_STRFUNC);
#if 0
    g_sequence_remove_range(
            g_sequence_get_begin_iter(priv->_items),
            g_sequence_get_end_iter(priv->_items));

    priv->n_visible_items = 0;

    priv->iter_visible_front = g_sequence_get_begin_iter(priv->_items);
    priv->iter_visible_start = g_sequence_get_begin_iter(priv->_items);
    priv->iter_visible_end = g_sequence_get_begin_iter(priv->_items);
#endif
    /* reset the scale and opacity of the outer container */
    reset(priv);
}
#endif

#if 0
static void
items_show_all(ClutterTimeline *timeline, ClutterCoverFlowPrivate *priv)
{
    /* reset the scale and opacity of the outer container */
    reset(priv);
}
#endif

#if 0
static CoverFlowItem *
model_get_item(ClutterCoverFlowPrivate *priv, int idx)
{
    GtkTreePath *path;
    GtkTreeIter *iter;
    CoverFlowItem *item;
    gboolean ok;
    
    path = gtk_tree_path_new_from_indices(idx, -1);
    ok = gtk_tree_model_get_iter(   GTK_TREE_MODEL(priv->model),
                                    iter,
                                    path);

    g_return_val_if_fail(ok == TRUE, NULL);
    g_return_val_if_fail(gtk_list_store_iter_is_valid (priv->model, iter), NULL);

    item = g_hash_table_lookup(priv->iter_added, iter);

    g_return_val_if_fail(item != NULL, NULL);

    return item;
}
#endif

static int
view_n_visible_items(ClutterCoverFlowPrivate *priv)
{
    return priv->idx_visible_end - priv->idx_visible_start;
}

static void
model_do_insert(ClutterCoverFlowPrivate *priv, GtkTreeIter *iter, GtkTreePath *path, GFile *file)
{
    gpointer cb;
    CoverFlowItem *item;
    gint *idxs;

    /* set a default info callback if one is not given */
    cb = g_object_get_qdata( G_OBJECT(file), priv->info_quark );
    if (cb == NULL)
        g_object_set_qdata( G_OBJECT(file), priv->info_quark, (gpointer)priv->get_info_callback );

    /* Create the new item */
    item = g_new0 (CoverFlowItem, 1);
    item->get_info_callback = cb;
    /* FIXME: Should we ref the file here?? */
    item->file = file;
    item->iter = iter;
    item->get_info_callback = g_object_get_qdata( G_OBJECT(file), priv->info_quark );

    /* for looking up iters by uri */
    g_hash_table_insert(
        priv->uri_to_item_map,
        g_file_get_uri(file),   /* freed by hashtable KeyDestroyFunc */
        iter);

    g_hash_table_insert(
        priv->iter_added,
        iter,
        (gpointer)item);

    idxs = gtk_tree_path_get_indices(path);
    g_return_if_fail(idxs != NULL);
    g_message("INSERT: %d", idxs[0]);

    if ( view_is_path_in_visible_range(priv, idxs[0]) ) {
        view_add_item(priv, item, idxs[0]);
    }

}

static GFile *
model_get_file(ClutterCoverFlowPrivate *priv, GtkTreeIter *iter)
{
    GFile *file = NULL;
    gtk_tree_model_get(GTK_TREE_MODEL(priv->model), iter, priv->file_column, &file, -1);
    return file;
}

int
model_get_length(ClutterCoverFlowPrivate *priv)
{
    return gtk_tree_model_iter_n_children ( GTK_TREE_MODEL(priv->model), NULL );
}

int
model_get_position(ClutterCoverFlowPrivate *priv, GtkTreeIter *iter)
{
    GtkTreePath *path;
    gint *idx;

    path = gtk_tree_model_get_path( GTK_TREE_MODEL(priv->model), iter );
    idx = gtk_tree_path_get_indices(path);
    gtk_tree_path_free(path);

    return idx[0];    
}

gboolean
model_is_empty(ClutterCoverFlowPrivate *priv)
{
    return model_get_length(priv) <= 0;
}

GFile *
model_get_front_file(ClutterCoverFlowPrivate *priv)
{
    g_critical("TODO: %s", G_STRFUNC);
    return NULL;
}

void
model_row_inserted(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ClutterCoverFlowPrivate *priv)
{
    GFile *file;

    g_message("Inserted: %s", G_STRFUNC);

    file = model_get_file(priv, iter);
    if (file) {
        model_do_insert(priv, iter, path, file);
    }
}

void
model_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ClutterCoverFlowPrivate *priv)
{
    /* If the user adds a file to the model using the following idiom
            gtk_list_store_append (...);
            gtk_list_store_set (...);

        then we will get signals in the following order
            row-inserted (file = NULL)
            row-changed (file valid, not yet added)
            row-deleted (on old file)
    */

    g_message("Changed: %s", G_STRFUNC);

    GFile *file;
    file = model_get_file(priv, iter);

    g_return_if_fail(file != NULL);

    if ( NULL == g_hash_table_lookup(priv->iter_added, iter) )
        model_do_insert(priv, iter, path, file);
    else
        g_critical("TODO: %s UPDATE: %s", G_STRFUNC, g_file_get_uri(file));

}

void
model_row_reordered(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer arg3, ClutterCoverFlowPrivate *priv)
{
    g_critical("TODO: %s", G_STRFUNC);
}

void
model_row_deleted(GtkTreeModel *model, GtkTreePath *path, ClutterCoverFlowPrivate *priv)
{
    g_critical("TODO: %s", G_STRFUNC);
}

void
model_add_file(ClutterCoverFlowPrivate *priv, GFile *file, ClutterCoverFlowGetInfoCallback cb)
{
    GtkTreeIter iter;

    g_return_if_fail( G_IS_FILE(file) );
    /* FIXME: Dynamic way to do this? */
    g_return_if_fail( priv->model_is_list_store );

#if 0
    gtk_list_store_insert_with_values (priv->model,
                                       &iter,           /* returned iter */
                                       G_MAXINT,        /* insert at end */
                                       priv->file_column, file,
                                       -1);
#else
    gtk_list_store_append ( 
            GTK_LIST_STORE(priv->model), &iter);
    gtk_list_store_set (
            GTK_LIST_STORE(priv->model),
            &iter,
            priv->file_column, file,
            -1);
#endif
}

static gboolean
view_is_path_in_visible_range(ClutterCoverFlowPrivate *priv, int pos)
{
    return (pos >= priv->idx_visible_start) && (pos <= priv->idx_visible_end);
}

static void
view_restack(ClutterCoverFlowPrivate *priv)
{
    int i;
    CoverFlowItem *j, *k, *l;

#define SWAP(a,b,tmp) { tmp = a; a = b; b = tmp; }
    /* Restack items on left */
    j = priv->visible_items[0];
    for (i = priv->idx_visible_start + 1; i < priv->idx_visible_front; i++) {
        k = priv->visible_items[i - priv->idx_visible_start];
        clutter_actor_raise(k->container, j->container);
        SWAP(j,k,l);
    }
    /* the front one */
    j = priv->visible_items[priv->idx_visible_front - priv->idx_visible_start];
    /* Restack items on right */
    for (i = priv->idx_visible_front + 1; i <= priv->idx_visible_end; i++) {
        k = priv->visible_items[i - priv->idx_visible_start];
        clutter_actor_lower(k->container, j->container);
        SWAP(j,k,l);
    }
}

static int
view_calc_dist_from_front(ClutterCoverFlowPrivate *priv, int pos)
{
    int dist_from_front;
    dist_from_front = (priv->idx_visible_front - priv->idx_visible_start) - pos;
    dist_from_front *= -1; /* FIXME: I think the semantic meaning of the sign here is probbably backwards */
    return dist_from_front;
}

void
view_add_item(ClutterCoverFlowPrivate *priv, CoverFlowItem *item, int pos)
{
    int dist_from_front;
    int bps;
    float scale, dist;
    int angle, opacity;
    int n_visible_items;
    GdkPixbuf *pb;
    ClutterRotateDirection rotation_dir;

    g_return_if_fail(item != NULL);
    g_return_if_fail(item->file != NULL);
    g_return_if_fail(item->get_info_callback != NULL);

    n_visible_items = view_n_visible_items(priv);
    g_return_if_fail(n_visible_items < VISIBLE_ITEMS);

    g_debug("ADD AT POS: %d STACK %d - %d - %d", pos, priv->idx_visible_start, priv->idx_visible_front, priv->idx_visible_end);

    /* adjust the position about the start index */
    pos -= priv->idx_visible_start;

    /* Get the information about the item */
    item->get_info_callback(
            item->file,
            &(item->display_name),
            &(item->display_type),
            &pb,
            DEFAULT_ICON_SIZE);

    item->texture = black_texture_new();
    if( gdk_pixbuf_get_has_alpha(pb) )
        bps = 4;
    else
        bps = 3;

    clutter_texture_set_from_rgb_data(
                CLUTTER_TEXTURE(item->texture), 
                gdk_pixbuf_get_pixels       (pb),
                gdk_pixbuf_get_has_alpha    (pb),
                gdk_pixbuf_get_width        (pb),
                gdk_pixbuf_get_height       (pb),
                gdk_pixbuf_get_rowstride    (pb),
                bps,
                (ClutterTextureFlags)0,
                NULL);

    scale_to_fit (item->texture);

    /* Reflection */
    //item->reflection = clutter_clone_new ( item->texture );
    item->reflection = black_texture_new();

    clutter_texture_set_from_rgb_data(
                CLUTTER_TEXTURE(item->reflection), 
                gdk_pixbuf_get_pixels       (pb),
                gdk_pixbuf_get_has_alpha    (pb),
                gdk_pixbuf_get_width        (pb),
                gdk_pixbuf_get_height       (pb),
                gdk_pixbuf_get_rowstride    (pb),
                bps,
                (ClutterTextureFlags)0,
                NULL);

    g_object_unref(pb);

    //clutter_actor_set_opacity ( item->reflection, 60);
    scale_to_fit ( item->reflection );
    clutter_actor_set_position ( item->reflection, 0, clutter_actor_get_height(item->reflection) );
    clutter_actor_set_rotation (
                item->reflection,
                CLUTTER_Z_AXIS,180,
                clutter_actor_get_width(item->reflection)/2,
                clutter_actor_get_height(item->reflection)/2,
                0);
    clutter_actor_set_rotation (
                item->reflection,
                CLUTTER_Y_AXIS,180,
                clutter_actor_get_width(item->reflection)/2,
                clutter_actor_get_height(item->reflection)/2,
                0);

    /* Container */
    item->container = clutter_group_new();
    clutter_container_add (
                CLUTTER_CONTAINER(item->container),
                item->texture,
                item->reflection,
                NULL);
    clutter_container_add_actor (
                CLUTTER_CONTAINER(priv->m_container),
                item->container);

    /* Calculate the position for the new item, relative to the front */
    dist_from_front = view_calc_dist_from_front(priv, pos);

    scale = get_item_scale(item, dist_from_front);
    dist = get_item_distance(item, dist_from_front);
    opacity = get_item_opacity(item, dist_from_front);
    get_item_angle_and_dir(item, dist_from_front, MOVE_LEFT /* FIXME */, &angle, &rotation_dir);

    /* Dont animate the item position, just put it there */
    clutter_actor_set_rotation (
            item->container,
            CLUTTER_Y_AXIS, angle,
            clutter_actor_get_width(item->texture)/2,
            0,0);
    clutter_actor_set_scale_full (
            item->container,
            scale, scale,
            clutter_actor_get_width(item->texture)/2,
            clutter_actor_get_height(item->texture)/2);
    clutter_actor_set_position ( 
            item->container, 
            dist - clutter_actor_get_width(item->texture)/2, 
            VERTICAL_OFFSET - clutter_actor_get_height(item->texture));

    /* Update the text. For > 1 items it is done when we animate
     * the new front into view */
    if(n_visible_items == 0) {
        update_item_text(priv, item);
    }

    /* Animate the fade in of the new item */
    fade_in (priv, item, dist_from_front);

    if (pos == n_visible_items) {
        /* if item added at the end, simply put it the lowest in the stack */
        /* FIXME: Remove old item */
        priv->visible_items[pos] = item;
        clutter_actor_lower_bottom(item->container);
    } else {
        /* move all items after the new item position along one */
        CoverFlowItem *a;
        int i, dist;

        /* FIXME: Remove old, first item */

        for (i = n_visible_items; i > pos; i--) {
            priv->visible_items[i] = priv->visible_items[i-1];
            dist = view_calc_dist_from_front(priv, i);
            a = priv->visible_items[i];
            animate_item_to_new_position(priv, a, dist, MOVE_LEFT);
        }

        priv->visible_items[pos] = item;
    }

    /* Restack */
    view_restack(priv);

    priv->idx_visible_end += 1;
}

/*
 * Moves all items that should be moved
 *
 *             ___
 *     __  __  |  | __  __  __  __  
 *     |_| |_| |__| |_| |_| |_| |_|
 *     
 *      ^       ^    ^           ^
 *      |       |    |           |
 *      |       |    |           +- iter_visible_end
 *      |       |    +------------- iter_new_front
 *      |       +------------------ iter_visible_front
 *      +-------------------------- iter_visible_start
 *
 *  <--- left --|
 *              |--- right --->
*/
static int
view_move_covers_to_new_positions(ClutterCoverFlowPrivate *priv, move_t dir)
{
//    int j;
    int n_items;
    int new_front, new_front_next, new_front_prev;
//    CoverFlowItem *item;

    /* If a one item list then do nothing */
    n_items = model_get_length(priv);
    if (n_items <= 1)
        return -1;

    /* First take the object on the other (relative to dir) side of the
     * centre and It becomes the new front
     */
    if (dir == MOVE_LEFT) {
        /* Already at the end, the front does not move ? */
        if ( priv->idx_visible_front == priv->idx_visible_end) {
            new_front = priv->idx_visible_front;
            new_front_next = priv->idx_visible_front;
            new_front_prev = priv->idx_visible_front - 1;
            g_debug("MOVE: Front at end");
        } else {
            new_front = priv->idx_visible_front + 1;
            /* Now at the end  ? */
            if ( new_front == priv->idx_visible_end ) {
                new_front_next = priv->idx_visible_end;
                new_front_prev = priv->idx_visible_end - 1;
                g_debug("MOVE: New front at end");
            } else {
                new_front_next = new_front + 1;
                new_front_prev = new_front - 1;
                g_debug("MOVE: Front <-- left --|");
            }
        }
    } else if (dir == MOVE_RIGHT) {
        /* Already at the start, the front does not move ? */
        if ( priv->idx_visible_front == priv->idx_visible_start ) {
            new_front = priv->idx_visible_front;
            new_front_next = priv->idx_visible_front + 1;
            new_front_prev = priv->idx_visible_front;
            g_debug("MOVE: Front at start");
        } else {
            new_front = priv->idx_visible_front - 1;
            /* Now at the start ? */
            if ( new_front == priv->idx_visible_start ) {
                new_front_next = priv->idx_visible_start + 1;
                new_front_prev = priv->idx_visible_start;
                g_debug("MOVE: New front at start");
            } else {
                new_front_next = priv->idx_visible_front + 1;
                new_front_prev = priv->idx_visible_front - 1;
                g_debug("MOVE: Front |-- right -->");
            }
        }
    }
    else {
        g_critical("Unknown move");
        return -1;
    }

#if 0
    item = g_sequence_get(iter_new_front);

    /* Move the new front item into place */
    animate_item_to_new_position(self, item, 0, dir);

    /* Update the text */
    update_item_text (self, item);

    /* Move, scale and rotate all the elements on the left of the new center */
    for (   iter = iter_new_front_prev, j = -1; 
            iter;
            iter = g_sequence_iter_prev(iter), j -= 1)
    {
        item = g_sequence_get(iter);
        animate_item_to_new_position(self, item, j, dir);

        if(clutter_actor_get_depth(item->container) <= 0)
            clutter_actor_lower_bottom(item->container);

        if (iter == priv->iter_visible_start)
            break;
    }

     /* Move, scale and rotate all the elements on the right of the new center */
    for (   iter = iter_new_front_next, j = 1;
            iter;
            iter = g_sequence_iter_next(iter), j += 1)
    {
        item = g_sequence_get(iter);
        animate_item_to_new_position(self, item, j, dir);

        if(clutter_actor_get_depth(item->container) <= 0)
            clutter_actor_lower_bottom(item->container);

        if (iter == priv->iter_visible_end)
            break;
    }

    return iter_new_front;
    #endif

    return priv->idx_visible_front;
}

#if 0
static void
move_end_iters(ClutterCoverFlowPrivate *priv, move_t dir)
{
    CoverFlowItem *item;
    GtkTreeIter *iter;

    if (dir == MOVE_LEFT) {
        iter = g_sequence_iter_next(priv->iter_visible_end);

        /* Are we at the end ? */
        if ( iter == g_sequence_get_end_iter(priv->_items) )
            return;

        /* Move the end along, and add a new item there */        
        priv->iter_visible_end = iter;
        item = g_sequence_get(priv->iter_visible_end);
        view_add_item(coverflow, item, MOVE_LEFT);

        /* Move the start along, and remove the item that was there */
        item = g_sequence_get(priv->iter_visible_start);
        item_free_visible(item);
        priv->iter_visible_start = g_sequence_iter_next(priv->iter_visible_start);

        g_debug("MOVE: Moving start and end iters");
        //amtest
        printf("MOVE_LEFT: start and end iters\n");
    }

    if (dir == MOVE_RIGHT) {
        iter = g_sequence_iter_prev(priv->iter_visible_start);

        /* Are we at the start ? */
        if ( iter == g_sequence_get_begin_iter(priv->_items))
            return;

        /* Move the start back, and add a new item there */        
        priv->iter_visible_start = iter;
        item = g_sequence_get(priv->iter_visible_start);
        view_add_item(coverflow, item, MOVE_RIGHT);

        /* Move the end back, and remove the item that was there */
        item = g_sequence_get(priv->iter_visible_end);
        item_free_visible(item);
        priv->iter_visible_end = g_sequence_iter_prev(priv->iter_visible_end);

        g_debug("MOVE: Moving start and end iters");
        //amtest
        printf("MOVE_RIGHT: start and end iters\n");
    }


}
#endif

void
view_move(ClutterCoverFlowPrivate *priv, move_t dir, gboolean move_ends)
{
//    int curr_pos;
    int new_front_idx;
//    GtkTreeIter *new_front_iter;
    int n_visible_items;

    n_visible_items = view_n_visible_items(priv);
    new_front_idx = view_move_covers_to_new_positions(priv, dir);

    return;

#if 0
    /* Move the start and end iters along one if we are at... */
    if (move_ends) {
        int nitems;

        nitems = model_get_length(priv);
        curr_pos = model_get_position(priv, priv->iter_visible_front);

        if (nitems > n_visible_items && curr_pos >= 5 && curr_pos <= (nitems - 5))
            move_end_iters(priv, dir);
    }

    /* Move the front iter along */
    if (new_front_iter && new_front_iter != priv->iter_visible_front)
        priv->iter_visible_front = new_front_iter;
#endif
}

void
reset(ClutterCoverFlowPrivate *priv)
{
    ClutterActor *stage;
    guint h,w;
    float relation;

    stage = clutter_actor_get_stage(priv->m_container);

    h = clutter_actor_get_height(stage);
    w = clutter_actor_get_width(stage);
    relation = (float)500/(float)h;

    clutter_actor_set_position(
                    priv->m_container,
                    w/2,
                    h/2);

    clutter_actor_set_position (
                    priv->item_name,
                    0 - clutter_actor_get_width(priv->item_name)/2,
                    (MAX_ITEM_HEIGHT/2) + TEXT_PAD_BELOW_ITEM);
    clutter_actor_set_position (
                    priv->item_type,
                    0 - clutter_actor_get_width(priv->item_type)/2,
                    (MAX_ITEM_HEIGHT/2) + TEXT_PAD_BELOW_ITEM + clutter_actor_get_height(priv->item_name) + 5);

    clutter_actor_set_scale(
                    priv->m_container,
                    1/relation,
                    1/relation);

    clutter_text_set_text(
                    CLUTTER_TEXT(priv->item_name),
                    "");
    clutter_actor_set_opacity(
                    priv->item_name,
                    255);

    clutter_text_set_text(
                    CLUTTER_TEXT(priv->item_type),
                    "");
    clutter_actor_set_opacity(
                    priv->item_type,
                    255);

}

void
zoom_items(ClutterCoverFlowPrivate *priv, float zoom_value, gboolean clear_when_complete)
{
#if 0
    GSequenceIter *iter;
    ClutterAnimation *anim;
    ClutterAlpha *alpha;
    ClutterTimeline *timeline;

    timeline = clutter_timeline_new(500);
    alpha = clutter_alpha_new_full(timeline, CLUTTER_EASE_OUT_EXPO);

    /* scale the container */
    anim = clutter_actor_animate_with_alpha (
            priv->m_container,
            alpha,
			"scale-x", zoom_value,
			"scale-y", zoom_value,
			NULL);

    /* shade out all the items */
    for (iter = priv->iter_visible_start;
         TRUE;
         iter = g_sequence_iter_next(iter))
    {
        CoverFlowItem *item;
        item = g_sequence_get(iter);
        clutter_actor_animate_with_alpha (
                    item->texture,
                    alpha,
                    "shade", 0,
                    NULL);
        clutter_actor_animate_with_alpha (
                    item->reflection,
                    alpha,
                    "shade", 0,
                    NULL);
        clutter_timeline_stop(timeline);
        if (iter == priv->iter_visible_end)
            break;
    }

    if (clear_when_complete) {
        g_signal_connect (
                    timeline, "completed",
                    G_CALLBACK (items_free_all), priv);
    } else {
        g_signal_connect (
                    timeline, "completed",
                    G_CALLBACK (items_show_all), priv);
    }

    clutter_timeline_start(timeline);
#endif
}

void
knock_down_items(ClutterCoverFlowPrivate *priv, gboolean clear_when_complete)
{
#if 0
    GSequenceIter *iter;
    ClutterTimeline *timeline;
    ClutterAlpha *alpha;

    timeline = clutter_timeline_new(500);
    alpha = clutter_alpha_new_full (timeline,CLUTTER_EASE_OUT_EXPO);

    for (iter = priv->iter_visible_start;
         TRUE;
         iter = g_sequence_iter_next(iter))
    {
        ClutterBehaviour *up, *down;
        CoverFlowItem *item;

        item = g_sequence_get(iter);

        down = clutter_behaviour_rotate_new (
                    alpha,
                    CLUTTER_X_AXIS,
                    CLUTTER_ROTATE_CCW,
                    clutter_actor_get_rotation(item->texture,CLUTTER_Y_AXIS,0,0,0),
                    270);
        clutter_behaviour_rotate_set_center ( 
                    CLUTTER_BEHAVIOUR_ROTATE(down),
                    0,
                    clutter_actor_get_height(item->texture),
                    0);
        clutter_behaviour_apply (down, item->texture);

        up = clutter_behaviour_rotate_new (
                    alpha,
                    CLUTTER_X_AXIS,
                    CLUTTER_ROTATE_CW,
                    clutter_actor_get_rotation(item->texture,CLUTTER_Y_AXIS,0,0,0),
                    360-270);  
        clutter_behaviour_rotate_set_center ( 
                    CLUTTER_BEHAVIOUR_ROTATE(up),
                    0,
                    clutter_actor_get_height(item->reflection),
                    0);
        clutter_behaviour_apply (up, item->reflection);

        /* Animate with alpha starts the timeline */
        clutter_actor_animate_with_alpha (
                    item->texture,
                    alpha,
                    "shade", 0,
                    NULL);
        clutter_actor_animate_with_alpha (
                    item->reflection,
                    alpha,
                    "shade", 0,
                    NULL);
        clutter_timeline_stop(timeline);
        if (iter == priv->iter_visible_end)
            break;
    }

    /* Also fade out the text */
    clutter_actor_animate_with_alpha (
                priv->item_name,
                alpha,
                "opacity", 0,
                NULL);
    clutter_actor_animate_with_alpha (
                priv->item_type,
                alpha,
                "opacity", 0,
                NULL);

    if (clear_when_complete) {
        g_signal_connect (
                    timeline, "completed",
                    G_CALLBACK (items_free_all), priv);
    } else {
        g_signal_connect (
                    timeline, "completed",
                    G_CALLBACK (items_show_all), priv);
    }

    clutter_timeline_start(timeline);
#endif
}

#if 0
GSequenceIter *
get_actor_iter(ClutterCoverFlowPrivate *priv, ClutterActor * actor)
{
    GSequenceIter *iter;

    if ( g_sequence_get_length(priv->_items) == 0 )
        return NULL;

    for (iter = priv->iter_visible_start;
         TRUE;
         iter = g_sequence_iter_next(iter))
    {
        CoverFlowItem *item = g_sequence_get(iter);
        if (item->texture == actor)
            return iter;
        if (iter == priv->iter_visible_end)
            break;
    }

    return NULL;
}
#endif

/*
 * This functions adds a rotation behaviour from the current angle to the final angle 
 * rotating with the direction <direction> 
 */
void
set_rotation_behaviour (ClutterCoverFlowPrivate *priv, CoverFlowItem *item, int final_angle, ClutterRotateDirection direction)
{
    double current;

    current = clutter_actor_get_rotation(item->container,CLUTTER_Y_AXIS,0,0,0);
    if(current<0) current+=360;
    if(current>360) current-=360;

    if(current != final_angle) {
        item->rotateBehaviour = clutter_behaviour_rotate_new (
                    priv->m_alpha,
                    CLUTTER_Y_AXIS,
                    direction,
                    current,
                    final_angle);
        clutter_behaviour_rotate_set_center ( 
                    CLUTTER_BEHAVIOUR_ROTATE(item->rotateBehaviour),
                    clutter_actor_get_width(item->container)/2,
                    0,0);
        clutter_behaviour_apply (item->rotateBehaviour, item->container);
    }
}

float
get_item_scale(CoverFlowItem *item, int dist_from_front)
{
    if(dist_from_front == 0 )
        return MAX_SCALE;
    else
        return 1.0;
}

void
get_item_angle_and_dir(CoverFlowItem *item, int dist_from_front, move_t dir, int *angle, ClutterRotateDirection *rotation_dir)
{
    /* The front item direction depends on the direction we came from */
    if (dist_from_front == 0) {
        *rotation_dir =  (dir == MOVE_RIGHT ? CLUTTER_ROTATE_CCW : CLUTTER_ROTATE_CW);
        *angle = 0;
    }

    /* Item on the right */
    if (dist_from_front > 0) {
        *rotation_dir = CLUTTER_ROTATE_CCW;
        *angle = 360 - MAX_ANGLE;
    }

    /* Item on the left */
    if (dist_from_front < 0) {
        *rotation_dir = CLUTTER_ROTATE_CW;
        *angle = MAX_ANGLE;
    }
}

gfloat
get_item_distance(CoverFlowItem *item, int dist_from_front)
{
    gfloat dist = (ABS(dist_from_front) * COVER_SPACE) + FRONT_COVER_SPACE;

    if (dist_from_front == 0)
        return 0.0;

    if (dist_from_front < 0)
        dist *= -1.0;

    return dist;
}

int
get_item_opacity(CoverFlowItem *item, int dist_from_front)
{
    return CLAMP(255*(VISIBLE_ITEMS - ABS(dist_from_front))/VISIBLE_ITEMS, 0, 255);
}

int
get_item_reflection_opacity(CoverFlowItem *item, int dist_from_front)
{
    return CLAMP((REFLECTION_ALPHA*(VISIBLE_ITEMS - ABS(dist_from_front))/VISIBLE_ITEMS), 0, REFLECTION_ALPHA);
}

void
animate_item_to_new_position(ClutterCoverFlowPrivate *priv, CoverFlowItem *item, int dist_from_front, move_t dir)
{
    float scale, dist;
    int angle, opacity, reflection_opacity;
    ClutterRotateDirection rotation_dir = 0;

    scale = get_item_scale(item, dist_from_front);
    dist = get_item_distance(item, dist_from_front);
    opacity = get_item_opacity(item, dist_from_front);
    reflection_opacity = get_item_reflection_opacity(item, dist_from_front);
    get_item_angle_and_dir(item, dist_from_front, dir, &angle, &rotation_dir);

    set_rotation_behaviour(priv, item, angle, rotation_dir);

    clutter_actor_animate_with_alpha (item->texture, priv->m_alpha,
                                      "shade", opacity,
                                      NULL);
    clutter_actor_animate_with_alpha (item->reflection, priv->m_alpha,
                                      "shade", reflection_opacity,
                                      NULL);

    clutter_actor_animate_with_alpha (
                            item->container,
                            priv->m_alpha,
                            "scale-x", scale,
                            "scale-y", scale,
                            "scale-center-x" , clutter_actor_get_width  (item->texture)/2,
                            "scale-center-y" , clutter_actor_get_height (item->texture)/2,
                            "x", dist - clutter_actor_get_width(item->container)/2,
                            NULL);
}

void
update_item_text(ClutterCoverFlowPrivate *priv, CoverFlowItem *item)
{
    /* Set text in the centre to the name of the file */
    clutter_text_set_text(
                CLUTTER_TEXT(priv->item_name),
                item->display_name);
    clutter_actor_set_x(
                priv->item_name, 
                0 - clutter_actor_get_width(priv->item_name)/2);
    clutter_text_set_text(
                CLUTTER_TEXT(priv->item_type),
                item->display_type);
    clutter_actor_set_x(
                priv->item_type, 
                0 - clutter_actor_get_width(priv->item_type)/2);
}

void
start(ClutterCoverFlow *self)
{
    clutter_timeline_start(self->priv->m_timeline);
}

void
stop(ClutterCoverFlow *self)
{
    clutter_timeline_stop(self->priv->m_timeline);
}

void
clear_behaviours (ClutterCoverFlow *self)
{
#if 0
    g_sequence_foreach_range(
        self->priv->iter_visible_start,
        self->priv->iter_visible_end,
        (GFunc)item_clear_behavior,
        NULL);
#endif
}

void
fade_in(ClutterCoverFlowPrivate *priv, CoverFlowItem *item, guint distance_from_centre)
{
    ClutterTimeline *timeline = clutter_timeline_new(FRAMES * FPS);
    ClutterAlpha    *alpha = clutter_alpha_new_full(timeline,CLUTTER_EASE_OUT_EXPO);

    int opacity = get_item_opacity(item, distance_from_centre);
    int reflection_opacity 	= get_item_reflection_opacity(item, distance_from_centre);

    clutter_actor_animate_with_alpha (item->texture, alpha, "shade", opacity, NULL);
    clutter_actor_animate_with_alpha (item->reflection, alpha , "shade", reflection_opacity, NULL);
    //clutter_timeline_start (timeline);
}

void
scale_to_fit(ClutterActor *actor)
{
    int w = clutter_actor_get_width(actor);
    int h = clutter_actor_get_height(actor);

    if( h > MAX_ITEM_HEIGHT) {
        int temp = w*MAX_ITEM_HEIGHT/h;
        clutter_actor_set_size(actor, temp, MAX_ITEM_HEIGHT);
    }
}

void
get_info(GFile *file, char **name, char **description, GdkPixbuf **pb, guint pbsize)
{
    GIcon *icon;
    GtkIconInfo *icon_info;
    GFileInfo *file_info;
    GtkIconTheme *icon_theme;
    char *uri;

	uri = g_file_get_uri(file);
	g_debug("Default get info: %s", uri);
	g_free(uri);

    icon_theme = gtk_icon_theme_get_default();
    file_info = g_file_query_info(
                    file,
                    G_FILE_ATTRIBUTE_STANDARD_ICON "," G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE,
                    G_FILE_QUERY_INFO_NONE, NULL , NULL);
    icon = g_file_info_get_icon(file_info);
    icon_info = gtk_icon_theme_lookup_by_gicon(
                    icon_theme,
                    icon,
                    pbsize,
                    GTK_ICON_LOOKUP_USE_BUILTIN | GTK_ICON_LOOKUP_GENERIC_FALLBACK | GTK_ICON_LOOKUP_FORCE_SIZE);

    *pb = gtk_icon_info_load_icon(icon_info, NULL);
    *name = g_strdup(
                g_file_info_get_display_name(file_info));
    *description = g_content_type_get_description(
                        g_file_info_get_attribute_string(
                            file_info,
                            G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE));

    g_object_unref(file_info);
    gtk_icon_info_free(icon_info);
}

