/* clutter-cover-flow.h */

#ifndef _CLUTTER_COVER_FLOW
#define _CLUTTER_COVER_FLOW

#include <gtk/gtk.h>
#include <gio/gio.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

#define CLUTTER_TYPE_COVER_FLOW clutter_cover_flow_get_type()

#define CLUTTER_COVER_FLOW(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLUTTER_TYPE_COVER_FLOW, ClutterCoverFlow))

#define CLUTTER_COVER_FLOW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), CLUTTER_TYPE_COVER_FLOW, ClutterCoverFlowClass))

#define CLUTTER_IS_COVER_FLOW(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLUTTER_TYPE_COVER_FLOW))

#define CLUTTER_IS_COVER_FLOW_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), CLUTTER_TYPE_COVER_FLOW))

#define CLUTTER_COVER_FLOW_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), CLUTTER_TYPE_COVER_FLOW, ClutterCoverFlowClass))

typedef struct _ClutterCoverFlowPrivate ClutterCoverFlowPrivate;

typedef struct {
    ClutterGroup parent;
    ClutterCoverFlowPrivate *priv;
} ClutterCoverFlow;

typedef struct {
    ClutterGroupClass parent_class;
} ClutterCoverFlowClass;

typedef void (*ClutterCoverFlowGetInfoCallback)(GFile *file, char **name, char **description, GdkPixbuf **pb, guint pbsize);

GType clutter_cover_flow_get_type (void);

ClutterCoverFlow* clutter_cover_flow_new (ClutterActor *stage);

ClutterCoverFlow* clutter_cover_flow_new_with_model (ClutterActor *stage, GtkTreeModel *store, GtkTreeView *tree, int file_column);

void clutter_cover_flow_set_model(ClutterCoverFlow *self, GtkTreeModel *model, int file_column);

GtkTreeModel *clutter_cover_flow_get_model(ClutterCoverFlow *self, int *file_column);

void clutter_cover_flow_set_info_callback(ClutterCoverFlow *self, ClutterCoverFlowGetInfoCallback cb);

void clutter_cover_flow_add_gfile(ClutterCoverFlow *coverflow, GFile *file);

void clutter_cover_flow_add_gfile_with_info_callback(ClutterCoverFlow *coverflow, GFile *file, ClutterCoverFlowGetInfoCallback cb);

gboolean clutter_cover_flow_isplaying(ClutterCoverFlow *coverflow);

void clutter_cover_flow_left(ClutterCoverFlow *coverflow);

void clutter_cover_flow_right(ClutterCoverFlow *coverflow);

ClutterActor* clutter_cover_flow_get_actor_at_pos(ClutterCoverFlow *coverflow, guint x, guint y);

void clutter_cover_flow_scroll_to_actor(ClutterCoverFlow *coverflow, ClutterActor * actor);

void clutter_cover_flow_scroll_to_position(ClutterCoverFlow *coverflow, int pos);

GFile *clutter_cover_flow_get_gfile_at_front(ClutterCoverFlow *coverflow);

void clutter_cover_flow_clear(ClutterCoverFlow *coverflow);

void clutter_cover_flow_select(ClutterCoverFlow *coverflow, gboolean should_clear);

void clutter_cover_flow_default_view(ClutterCoverFlow *coverflow);

void clutter_cover_flow_grid_view(ClutterCoverFlow *coverflow);

void clutter_cover_flow_film_view(ClutterCoverFlow *coverflow);

int clutter_cover_flow_get_front_index(ClutterCoverFlow *coverflow);

G_END_DECLS

#endif /* _CLUTTER_COVER_FLOW */

