#include "clutter-cover-flow-internal.h"

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

void
items_free_all(ClutterTimeline *timeline, ClutterCoverFlowPrivate *priv)
{
    g_sequence_remove_range(
            g_sequence_get_begin_iter(priv->_items),
            g_sequence_get_end_iter(priv->_items));

    priv->n_visible_items = 0;

    priv->iter_visible_front = g_sequence_get_begin_iter(priv->_items);
    priv->iter_visible_start = g_sequence_get_begin_iter(priv->_items);
    priv->iter_visible_end = g_sequence_get_begin_iter(priv->_items);

}

void
zoom_items(ClutterCoverFlowPrivate *priv, float zoom_value)
{
    ClutterAnimation *anim;
    ClutterTimeline *timeline;

    anim = clutter_actor_animate (
            priv->m_container,
            CLUTTER_EASE_OUT_EXPO, 500,
			"scale-x", zoom_value,
			"scale-y", zoom_value,
			"opacity", 0,
			NULL);
    timeline = clutter_animation_get_timeline(anim);

    g_signal_connect (
                timeline, "completed",
                G_CALLBACK (items_free_all), priv);
}

void
knock_down_items(ClutterCoverFlowPrivate *priv)
{
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
                    "opacity", 0,
                    NULL);
        clutter_actor_animate_with_alpha (
                    item->reflection,
                    alpha,
                    "opacity", 0,
                    NULL);
        clutter_timeline_stop(timeline);
        if (iter == priv->iter_visible_end)
            break;
    }

    g_signal_connect (
                timeline, "completed",
                G_CALLBACK (items_free_all), priv);

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
}

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

void
move_end_iters(ClutterCoverFlow *coverflow, move_t dir)
{
    CoverFlowItem *item;
    GSequenceIter *iter;
    ClutterCoverFlowPrivate *priv = coverflow->priv;

    if (dir == MOVE_LEFT) {
        iter = g_sequence_iter_next(priv->iter_visible_end);

        /* Are we at the end ? */
        if ( iter == g_sequence_get_end_iter(priv->_items) )
            return;

        /* Move the end along, and add a new item there */        
        priv->iter_visible_end = iter;
        item = g_sequence_get(priv->iter_visible_end);
        add_item_visible(coverflow, item, MOVE_LEFT);

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
        add_item_visible(coverflow, item, MOVE_RIGHT);

        /* Move the end back, and remove the item that was there */
        item = g_sequence_get(priv->iter_visible_end);
        item_free_visible(item);
        priv->iter_visible_end = g_sequence_iter_prev(priv->iter_visible_end);

        g_debug("MOVE: Moving start and end iters");
        //amtest
        printf("MOVE_RIGHT: start and end iters\n");
    }


}

void
move_iters(ClutterCoverFlow *coverflow, move_t dir, gboolean move_ends)
{
    GSequenceIter *new_front_iter;
    ClutterCoverFlowPrivate *priv = coverflow->priv;

    new_front_iter = move_covers_to_new_positions(coverflow, dir);

    int curr_pos;

    /* Move the start and end iters along one if we are at... */
    if (move_ends) {
        int nitems = g_sequence_get_length(priv->_items);
        //amtest
        curr_pos = g_sequence_iter_get_position(priv->iter_visible_front);
        printf("!! nitems: %d n_visible: %d curr_pos: %d\n", nitems, priv->n_visible_items, curr_pos);
        printf("iter start:%d front:%d end:%d\n",
        g_sequence_iter_get_position(priv->iter_visible_start),
        g_sequence_iter_get_position(priv->iter_visible_front),
        g_sequence_iter_get_position(priv->iter_visible_end));
        
        if (nitems > priv->n_visible_items && curr_pos >= 5 && curr_pos <= nitems-5)
            move_end_iters(coverflow, dir);
    }

    /* Move the front iter along */
    if (new_front_iter && new_front_iter != priv->iter_visible_front)
        priv->iter_visible_front = new_front_iter;
}

/*
 * This functions adds a rotation behaviour from the current angle to the final angle 
 * rotating with the direction <direction> 
 */
void
set_rotation_behaviour (ClutterCoverFlow *self, CoverFlowItem *item, int final_angle, ClutterRotateDirection direction)
{
    double current;

    current = clutter_actor_get_rotation(item->container,CLUTTER_Y_AXIS,0,0,0);
    if(current<0) current+=360;
    if(current>360) current-=360;

    if(current != final_angle) {
        item->rotateBehaviour = clutter_behaviour_rotate_new (
                    self->priv->m_alpha,
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
get_item_scale(CoverFlowItem *item, int dist_from_front, move_t dir)
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

int
get_item_distance(CoverFlowItem *item, int dist_from_front, move_t dir)
{
    int dist = (ABS(dist_from_front) * COVER_SPACE) + FRONT_COVER_SPACE;

    if (dist_from_front == 0)
        return 0;

    return (dist_from_front > 0 ? dist : 0 - dist);
}

int
get_item_opacity(CoverFlowItem *item, int dist_from_front, move_t dir)
{
    return CLAMP(255*(VISIBLE_ITEMS - ABS(dist_from_front))/VISIBLE_ITEMS, 0, 255);
}

int
get_item_reflection_opacity(CoverFlowItem *item, int dist_from_front, move_t dir)
{
    return CLAMP((REFLECTION_ALPHA*(VISIBLE_ITEMS - ABS(dist_from_front))/VISIBLE_ITEMS), 0, REFLECTION_ALPHA);
}

void
animate_item_to_new_position(ClutterCoverFlow *self, CoverFlowItem *item, int dist_from_front, move_t dir)
{
    float scale;
    int dist, angle, opacity, reflection_opacity;
    ClutterRotateDirection rotation_dir = 0;

    scale = get_item_scale(item, dist_from_front, dir);
    dist = get_item_distance(item, dist_from_front, dir);
    opacity = get_item_opacity(item, dist_from_front, dir);
    reflection_opacity = get_item_reflection_opacity(item, dist_from_front, dir);
    get_item_angle_and_dir(item, dist_from_front, dir, &angle, &rotation_dir);

    set_rotation_behaviour(self, item, angle, rotation_dir);

    clutter_actor_animate_with_alpha (item->texture, self->priv->m_alpha,
                                      "shade", opacity,
                                      NULL);
    clutter_actor_animate_with_alpha (item->reflection, self->priv->m_alpha,
                                      "shade", reflection_opacity,
                                      NULL);

    clutter_actor_animate_with_alpha (
                            item->container,
                            self->priv->m_alpha,
                            "scale-x", scale,
                            "scale-y", scale,
                            "scale-center-x" , clutter_actor_get_width  (item->texture)/2,
                            "scale-center-y" , clutter_actor_get_height (item->texture)/2,
                            "x", dist - clutter_actor_get_width(item->container)/2,
                            NULL);
}

void
update_item_text(ClutterCoverFlow *self, CoverFlowItem *item)
{
    ClutterCoverFlowPrivate *priv = self->priv;

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
GSequenceIter *
move_covers_to_new_positions(ClutterCoverFlow *self, move_t dir)
{
    int j;
    CoverFlowItem *item;
    GSequenceIter *iter, *iter_new_front, *iter_new_front_next, *iter_new_front_prev;
    ClutterCoverFlowPrivate *priv = self->priv;

    /* If a one item list then do nothing */
    if (priv->iter_visible_front == priv->iter_visible_start && priv->iter_visible_front == priv->iter_visible_end)
        return NULL;

    /* First take the object on the other (relative to dir) side of the
     * centre and It becomes the new front
     */
    if (dir == MOVE_LEFT) {
        /* Already at the end, the front does not move ? */
        if (priv->iter_visible_front == priv->iter_visible_end) {
            iter_new_front = priv->iter_visible_front;
            iter_new_front_next = NULL;
            iter_new_front_prev = g_sequence_iter_prev (priv->iter_visible_front);
            g_debug("MOVE: Front at end");
        } else {
            iter_new_front = g_sequence_iter_next(priv->iter_visible_front);
            /* Now at the end  ? */
            if ( iter_new_front == priv->iter_visible_end ) {
                iter_new_front_next = NULL;
                iter_new_front_prev = g_sequence_iter_prev (priv->iter_visible_end);
                g_debug("MOVE: New front at end");
            } else {
                iter_new_front_next = g_sequence_iter_next(iter_new_front);
                iter_new_front_prev = g_sequence_iter_prev(iter_new_front);
                g_debug("MOVE: Front <-- left --|");
            }
        }
    } else if (dir == MOVE_RIGHT) {
        /* Already at the start, the front does not move ? */
        if (priv->iter_visible_front == priv->iter_visible_start) {
            iter_new_front = priv->iter_visible_front;
            iter_new_front_next = g_sequence_iter_next (priv->iter_visible_front);
            iter_new_front_prev = NULL;
            g_debug("MOVE: Front at start");
        } else {
            iter_new_front = g_sequence_iter_prev(priv->iter_visible_front);
            /* Now at the start ? */
            if ( iter_new_front == priv->iter_visible_start ) {
                iter_new_front_next = g_sequence_iter_next (priv->iter_visible_start);
                iter_new_front_prev = NULL;
                g_debug("MOVE: New front at start");
            } else {
                iter_new_front_next = g_sequence_iter_next(iter_new_front);
                iter_new_front_prev = g_sequence_iter_prev(iter_new_front);
                g_debug("MOVE: Front |-- right -->");
            }
        }
    }
    else {
        g_critical("Unknown move");
        return NULL;
    }

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
    g_sequence_foreach_range(
        self->priv->iter_visible_start,
        self->priv->iter_visible_end,
        (GFunc)item_clear_behavior,
        NULL);
}

void
fade_in(ClutterCoverFlow *self, CoverFlowItem *item, guint distance_from_centre)
{
    ClutterTimeline *timeline = clutter_timeline_new(FRAMES * FPS);
    ClutterAlpha    *alpha = clutter_alpha_new_full(timeline,CLUTTER_EASE_OUT_EXPO);

    int opacity = get_item_opacity(item, distance_from_centre, DONT_MOVE);
    int reflection_opacity 	= get_item_reflection_opacity(item, distance_from_centre, DONT_MOVE);

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

/* Takes the given item, gets the visual resources to represent it,
 * and makes it visible in the stack
 */
void
add_item_visible(ClutterCoverFlow *self, CoverFlowItem *item, move_t dir)
{
    int bps;
    float scale;
    int dist, angle, opacity;
    GdkPixbuf *pb;
    ClutterCoverFlowPrivate *priv;
    ClutterRotateDirection rotation_dir;

    g_return_if_fail(item != NULL);
    g_return_if_fail(item->file != NULL);
    g_return_if_fail(item->get_info_callback != NULL);

    priv = self->priv;
    g_return_if_fail(priv->n_visible_items <= NLOAD_ITEMS);

    g_debug("ADDING");

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

    /* Calculate the position for the new item. */
    scale = get_item_scale(item, priv->n_visible_items, dir);
    dist = get_item_distance(item, priv->n_visible_items, dir);
    opacity = get_item_opacity(item, priv->n_visible_items, dir);
    get_item_angle_and_dir(item, priv->n_visible_items, dir, &angle, &rotation_dir);

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

    //amtest
    printf("!!ADDING pos: %f %d\n", dist - clutter_actor_get_width(item->texture)/2,
           priv->n_visible_items);
    /* But animate the fade in */
    fade_in (self, item, priv->n_visible_items);

    /* Update the text. For > 1 items it is done when we animate
     * the new front into view */
    if(priv->n_visible_items == 0) {
        update_item_text(self, item);
    }
        /* Reset items to default settings incase they were previously animated
         * away */
/*        clutter_actor_set_opacity (
                    priv->item_name,
                    255);
        clutter_actor_set_opacity (
                    priv->item_type,
                    255);
        clutter_actor_set_opacity (
                    priv->m_container,
                    255);
        clutter_actor_set_scale (
                    priv->m_container,
                    1.0, 1.0);
    }
*/
    /* New items always go on the right, i.e. at the back too */
    clutter_actor_lower_bottom (item->container);
}

void
add_file_internal(ClutterCoverFlow *self, GFile *file, ClutterCoverFlowGetInfoCallback cb)
{
    GSequenceIter *iter;
    CoverFlowItem *item;
    ClutterCoverFlowPrivate *priv = self->priv;

    /* Create the new item */
    item = g_new0 (CoverFlowItem, 1);
    item->get_info_callback = cb;
    item->file = file;
    g_object_ref(file);

    /* Add it to the list, and the map of uri->iter */
    iter = g_sequence_append(priv->_items, item);

    g_hash_table_insert(
        priv->uri_to_item_map,
        g_file_get_uri(file),   /* freed by hashtable KeyDestroyFunc */
        iter);

    if (priv->n_visible_items < NLOAD_ITEMS) {
        //amtest
        printf("!! WEIRD n_vis < VISIBLE %d\n", priv->n_visible_items);
        /* We use MOVE_LEFT as new items all being placed on the right */
        add_item_visible(self, item, MOVE_LEFT);

        if (priv->n_visible_items == 0) {
            priv->iter_visible_front = iter;
            priv->iter_visible_start = iter;
        }

        /* Added on the right, the last one visible */
        priv->iter_visible_end = iter;
        priv->n_visible_items += 1;
    }

}


