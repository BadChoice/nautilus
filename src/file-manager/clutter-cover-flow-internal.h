/* clutter-cover-flow.h */

#ifndef _CLUTTER_COVER_FLOW_INTERNAL_
#define _CLUTTER_COVER_FLOW_INTERNAL_

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <clutter/clutter.h>
#include <libnautilus-private/nautilus-file.h>
#include <string.h>

#include "clutter-cover-flow.h"
#include "clutter-black-texture.h"

#define VISIBLE_ITEMS       15
#define V_ITEMS             21          /* number of items to hilight (opacity > 0) */
#define FRAMES              40
#define FPS                 40
#define KEY_SENSIBILITY     40          /* delay in ms between 2 input */
#define MAX_ANGLE           70
#define COVER_SPACE         30
#define FRONT_COVER_SPACE   170
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
    GtkTreeIter                     *iter;
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
    GHashTable                  *uri_to_item_map;
    GHashTable                  *iter_added;
    GtkTreeModel                *model;
    int                         model_is_list_store;
    gint                        file_column;

    ClutterCoverFlowGetInfoCallback get_info_callback;
    GQuark                      info_quark;
    GQuark                      item_quark;

    guint                       m_item_count;
    CoverFlowItem               **visible_items;
    CoverFlowItem               **onstage_items;

    int                         idx_visible_front;

    ClutterActor                *m_stage;                   //stage (Window)
    ClutterActor                *item_name;                 //Text to display
    ClutterActor                *item_type;                 //Text to display
    ClutterAlpha                *m_alpha;                   //Alpha function
    ClutterTimeline             *m_timeline;                //Timeline (Values in defines.h)
    ClutterActor                *m_container;

    viewmode_t                  view_mode;
};

CoverFlowItem *item_new(ClutterCoverFlowPrivate *priv, GtkTreeIter *iter);
CoverFlowItem *item_grab_index(ClutterCoverFlowPrivate *priv, int idx);
void items_update(ClutterCoverFlowPrivate *priv);
void item_clear_behavior (CoverFlowItem *item, gpointer user_data);
void item_free_visible(CoverFlowItem *item);
void item_free_invisible(CoverFlowItem *item);

void reset(ClutterCoverFlowPrivate *priv);
void zoom_items(ClutterCoverFlowPrivate *priv, float zoom_value, gboolean clear_when_complete);
void knock_down_items(ClutterCoverFlowPrivate *priv, gboolean clear_when_complete);

int         model_get_length(ClutterCoverFlowPrivate *priv);
int         model_get_position(ClutterCoverFlowPrivate *priv, GtkTreeIter *iter);
gboolean    model_is_empty(ClutterCoverFlowPrivate *priv);
GFile*      model_get_front_file(ClutterCoverFlowPrivate *priv);
void        model_row_inserted(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ClutterCoverFlowPrivate *priv);
void        model_row_changed(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, ClutterCoverFlowPrivate *priv);
void        model_rows_reordered(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gint *new_order, ClutterCoverFlowPrivate *priv);
void        model_row_deleted(GtkTreeModel *model, GtkTreePath *path, ClutterCoverFlowPrivate *priv);
void        model_add_file(ClutterCoverFlowPrivate *priv, GFile *file, ClutterCoverFlowGetInfoCallback cb);

void        view_add_item(ClutterCoverFlowPrivate *priv, CoverFlowItem *item, move_t dir);
void        view_move(ClutterCoverFlowPrivate *priv, move_t dir, gboolean move_ends);

//GSequenceIter *get_actor_iter(ClutterCoverFlowPrivate *priv, ClutterActor * actor);
int get_actor_pos(ClutterCoverFlowPrivate *priv, ClutterActor * actor);
void get_info(GFile *file, char **name, char **description, GdkPixbuf **pb, char **pbpath, guint pbsize);
void fade_in(ClutterCoverFlowPrivate *priv, CoverFlowItem *item, guint distance_from_centre);
void start(ClutterCoverFlow *self);
void stop(ClutterCoverFlow *self);
void clear_behaviours (ClutterCoverFlow *self);
void update_item_text(ClutterCoverFlowPrivate *priv, CoverFlowItem *item);
gfloat get_item_distance(CoverFlowItem *item, int dist_from_front);
int get_item_opacity(CoverFlowItem *item, int dist_from_front);
int get_item_reflection_opacity(CoverFlowItem *item, int dist_from_front);
void animate_item_to_new_position(ClutterCoverFlowPrivate *priv, CoverFlowItem *item, int dist_from_front, move_t dir);
void view_restack(ClutterCoverFlowPrivate *priv);
void set_rotation_behaviour (ClutterCoverFlowPrivate *priv, CoverFlowItem *item, int final_angle, ClutterRotateDirection direction);
float get_item_scale(CoverFlowItem *item, int dist_from_front);
void get_item_angle_and_dir(CoverFlowItem *item, int dist_from_front, move_t dir, int *angle, ClutterRotateDirection *rotation_dir);

#endif /* _CLUTTER_COVER_FLOW_INTERNAL_ */
