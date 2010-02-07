#include "clutter-cover-flow-internal.h"

typedef struct _AsyncTextureLoadData
{
    CoverFlowItem   *item;
    gint            pos;
    gboolean        texture_finished;
    gboolean        reflection_finished;
} AsyncTextureLoadData;

static AsyncTextureLoadData *
async_texture_load_data_new(void)
{
    return g_new0 (AsyncTextureLoadData, 1);
}

static void 
async_texture_load_data_maybe_free(AsyncTextureLoadData *data)
{
    if (data && data->texture_finished && data->reflection_finished)
        g_free(data);
}

void
item_clear_behavior (CoverFlowItem *item, gpointer user_data)
{
    g_return_if_fail(item != NULL);

    if ( item->rotateBehaviour && 
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

    if (item->container)
        clutter_actor_destroy(item->container);
    /* clutter_actor_destroy remove all childs if it s a container 
    ** etc so below is not really necessary */
#if 0
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
#endif
}

void 
item_free_invisible(CoverFlowItem *item)
{
    if(item!=NULL)
    {
        item_free_visible(item);
        g_object_unref(item->file);
        g_free(item);
    }
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

static void
get_item_count(ClutterCoverFlowPrivate *priv, GFile *file)
{
    /*if (!priv->m_item_count)
      {*/
    guint count;
    gboolean unreadable;

    //printf("mmm: %s\n", g_file_get_uri(file));
    /* FIXME: store the folder in CoverFlowPrivate to avoid recalculating
    ** for each file added to the directory */
    if (nautilus_file_get_directory_item_count(nautilus_file_get_parent(nautilus_file_get(file)), &count, &unreadable))
        priv->m_item_count = count;
    //}
}

#if 0
static int
view_calc_dist_from_front(ClutterCoverFlowPrivate *priv, int pos)
{
    int dist_from_front;
    dist_from_front = (priv->idx_visible_front - priv->idx_visible_start) - pos;
    dist_from_front *= -1; /* FIXME: I think the semantic meaning of the sign here is probbably backwards */
    return dist_from_front;
}
#endif

CoverFlowItem *item_new(ClutterCoverFlowPrivate *priv, GtkTreeIter *iter)
{
    gpointer cb;
    CoverFlowItem *item;
    GFile *file;

    /* FIXME: we got a CRITICAL here for folders: 
    ** CRITICAL **: nautilus_file_get_location: assertion `NAUTILUS_IS_FILE (file)' failed */
    gtk_tree_model_get (priv->model, iter, priv->file_column, &file, -1);
    if (file == NULL)
    {
        printf("WWW file is null :/\n");
        return NULL;
    }
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

    /* amtest do we need that for later? */
    /* for looking up iters by uri */
#if 0
    g_hash_table_insert(
                        priv->uri_to_item_map,
                        g_file_get_uri(file),   /* freed by hashtable KeyDestroyFunc */
                        iter);

    g_hash_table_insert(
                        priv->iter_added,
                        iter,
                        (gpointer)item);
#endif
    return (item);
}

CoverFlowItem *item_grab_index(ClutterCoverFlowPrivate *priv, int idx)
{
    GtkTreePath *path;
    GtkTreeIter iter;

    if (idx >= 0)
    {
        path = gtk_tree_path_new_from_indices(idx, -1);
        if (gtk_tree_model_get_iter (priv->model, &iter, path))
        {
            gtk_tree_path_free (path);
            return (item_new(priv, &iter));
        }
        gtk_tree_path_free (path);
    }
    return NULL;
}

static gboolean
foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
    //gpointer cb;
    CoverFlowItem *item;
    //GFile *file;
    ClutterCoverFlowPrivate *priv;

    priv = user_data;
    //gtk_tree_model_get (model, iter, priv->file_column, &file, -1);
    gint *idxs = gtk_tree_path_get_indices(path);
    //item = item_new(priv, iter);
    //printf("ADDED: %s %d\n", g_file_get_uri(item->file), idxs[0]);

    //if (view_is_path_in_visible_range(priv, idxs[0]))
    item = item_new(priv, iter);
    //int dist = view_calc_dist_from_front(priv, idxs[0]);
    if (item != NULL)
    {
        printf("ADDED: %s %d dist:%d index:%d\n", g_file_get_uri(item->file), idxs[0], idxs[0], idxs[0]+VISIBLE_ITEMS/2);
        priv->visible_items[idxs[0]+VISIBLE_ITEMS/2] = item;
        /* let appear all items in position 0 to get a nice first animation */
        view_add_item(priv, item, 0);
        animate_item_to_new_position(priv, item, idxs[0], MOVE_LEFT);
    }

    if (idxs[0] >= VISIBLE_ITEMS/2) 
        return TRUE;
    return FALSE;
}

/* Return the indice of the tab where it match otherwise return -1 */
static int
is_incfitems(CoverFlowItem *item, CoverFlowItem **t)
{
    /* FIXME: we are calling g_file_get_uri ..it must be done before in item_new
    ** maybe ? */
    int i;
    for (i=0; i<VISIBLE_ITEMS; i++)
    {
        if (t[i])
        {
            //printf ("STRCMP[%d] %s %s\n", i, g_file_get_uri(priv->onstage_items[i]->file), g_file_get_uri(item->file));
            if (!strcmp(g_file_get_uri(t[i]->file), g_file_get_uri(item->file)))
                return i;
        }
    }
    return -1;
}

#if 0
static gboolean
foreach_func2 (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
    //GFile *file;
    ClutterCoverFlowPrivate *priv;
    /*CoverFlowItem *item;
    int n;*/

    priv = user_data;
    //gtk_tree_model_get (model, iter, priv->file_column, &file, -1);
    gtk_tree_view_get_cursor    (priv->tree, &path, NULL);
    if (path == NULL)
        printf("get cursor FAILED\n");
    gint *idxs = gtk_tree_path_get_indices(path);
    printf(":::: idx front %d selected %d\n", priv->idx_visible_front, idxs[0]);
    priv->idx_visible_front = idxs[0];

/* FIXME: need to debug that part */
#if 0
    if (priv->idx_visible_front - VISIBLE_ITEMS/2 >= 0)
        path = gtk_tree_path_new_from_indices(priv->idx_visible_front -
                                              VISIBLE_ITEMS/2, -1);
    /*GtkTreePath *path2 = gtk_tree_path_new_from_indices(priv->idx_visible_front, -1);
    gint *idxs = gtk_tree_path_get_indices(path2);*/
    //gint *idxs = gtk_tree_path_get_indices(path);
    //printf("ADDED: %s %d\n", g_file_get_uri(item->file), idxs[0]);

    item = item_new(priv, iter);
    //int dist = view_calc_dist_from_front(priv, idxs[0]);
    int dist = idxs[0] - priv->idx_visible_front;
    //printf("ADDED: %s %d dist:%d index:%d\n", g_file_get_uri(item->file), idxs[0], dist, idxs[0]+VISIBLE_ITEMS/2);
    if ((n = is_incfitems(item, priv->onstage_items)) == -1)
    {
        printf("ADDED: %s %d dist:%d index:%d\n", g_file_get_uri(item->file), idxs[0], dist, idxs[0]+VISIBLE_ITEMS/2);
        view_add_item(priv, item, idxs[0]);
    }
    else
    {
        printf("REUSE: %s %d dist:%d index:%d\n", g_file_get_uri(item->file), idxs[0], dist, idxs[0]+VISIBLE_ITEMS/2);
        //item = priv->onstage_items[idxs[0]+VISIBLE_ITEMS/2];
        item = priv->onstage_items[n];
    }
    priv->visible_items[idxs[0]+VISIBLE_ITEMS/2] = item;
    animate_item_to_new_position(priv, item, dist, MOVE_LEFT);

    if (idxs[0] >= priv->idx_visible_front + VISIBLE_ITEMS/2) 
        return TRUE;
#endif
    return FALSE;
}
#endif

static void 
duplicate_visible_items(ClutterCoverFlowPrivate *priv)
{
    int i;

    for (i=0; i<VISIBLE_ITEMS; i++)
        priv->onstage_items[i] = priv->visible_items[i];
}

static void
remove_outof_range_actors(ClutterCoverFlowPrivate *priv)
{
    int i;

    for (i=0; i<VISIBLE_ITEMS; i++)
        if (priv->onstage_items[i])
        {
            //printf ("test: %20s %20s\n", g_file_get_uri(priv->onstage_items[i]->file), g_file_get_uri(priv->visible_items[i]->file));
            if (is_incfitems(priv->onstage_items[i], priv->visible_items) == -1)
            {
                printf ("delete: %s\n", g_file_get_uri(priv->onstage_items[i]->file));
                item_free_invisible(priv->onstage_items[i]);
            }
        }
}

void
items_update(ClutterCoverFlowPrivate *priv)
{
    GtkTreePath *path;
    //GtkTreeIter *iter;
    CoverFlowItem *item;
    int i, n;

    g_message("%s", G_STRFUNC);
    //gtk_tree_model_foreach(priv->model, foreach_func2, priv);
    
    //gtk_tree_model_get (model, iter, priv->file_column, &file, -1);
    gtk_tree_view_get_cursor    (priv->tree, &path, NULL);
    if (path == NULL)
        g_critical("get cursor FAILED\n");
    gint *idxs = gtk_tree_path_get_indices(path);
    char *spath = gtk_tree_path_to_string(path);
    g_message("ITEMS_UPDATE: idx front %d selected %s\n", priv->idx_visible_front, spath);
    priv->idx_visible_front = idxs[0];

    for (i=0; i<VISIBLE_ITEMS; i++)
    {
        item = item_grab_index(priv, idxs[0]-VISIBLE_ITEMS/2+i);
        if (item != NULL)
        {
            if ((n = is_incfitems(item, priv->onstage_items)) == -1)
            {
                g_message("ADDED: %s %d dist:%d index:%d\n", g_file_get_uri(item->file), idxs[0]-VISIBLE_ITEMS/2+i, 
                        i-VISIBLE_ITEMS/2, i);
                view_add_item(priv, item, i-VISIBLE_ITEMS/2);
            }
            else
            {
                g_message("REUSE: %s %d dist:%d index:%d\n", g_file_get_uri(item->file), idxs[0]-VISIBLE_ITEMS/2+i, 
                       i-VISIBLE_ITEMS/2, i);
                //item = priv->onstage_items[idxs[0]+VISIBLE_ITEMS/2];
                item = priv->onstage_items[n];
            }
            animate_item_to_new_position(priv, item, i-VISIBLE_ITEMS/2, MOVE_LEFT);
        }
        priv->visible_items[i] = item;
    }

    remove_outof_range_actors(priv);
    duplicate_visible_items(priv);
    view_restack(priv);

    gtk_tree_path_free(path);
}

static void
model_do_insert(ClutterCoverFlowPrivate *priv, GtkTreeIter *iter, GtkTreePath *path, GFile *file)
{
    //gint *idxs;
    //CoverFlowItem *item;

    /* get the number of files of current dir */
    get_item_count(priv, file);

    //idxs = gtk_tree_path_get_indices(path);
    //g_return_if_fail(idxs != NULL);
    int nbs = model_get_length(priv);
    //g_message("INSERT: %d %s [%d/%d]", idxs[0], g_file_get_uri(file), nbs, priv->m_item_count);
    //g_message("INSERT: %s [%d/%d]", g_file_get_uri(file), nbs, priv->m_item_count);

    /* object appear inverted, not in good oder and gtk_tree_path_get_indices
    ** return incomprehensive results so we need to wait all the
    files before processing */
    //printf(":: nbs: %d m_item_count: %d\n", nbs, priv->m_item_count);
    if (nbs == priv->m_item_count)
    {
        if (!priv->visible_items)
        {
            priv->visible_items = g_new0 (CoverFlowItem*, VISIBLE_ITEMS);
            gtk_tree_model_foreach(priv->model, foreach_func, priv);
            duplicate_visible_items(priv);
            printf("front fixed -- path: %s\n", gtk_tree_path_to_string(path));
        }
        else
        {
            items_update(priv);
        }
    }
    /*
       if (nbs > priv->m_item_count)
       {
       gint *idxs = gtk_tree_path_get_indices(path);
       item = item_new(priv, iter);
       printf("got a WINNER: %d\n", idxs[0]);
       }*/

#if 0
    CoverFlowItem *item = item_new(priv, iter);

    int pos = gtk_tree_path_get_indices(path)[0];
    int n_visible_items = view_n_visible_items(priv);
    g_return_if_fail(n_visible_items < VISIBLE_ITEMS);

    if (view_is_path_in_visible_range(priv, pos))
    {
        g_message("INSERT: %s [%d]", g_file_get_uri(item->file), pos);
        view_add_item(priv, item, pos);
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
    }
#endif

    /* 
       GtkTreeIter myiter;
       GFile *myfile;
       if (gtk_tree_model_get_iter_first(priv->model, &myiter))
       {
       gtk_tree_model_get (priv->model, &myiter,
       priv->file_column, &myfile,
       -1);
       printf("we got the first: %s\n", g_file_get_uri(myfile));
       if (gtk_tree_model_iter_next(priv->model, &myiter))
       {
       gtk_tree_model_get (priv->model, &myiter,
       priv->file_column, &myfile,
       -1);
       printf("we got: %s\n", g_file_get_uri(myfile));
       }
       }
       */
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
    printf("REORDERED path: %s\n", gtk_tree_path_to_string(path));
    if (path == NULL)
        printf("FAILED\n");
    items_update(priv);
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


void
view_restack(ClutterCoverFlowPrivate *priv)
{

    int i;
    CoverFlowItem *j, *k;// *l;
    
    g_message("Restack");
    /*Restack items in the left*/
    for(i = 0; i< VISIBLE_ITEMS/2; i++)
    {
        j = priv->visible_items[i];
        k = priv->visible_items[i+1];
        if(k != NULL && j != NULL)
            clutter_actor_raise(k->container, j->container);
    }   
    /*Restack items in the Right*/
    for(i = VISIBLE_ITEMS-1; i> VISIBLE_ITEMS/2+1; i--)
    {
        j = priv->visible_items[i];
        k = priv->visible_items[i-1];
        if(k != NULL && j != NULL)
            clutter_actor_raise(k->container, j->container);
    }   

    /*Raise cover*/
    k = priv->visible_items[VISIBLE_ITEMS/2];
    clutter_actor_raise_top(k->container);
    clutter_actor_raise_top(priv->item_name);
    clutter_actor_raise_top(priv->item_type);


    
    #if 0
    #define SWAP(a,b,tmp) { tmp = a; a = b; b = tmp; }
    /* Restack items on left */
    j = priv->visible_items[0];
    for (i = priv->idx_visible_start + 1; i < priv->idx_visible_front; i++) {
        k = priv->visible_items[i - priv->idx_visible_start];
        if(k != NULL && j != NULL)
        {
            clutter_actor_raise(k->container, j->container);
            SWAP(j,k,l);
        }
    }
    /* the front one */
    j = priv->visible_items[priv->idx_visible_front - priv->idx_visible_start];
    /* Restack items on right */
    for (i = priv->idx_visible_front + 1; i <= priv->idx_visible_end; i++) {
        k = priv->visible_items[i - priv->idx_visible_start];
        if(k != NULL && j != NULL)
        {
            clutter_actor_lower(k->container, j->container);
            SWAP(j,k,l);
        }
    }
    #endif

}

static void
scale_to_fit(ClutterActor *actor)
{
    int w = clutter_actor_get_width(actor);
    int h = clutter_actor_get_height(actor);

    if( h > MAX_ITEM_HEIGHT) {
        int temp = w*MAX_ITEM_HEIGHT/h;
        clutter_actor_set_size(actor, temp, MAX_ITEM_HEIGHT);
    }
}

static void
set_reflection(ClutterActor *reflection)
{
    clutter_actor_set_position ( reflection, 0, clutter_actor_get_height(reflection) );
    clutter_actor_set_rotation (
                                reflection,
                                CLUTTER_Z_AXIS,180,
                                clutter_actor_get_width(reflection)/2,
                                clutter_actor_get_height(reflection)/2,
                                0);
    clutter_actor_set_rotation (
                                reflection,
                                CLUTTER_Y_AXIS,180,
                                clutter_actor_get_width(reflection)/2,
                                clutter_actor_get_height(reflection)/2,
                                0);
}

static void
set_texture(CoverFlowItem *item, int pos)
{
    float scale, dist;

    scale = get_item_scale(item, pos);
    dist = get_item_distance(item, pos);

    clutter_actor_set_position ( 
                                item->container, 
                                dist -clutter_actor_get_width(item->texture)/2, 
                                -clutter_actor_get_height(item->texture)/2);

    clutter_actor_set_scale_full (
                                  item->container, scale, scale,
                                  clutter_actor_get_width(item->texture)/2,
                                  clutter_actor_get_height(item->texture)/2);
}

static void 
load_texture_finished (ClutterTexture *texture, const GError *error, AsyncTextureLoadData *data)
{
    ClutterActor *actor = CLUTTER_ACTOR(texture);
    scale_to_fit (actor);
    set_texture (data->item, data->pos);

    data->texture_finished = TRUE;
    async_texture_load_data_maybe_free(data);
}

static void 
load_reflection_finished (ClutterTexture *texture, const GError *error, AsyncTextureLoadData *data)
{
    ClutterActor *actor = CLUTTER_ACTOR(texture);
    scale_to_fit (actor);
    set_reflection (actor);

    data->reflection_finished = TRUE;
    async_texture_load_data_maybe_free(data);
} 

void
view_add_item(ClutterCoverFlowPrivate *priv, CoverFlowItem *item, int pos)
{
    //int dist_from_front;
//    float scale, dist;
    //int angle, opacity;
    GdkPixbuf *pb;
    char *pbpath;
    //ClutterRotateDirection rotation_dir;

    g_return_if_fail(item != NULL);
    g_return_if_fail(item->file != NULL);
    g_return_if_fail(item->get_info_callback != NULL);

    //g_return_if_fail(n_visible_items < VISIBLE_ITEMS);


    /* adjust the position about the start index */
    //pos -= priv->idx_visible_start;

    g_message("ADD AT POS: %d FRONT %d", pos, priv->idx_visible_front);

    /* Get the information about the item */
    item->get_info_callback(
                            item->file,
                            &(item->display_name),
                            &(item->display_type),
                            &pb,
                            &pbpath,
                            DEFAULT_ICON_SIZE);

    item->texture = black_texture_new();
    item->reflection = black_texture_new();

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

    /* prefer using the local path so we can load the texture async */
    if (pbpath) {
        AsyncTextureLoadData *data;

        data = async_texture_load_data_new();
        data->item = item;
        data->pos = pos;

        /* Texture */
        clutter_texture_set_load_async (CLUTTER_TEXTURE(item->texture), TRUE);
        g_signal_connect (item->texture, "load-finished", G_CALLBACK (load_texture_finished), data);
        clutter_texture_set_from_file (CLUTTER_TEXTURE(item->texture), pbpath, NULL);

        /* Reflection */
        clutter_texture_set_load_async (CLUTTER_TEXTURE(item->reflection), TRUE);
        g_signal_connect (item->reflection, "load-finished", G_CALLBACK (load_reflection_finished), data);
        clutter_texture_set_from_file (CLUTTER_TEXTURE(item->reflection), pbpath, NULL);

        g_free(pbpath);
    } else if (pb) {
        int bps;

        if( gdk_pixbuf_get_has_alpha(pb) )
            bps = 4;
        else
            bps = 3;

        /* Texture */
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
        set_texture(item, pos);

        /* Reflection */
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
        scale_to_fit (item->reflection);
        set_reflection (item->reflection);

        g_object_unref(pb);
    } else {
        g_critical("Must provide either pixbuf thumbnail or path to pixbuf thumbnail");
        return;
    }

    fade_in (priv, item, pos);

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
#if 0 
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
            //g_debug("MOVE: Front at end");
            g_message("MOVE LEFT: Front at end");
        } else {
            new_front = priv->idx_visible_front + 1;
            /* Now at the end  ? */
            if ( new_front == priv->idx_visible_end ) {
                new_front_next = priv->idx_visible_end;
                new_front_prev = priv->idx_visible_end - 1;
                //g_debug("MOVE: New front at end");
                g_message("MOVE LEFT: New front at end");
            } else {
                new_front_next = new_front + 1;
                new_front_prev = new_front - 1;
                //g_debug("MOVE: Front <-- left --|");
                g_message("MOVEi LEFT: Front <-- left --|");
            }
        }
    } else if (dir == MOVE_RIGHT) {
        /* Already at the start, the front does not move ? */
        if ( priv->idx_visible_front == priv->idx_visible_start ) {
            new_front = priv->idx_visible_front;
            new_front_next = priv->idx_visible_front + 1;
            new_front_prev = priv->idx_visible_front;
            //g_debug("MOVE: Front at start");
            g_message("MOVE RIGHT: Front at start");
        } else {
            new_front = priv->idx_visible_front - 1;
            /* Now at the start ? */
            if ( new_front == priv->idx_visible_start ) {
                new_front_next = priv->idx_visible_start + 1;
                new_front_prev = priv->idx_visible_start;
                //g_debug("MOVE: New front at start");
                g_message("MOVE RIGHT: New front at start");
            } else {
                new_front_next = priv->idx_visible_front + 1;
                new_front_prev = priv->idx_visible_front - 1;
                //g_debug("MOVE: Front |-- right -->");
                g_message("MOVE RIGHT: Front |-- right -->");
            }
        }
    }
    else {
        //g_critical("Unknown move");
        g_message("Unknown move");
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
#endif

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

    int i, dist;   
    CoverFlowItem *item;

    if(dir == MOVE_LEFT /*&&  priv->idx_visible_front NOT AT THE END*/) /*not at ***O position*/
    { 
        item_free_invisible(priv->visible_items[0]);    /*Free first item*/
        priv->idx_visible_front++;                      /*Update front cover idx*/

        /*     xxOxx ===> xxxOx       */
        for(i=0; i<VISIBLE_ITEMS-1; i++)
        {
            priv->visible_items[i] = priv->visible_items[i+1]; 
            //dist = view_calc_dist_from_front(priv,i) - 3;
            dist = i - VISIBLE_ITEMS/2;

            /*If there is a cover*/        
            if(priv->visible_items[i] != NULL)
            {
                g_message("Moving left %s %i front cover is %i so dist is %i", priv->visible_items[i]->display_name, i , VISIBLE_ITEMS/2, dist);
                animate_item_to_new_position(priv, priv->visible_items[i], dist , dir);
            }
        }
        /* Add the new item*/
        //priv->visible_items[VISIBLE_ITEMS-1] = NULL;
        item = item_grab_index(priv, priv->idx_visible_front + VISIBLE_ITEMS/2);
        priv->visible_items[VISIBLE_ITEMS-1] = item;
        if (item != NULL)
        {
            view_add_item(priv, item, VISIBLE_ITEMS/2);
            animate_item_to_new_position(priv, item, VISIBLE_ITEMS/2, MOVE_LEFT);
        }
        duplicate_visible_items(priv);
    }
    /*TODO: ADD new item and animate it*/
    //if(dir == MOVE_RIGHT &&  priv->idx_visible_front > 0)   /*not at O*** position*/
    if(dir == MOVE_RIGHT)   /*not at O*** position*/
    {
        /*Free last item*/
        item_free_invisible(priv->visible_items[VISIBLE_ITEMS-1]);
        priv->idx_visible_front--;


        for(i=VISIBLE_ITEMS-1; i>0; i--)
        {
            priv->visible_items[i] = priv->visible_items[i-1];
            dist = i - VISIBLE_ITEMS/2;

            /*If there is a cover*/        
            if(priv->visible_items[i] != NULL)
            {
                g_message("Moving left %s %i front cover is %i so dist is %i", priv->visible_items[i]->display_name, i , VISIBLE_ITEMS/2, dist);
                animate_item_to_new_position(priv, priv->visible_items[i], dist, dir);
            }
        } 
        /* Add the new item*/
        //priv->visible_items[0] = NULL;      
        item = item_grab_index(priv, priv->idx_visible_front - VISIBLE_ITEMS/2);
        priv->visible_items[0] = item;
        if (item != NULL)
        {
            view_add_item(priv, item, -VISIBLE_ITEMS/2);
            animate_item_to_new_position(priv, item, -VISIBLE_ITEMS/2, MOVE_LEFT);
        }
        duplicate_visible_items(priv);
    }

    view_restack(priv);
    
    /*Update text*/
    update_item_text(priv, priv->visible_items[VISIBLE_ITEMS/2]);
    


    //    int curr_pos;
    //int new_front_idx;
    //    GtkTreeIter *new_front_iter;
    /*int n_visible_items;

      n_visible_items = view_n_visible_items(priv);
      new_front_idx = view_move_covers_to_new_positions(priv, dir);

      return;*/



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


/* amtest: usefull? */
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

    /*clutter_text_set_text(
                          CLUTTER_TEXT(priv->item_name),
                          "");*/
    clutter_actor_set_opacity(
                              priv->item_name,
                              255);

    /*clutter_text_set_text(
                          CLUTTER_TEXT(priv->item_type),
                          "");*/
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

int
get_actor_pos(ClutterCoverFlowPrivate *priv, ClutterActor * actor)
{
    int i;

    for (i=0; i<VISIBLE_ITEMS; i++)
    {
        if (priv->visible_items[i])
            if (priv->visible_items[i]->texture == actor)
                return i-VISIBLE_ITEMS/2;
    }
    return 0;
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
    //return CLAMP(255*(VISIBLE_ITEMS - ABS(dist_from_front))/VISIBLE_ITEMS, 0, 255);
    //return CLAMP(255*((VISIBLE_ITEMS/2)+1 - ABS(dist_from_front))/(VISIBLE_ITEMS/2), 0, 255);
    return CLAMP(255*((V_ITEMS/2)+1 - ABS(dist_from_front))/(V_ITEMS/2), 0, 255);
}

int
get_item_reflection_opacity(CoverFlowItem *item, int dist_from_front)
{
    //return CLAMP((REFLECTION_ALPHA*(VISIBLE_ITEMS - ABS(dist_from_front))/VISIBLE_ITEMS), 0, REFLECTION_ALPHA);
    //return CLAMP((REFLECTION_ALPHA*((VISIBLE_ITEMS/2)+1 - ABS(dist_from_front))/(VISIBLE_ITEMS/2)), 0, REFLECTION_ALPHA);
    return CLAMP((REFLECTION_ALPHA*((V_ITEMS/2)+1 - ABS(dist_from_front))/(V_ITEMS/2)), 0, REFLECTION_ALPHA);
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
    clutter_actor_lower_bottom(item->container);
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
    // useless with clutter_actor_animate
    //clutter_timeline_start(self->priv->m_timeline);
}

void
stop(ClutterCoverFlow *self)
{
    clutter_timeline_stop(self->priv->m_timeline);
}

void
clear_behaviours (ClutterCoverFlow *self)
{
    int i;
    ClutterCoverFlowPrivate *priv;

    priv = self->priv;
    if ( ! model_is_empty(priv) ) 
        for (i=0; i<VISIBLE_ITEMS; i++)
            if (priv->visible_items[i])
                if (CLUTTER_IS_BEHAVIOUR(priv->visible_items[i]->rotateBehaviour) && 
                    clutter_behaviour_is_applied(priv->visible_items[i]->rotateBehaviour, priv->visible_items[i]->container) )
                {
                    clutter_behaviour_remove(CLUTTER_BEHAVIOUR(priv->visible_items[i]->rotateBehaviour), priv->visible_items[i]->container);
                }

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
    /*ClutterTimeline *timeline = clutter_timeline_new(FRAMES * FPS);
    ClutterAlpha    *alpha = clutter_alpha_new_full(timeline,CLUTTER_EASE_OUT_EXPO);*/

    int opacity = get_item_opacity(item, distance_from_centre);
    int reflection_opacity 	= get_item_reflection_opacity(item, distance_from_centre);

    clutter_actor_animate_with_alpha (item->texture, priv->m_alpha, "shade", opacity, NULL);
    clutter_actor_animate_with_alpha (item->reflection, priv->m_alpha , "shade", reflection_opacity, NULL);
    //clutter_timeline_start (timeline);
}

void
get_info(GFile *file, char **name, char **description, GdkPixbuf **pb, char **pbpath, guint pbsize)
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
    *name = g_strdup(g_file_info_get_display_name(file_info));
    *description = g_content_type_get_description(
                        g_file_info_get_attribute_string(
                                                       file_info,
                                                       G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE));

    g_object_unref(file_info);
    gtk_icon_info_free(icon_info);
}

