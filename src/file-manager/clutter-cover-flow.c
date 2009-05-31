/* clutter-cover-flow.c */

#include <glib.h>
#include "clutter-cover-flow.h"
#include "clutter-black-texture.h"

G_DEFINE_TYPE (ClutterCoverFlow, clutter_cover_flow, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLUTTER_TYPE_COVER_FLOW, ClutterCoverFlowPrivate))

#define VISIBLE_ITEMS		10
#define FRAMES				40
#define FPS					40
#define MAX_ANGLE			70
#define COVER_SPACE			50
#define FRONT_COVER_SPACE 	200
#define MAX_SCALE			1.7
#define MAX_ITEM_HEIGHT		240
#define TEXT_PAD_BELOW_ITEM 50
#define DEFAULT_ICON_SIZE   200
#define VERTICAL_OFFSET     110
#define WATERMARK           3

typedef struct _CoverflowItem
{
    GFile                           *file;
    ClutterCoverFlowGetInfoCallback get_info_callback;
	char				            *display_name;
	char				            *display_type;
	ClutterActor		            *container;
	ClutterActor		            *texture;
	ClutterActor		            *reflection;
	ClutterBehaviour	            *rotateBehaviour;
} CoverFlowItem;

typedef enum
{
    MOVE_LEFT = -1,
    MOVE_RIGHT = 1
} move_t;

struct _ClutterCoverFlowPrivate {
    GSequence                   *_items;
    GHashTable                  *uri_to_item_map;

    GSequenceIter               *iter_visible_front;
    GSequenceIter               *iter_visible_start;
    GSequenceIter               *iter_visible_end;

    int                         nitems;
    int                         n_visible_items;
    int                         watermark;

    ClutterActor 				*m_stage;					//stage (Window)
    ClutterActor				*item_name;					//Text to display
    ClutterActor				*item_type;					//Text to display
    int         				m_nextItem;					//Next item to be in front
    ClutterAlpha 				*m_alpha;					//Alpha function
    ClutterTimeline 			*m_timeline;				//Timeline (Values in defines.h)
    int 						m_middle_x;
    int 						m_middle_y;
    ClutterActor				*m_container;
    int   						m_loaded;					//Pixbuf Loadeds
};

void fade_in(ClutterCoverFlow *coverflow, CoverFlowItem *item, guint distance_from_centre);
static void scale_to_fit(ClutterActor *actor);
void set_rotation_behaviour (ClutterCoverFlow *self, CoverFlowItem *item, int final_angle, ClutterRotateDirection direction);
static void get_info(GFile *file, char **name, char **description, GdkPixbuf **pb, guint pbsize);
void stop(ClutterCoverFlow *self);
void clear_behaviours (ClutterCoverFlow *self);

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
free_item(CoverFlowItem *item)
{
    ;
}

static void
clutter_cover_flow_init (ClutterCoverFlow *self)
{
  self->priv  = g_new0 (ClutterCoverFlowPrivate, 1);

    self->priv->m_timeline = clutter_timeline_new(FRAMES, FPS);
    self->priv->m_alpha = clutter_alpha_new_full(self->priv->m_timeline,CLUTTER_EASE_OUT_EXPO);
    self->priv->_items = g_sequence_new((GDestroyNotify)free_item);

    /* Maps uris to iters in the GSequence. The GSequence cleans up the iters,
     * we must free the keys */
    self->priv->uri_to_item_map = g_hash_table_new_full(
                                    g_str_hash,
                                    g_str_equal,
                                    g_free, /* KeyDestroyFunc, keys are uri strings */
                                    NULL);
}

static gboolean
on_stage_resized(ClutterStage *stage, ClutterButtonEvent *event, gpointer user_data)
{
    ClutterCoverFlow *self = CLUTTER_COVER_FLOW(user_data); 
    guint h = clutter_actor_get_height(CLUTTER_ACTOR(stage));
    guint w = clutter_actor_get_width(CLUTTER_ACTOR(stage));
    float relation = (float)500/(float)h;
    
    self->priv->m_middle_y = h/2;
    self->priv->m_middle_x = w/2;
    clutter_actor_set_position( 
    					self->priv->m_container, 
    					self->priv->m_middle_x,
    					self->priv->m_middle_y);
    					
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

/*
 * This functions adds a rotation behaviour from the current angle to the final angle 
 * rotating with the direction <direction> 
 */
void set_rotation_behaviour (ClutterCoverFlow *self, CoverFlowItem *item, int final_angle, ClutterRotateDirection direction)
{
	double current;

    current = clutter_actor_get_rotation(item->container,CLUTTER_Y_AXIS,0,0,0);
	if(current<0) 	current+=360;
	if(current>360) current-=360;

	if(current != final_angle)
	{
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

static float
get_item_scale(CoverFlowItem *item, int dist_from_front, move_t dir)
{
    static float ITEM_SCALES[VISIBLE_ITEMS] = 
        {1.7,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
    return ITEM_SCALES[ABS(dist_from_front)];
}

static void
get_item_angle_and_dir(CoverFlowItem *item, int dist_from_front, move_t dir, int *angle, ClutterRotateDirection *rotation_dir)
{
    static int  ITEM_ANGLES[VISIBLE_ITEMS] = 
        {0,70,70,70,70,70,70,70,70,70};
    int idx;

    /* The front item direction depends on the direction we came from */
    if (dist_from_front == 0) {
        *rotation_dir =  (dir == MOVE_RIGHT ? CLUTTER_ROTATE_CCW : CLUTTER_ROTATE_CW);
        *angle = 0;
    }

    idx = ABS(dist_from_front);

    /* Item on the right */
    if (dist_from_front > 0) {
        *rotation_dir = CLUTTER_ROTATE_CCW;
        *angle = 360 - ITEM_ANGLES[idx];
    }

    /* Item on the left */
    if (dist_from_front < 0) {
        *rotation_dir = CLUTTER_ROTATE_CW;
        *angle = ITEM_ANGLES[idx];
    }
}

static int
get_item_distance(CoverFlowItem *item, int dist_from_front, move_t dir)
{
    int dist = (ABS(dist_from_front) * COVER_SPACE) + FRONT_COVER_SPACE;

    if (dist_from_front == 0)
        return 0;

    return (dist_from_front > 0 ? dist : 0 - dist);
}

static int
get_item_opacity(CoverFlowItem *item, int dist_from_front, move_t dir)
{
    return CLAMP(255*(VISIBLE_ITEMS - ABS(dist_from_front))/VISIBLE_ITEMS, 0, 255);
}

static void
animate_item_to_new_position(ClutterCoverFlow *self, CoverFlowItem *item, int dist_from_front, move_t dir)
{
    ClutterRotateDirection rotation_dir;
    float scale;
    int dist, angle, opacity;

    scale = get_item_scale(item, dist_from_front, dir);
    dist = get_item_distance(item, dist_from_front, dir);
    opacity = get_item_opacity(item, dist_from_front, dir);
    get_item_angle_and_dir(item, dist_from_front, dir, &angle, &rotation_dir);

    set_rotation_behaviour(self, item, angle, rotation_dir);

    clutter_actor_animate_with_alpha (
                            item->texture,
                            self->priv->m_alpha,
                            "shade", opacity,
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

static void
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
static void
move_covers_to_new_positions(ClutterCoverFlow *self, move_t dir)
{
    int j;
    CoverFlowItem *item;
    GSequenceIter *iter, *iter_new_front, *old_front, *iter_new_front_next, *iter_new_front_prev;
    ClutterCoverFlowPrivate *priv = self->priv;

    //begin = g_sequence_get_begin_iter(priv->_items);
    //end = g_sequence_get_end_iter(priv->_items);
    old_front = priv->iter_visible_front;

    /* If a one item list then do nothing */
    if (priv->iter_visible_front == priv->iter_visible_start && priv->iter_visible_front == priv->iter_visible_end)
        return;

    /* First take the object on the other (relative to dir) side of the
     * centre and It becomes the new front
     */
    if (dir == MOVE_LEFT) {
        /* Already at the end */
        if (priv->iter_visible_front == priv->iter_visible_end) {
            iter_new_front = priv->iter_visible_front;
            iter_new_front_next = priv->iter_visible_front;
            iter_new_front_prev = g_sequence_iter_prev (priv->iter_visible_front);
        } else {
            iter_new_front = g_sequence_iter_next(priv->iter_visible_front);
            /* Now at the end */
            if ( iter_new_front == priv->iter_visible_end ) {
                iter_new_front_next = priv->iter_visible_end;
                iter_new_front_prev = g_sequence_iter_prev (priv->iter_visible_end);
            } else {
                iter_new_front_next = g_sequence_iter_next(iter_new_front);
                iter_new_front_prev = g_sequence_iter_prev(iter_new_front);
            }
        }
    } 
#if 0
else if (dir == MOVE_RIGHT) {
        /* Already at the start */
        if (priv->iter_visible_front == priv->iter_visible_start) {
            iter_new_front = priv->iter_visible_front;
            iter_new_front_next = g_sequence_iter_next (priv->iter_visible_front);
            iter_new_front_prev = priv->iter_visible_front;
        } else {
            iter_new_front = g_sequence_iter_next(priv->iter_visible_front);
            /* Now at the end */
            if ( iter_new_front == priv->iter_visible_end ) {
                iter_new_front_next = priv->iter_visible_end;
                iter_new_front_prev = g_sequence_iter_prev (priv->iter_visible_end);
            } else {
                iter_new_front_next = g_sequence_iter_next(iter_new_front);
                iter_new_front_prev = g_sequence_iter_prev(iter_new_front);
            }
        }
    }
#endif
    else
        g_critical("Unknown move");

//    if (dir == MOVE_LEFT)
//        iter_new_front = g_sequence_iter_next(priv->iter_visible_front);
//    else if (dir == MOVE_RIGHT)
//        iter_new_front = g_sequence_iter_prev(priv->iter_visible_front);
//    else
//        g_critical("Unknown move");


    item = g_sequence_get(iter_new_front);

    /* Move the new front item into place */
    animate_item_to_new_position(self, item, 0, dir);

    /* Update the text */
    update_item_text (self, item);

     /* Move, scale and rotate all the elements on the left of the new center */
    for (   iter = g_sequence_iter_prev(iter_new_front), j = -1; 
            TRUE;
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
    for (   iter = g_sequence_iter_next(iter_new_front), j = 1;
            TRUE;
            iter = g_sequence_iter_next(iter), j += 1)
	{
        item = g_sequence_get(iter);
        animate_item_to_new_position(self, item, j, dir);

        if(clutter_actor_get_depth(item->container) <= 0)
            clutter_actor_lower_bottom(item->container);

        if (iter == priv->iter_visible_end)
            break;
    }
}

static void
start(ClutterCoverFlow *self, move_t direction)
{
	clutter_timeline_start(self->priv->m_timeline);

    /* If we moved left, the new front is to the right (-ve) */
    self->priv->iter_visible_front = g_sequence_iter_move(
                self->priv->iter_visible_front,
                0 - direction);
}

void stop(ClutterCoverFlow *self)
{
	clutter_timeline_stop(self->priv->m_timeline);
}

static void
clear_item_behavior (CoverFlowItem *item, gpointer user_data)
{
    g_return_if_fail(item != NULL);

    if (    item->rotateBehaviour && 
            CLUTTER_IS_BEHAVIOUR(item->rotateBehaviour) && 
            clutter_behaviour_is_applied(item->rotateBehaviour, item->container) )
	    {	
		    clutter_behaviour_remove(item->rotateBehaviour,item->container);
	    }
}

void clear_behaviours (ClutterCoverFlow *self)
{
    g_sequence_foreach_range(
        self->priv->iter_visible_start,
        self->priv->iter_visible_end,
        (GFunc)clear_item_behavior,
        NULL);
}

void fade_in(ClutterCoverFlow *self, CoverFlowItem *item, guint distance_from_centre)
{
    int opacity;
	ClutterTimeline *timeline;
	ClutterAlpha *alpha;
    ClutterActor *container;

    container = item->container;
	timeline = clutter_timeline_new(FRAMES, FPS);
	alpha = clutter_alpha_new_full(timeline,CLUTTER_EASE_OUT_EXPO);

    opacity = CLAMP((255*(VISIBLE_ITEMS - distance_from_centre)/VISIBLE_ITEMS), 0, 255);
    clutter_actor_animate_with_alpha (
                        item->texture,
                        alpha ,
                        "shade",    opacity,
                        NULL);

    clutter_timeline_start (timeline);
    return;
}

static void
scale_to_fit(ClutterActor *actor)
{
 	int w =	clutter_actor_get_width(actor);
 	int h = clutter_actor_get_height(actor);
 	
 	if( h > MAX_ITEM_HEIGHT)
 	{
 		int temp = w*MAX_ITEM_HEIGHT/h;
 		clutter_actor_set_size(actor, temp, MAX_ITEM_HEIGHT);
 	}

}

static void
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
    *description = g_strdup(
                    g_content_type_get_description(
                        g_file_info_get_attribute_string(
                            file_info,
                            G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE)));
}

static void
remove_item_visible(ClutterCoverFlow *self, CoverFlowItem *item)
{
    ClutterCoverFlowPrivate *priv = self->priv;

    //FIXME: Free actor resources
    clutter_container_remove_actor	(
                CLUTTER_CONTAINER(priv->m_container),
                item->container);
}

/* Takes the given item, gets the visual resources to represent it,
 * and makes it visible in the stack
 */
static void
add_item_visible(ClutterCoverFlow *self, CoverFlowItem *item)
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
    g_return_if_fail(priv->n_visible_items <= VISIBLE_ITEMS);

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
                gdk_pixbuf_get_pixels		(pb),
                gdk_pixbuf_get_has_alpha	(pb),
                gdk_pixbuf_get_width		(pb),
                gdk_pixbuf_get_height		(pb),
                gdk_pixbuf_get_rowstride	(pb),
                bps,
                (ClutterTextureFlags)0,
                NULL);

	scale_to_fit (item->texture);
	clutter_actor_set_position(item->texture, 0, 0);

  	/* Reflection */
  	item->reflection = clutter_clone_new ( item->texture );
  	clutter_actor_set_opacity ( item->reflection, 60);
	scale_to_fit ( item->reflection );
  	clutter_actor_set_position ( item->reflection, 0, clutter_actor_get_height(item->reflection) );
	clutter_actor_set_rotation	(
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
	item->container	= clutter_group_new();
	clutter_group_add_many	(
                CLUTTER_GROUP(item->container),
                item->texture, item->reflection, NULL );
	clutter_container_add_actor	(
                CLUTTER_CONTAINER(priv->m_container),
                item->container);

    /* Calculate the position for the new item.
     * We use MOVE_LEFT as this is equiv to new items all being placed
     * on the right
     */
    scale = get_item_scale(item, priv->n_visible_items, MOVE_LEFT);
    dist = get_item_distance(item, priv->n_visible_items, MOVE_LEFT);
    opacity = get_item_opacity(item, priv->n_visible_items, MOVE_LEFT);
    get_item_angle_and_dir(item, priv->n_visible_items, MOVE_LEFT, &angle, &rotation_dir);

    /* Dont animate the item position, just put it there position */
	clutter_actor_set_rotation	(
            item->container,
            CLUTTER_Y_AXIS, angle,
            clutter_actor_get_width(item->texture)/2,
            0,0);
	clutter_actor_set_scale_full (
            item->container,
            scale, scale,
            clutter_actor_get_width(item->texture)/2,
            clutter_actor_get_height(item->texture)/2);
	clutter_actor_set_position 	( 
            item->container, 
            dist - clutter_actor_get_width(item->texture)/2, 
            VERTICAL_OFFSET - clutter_actor_get_height(item->texture));

    /* But animate the fade in */
    fade_in	(self, item, priv->n_visible_items);

    /* Update the text. For > 1 items it is done when we animate
     * the new front into view */
	if(priv->n_visible_items == 0)
        update_item_text(self, item);
	
    /* New items always go on the right, i.e. at the back too */
    clutter_actor_lower_bottom (item->container);
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

static void
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

    if (priv->n_visible_items < VISIBLE_ITEMS) {
        add_item_visible(self, item);

        if (priv->n_visible_items == 0) {
            priv->iter_visible_front = iter;
            priv->iter_visible_start = iter;
        }

        /* Added on the right, the last one visible */
        priv->iter_visible_end = iter;
        priv->n_visible_items += 1;
    }

    priv->nitems += 1;
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

	stop(coverflow);
	clear_behaviours(coverflow);
 	move_covers_to_new_positions(coverflow, MOVE_LEFT);

    /* Move the start and end iters along one if we are at... */
    if (priv->watermark == WATERMARK && priv->nitems > priv->n_visible_items) {
        CoverFlowItem *item;

        /* Load data into the new end one */
        priv->iter_visible_end = g_sequence_iter_next(priv->iter_visible_end);
        item = g_sequence_get(priv->iter_visible_end);
        add_item_visible(coverflow, item);

        /* Remove the old iter */
        item = g_sequence_get(priv->iter_visible_start);
        remove_item_visible(coverflow, item);
        priv->iter_visible_start = g_sequence_iter_next(priv->iter_visible_start);
    }

 	start(coverflow, MOVE_LEFT);

    priv->watermark = CLAMP(priv->watermark + 1, -WATERMARK, WATERMARK);
    g_debug("Moving left (wm: %d)", priv->watermark);
}

void clutter_cover_flow_right(ClutterCoverFlow *coverflow)
{
    ClutterCoverFlowPrivate *priv = coverflow->priv;

	stop(coverflow);
	clear_behaviours(coverflow);
 	move_covers_to_new_positions(coverflow, MOVE_RIGHT);

    /* Move the start and end iters along one if we are at... */
    if (priv->watermark == WATERMARK && priv->nitems > priv->n_visible_items) {
        CoverFlowItem *item;

        /* Load data into the new end one */
        priv->iter_visible_end = g_sequence_iter_next(priv->iter_visible_end);
        item = g_sequence_get(priv->iter_visible_end);
        add_item_visible(coverflow, item);

        /* Remove the old iter */
        item = g_sequence_get(priv->iter_visible_start);
        remove_item_visible(coverflow, item);
        priv->iter_visible_start = g_sequence_iter_next(priv->iter_visible_start);
    }

 	start(coverflow, MOVE_RIGHT); 

    priv->watermark = CLAMP(priv->watermark - 1, -WATERMARK, WATERMARK);
    g_debug("Moving right (wm: %d)", priv->watermark);
}

