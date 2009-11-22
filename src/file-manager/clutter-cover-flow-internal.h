/* clutter-cover-flow.h */

#ifndef _CLUTTER_COVER_FLOW_INTERNAL_
#define _CLUTTER_COVER_FLOW_INTERNAL_

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <clutter/clutter.h>

#include "clutter-cover-flow.h"
#include "clutter-black-texture.h"

#define VISIBLE_ITEMS       11
#define FRAMES              40
#define FPS                 40
#define MAX_ANGLE           70
#define COVER_SPACE         50
#define FRONT_COVER_SPACE   200
#define MAX_SCALE           1.8
#define REFLECTION_ALPHA	60			/* 0 = no reflection, 255 = total reflection*/
#define MAX_ITEM_HEIGHT     240
#define TEXT_PAD_BELOW_ITEM 50
#define DEFAULT_ICON_SIZE   200
#define VERTICAL_OFFSET     110

typedef struct _CoverflowItem
{
    GFile                           *file;
    ClutterCoverFlowGetInfoCallback get_info_callback;
    char                            *display_name;
    char                            *display_type;
    ClutterActor                    *container;
    ClutterActor                    *texture;
    ClutterActor                    *reflection;
    ClutterBehaviour                *rotateBehaviour;
} CoverFlowItem;

typedef enum
{
    MOVE_LEFT = -1,
    MOVE_RIGHT = 1,
    DONT_MOVE = 0
} move_t;

typedef enum
{
    COVERFLOW_MODE,
    GRID_MODE,
    FILM_MODE
} viewmode_t;

struct _ClutterCoverFlowPrivate {
    GSequence                   *_items;
    GHashTable                  *uri_to_item_map;

    GSequenceIter               *iter_visible_front;
    GSequenceIter               *iter_visible_start;
    GSequenceIter               *iter_visible_end;

    int                         n_visible_items;
    //int                         watermark;

    ClutterActor                *m_stage;                   //stage (Window)
    ClutterActor                *item_name;                 //Text to display
    ClutterActor                *item_type;                 //Text to display
    ClutterAlpha                *m_alpha;                   //Alpha function
    ClutterTimeline             *m_timeline;                //Timeline (Values in defines.h)
    ClutterActor                *m_container;

    viewmode_t                  view_mode;
};

void item_clear_behavior (CoverFlowItem *item, gpointer user_data);
void item_free_visible(CoverFlowItem *item);
void item_free_invisible(CoverFlowItem *item);
void items_free_all(ClutterTimeline *timeline, ClutterCoverFlowPrivate *priv);

void zoom_items(ClutterCoverFlowPrivate *priv, float zoom_value);
void knock_down_items(ClutterCoverFlowPrivate *priv);
GSequenceIter *get_actor_iter(ClutterCoverFlowPrivate *priv, ClutterActor * actor);
void move_end_iters(ClutterCoverFlow *coverflow, move_t dir);
void move_iters(ClutterCoverFlow *coverflow, move_t dir, gboolean move_ends);
void add_item_visible(ClutterCoverFlow *self, CoverFlowItem *item, move_t dir);
void get_info(GFile *file, char **name, char **description, GdkPixbuf **pb, guint pbsize);
void scale_to_fit(ClutterActor *actor);
void fade_in(ClutterCoverFlow *self, CoverFlowItem *item, guint distance_from_centre);
void start(ClutterCoverFlow *self);
void stop(ClutterCoverFlow *self);
void clear_behaviours (ClutterCoverFlow *self);
GSequenceIter *move_covers_to_new_positions(ClutterCoverFlow *self, move_t dir);
void update_item_text(ClutterCoverFlow *self, CoverFlowItem *item);
gfloat get_item_distance(CoverFlowItem *item, int dist_from_front, move_t dir);
int get_item_opacity(CoverFlowItem *item, int dist_from_front, move_t dir);
int get_item_reflection_opacity(CoverFlowItem *item, int dist_from_front, move_t dir);
void animate_item_to_new_position(ClutterCoverFlow *self, CoverFlowItem *item, int dist_from_front, move_t dir);
void set_rotation_behaviour (ClutterCoverFlow *self, CoverFlowItem *item, int final_angle, ClutterRotateDirection direction);
float get_item_scale(CoverFlowItem *item, int dist_from_front, move_t dir);
void get_item_angle_and_dir(CoverFlowItem *item, int dist_from_front, move_t dir, int *angle, ClutterRotateDirection *rotation_dir);
void add_file_internal(ClutterCoverFlow *self, GFile *file, ClutterCoverFlowGetInfoCallback cb);


#endif /* _CLUTTER_COVER_FLOW_INTERNAL_ */
