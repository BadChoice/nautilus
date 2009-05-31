#include "clutter-black-texture.h"

typedef struct _BlackTextureClass 	BlackTextureClass;

struct _BlackTextureClass
{
  ClutterTextureClass parent_class;
};

struct _BlackTexture
{
  ClutterTexture parent;

  guint shade;
};

enum
{
  PROP_0,

  PROP_SHADE
};

G_DEFINE_TYPE (BlackTexture, black_texture, CLUTTER_TYPE_TEXTURE);

static void
black_texture_paint (ClutterActor *actor)
{
  BlackTexture *self = TEST_BLACK_TEXTURE (actor);
  CoglHandle material;
  gint x_1, y_1, x_2, y_2;

  clutter_actor_get_allocation_coords (actor, &x_1, &y_1, &x_2, &y_2);

  material = clutter_texture_get_cogl_material (CLUTTER_TEXTURE (self));

  cogl_material_set_color4ub (material, self->shade, self->shade, self->shade,
                              clutter_actor_get_paint_opacity (actor));
  cogl_set_source (material);
  cogl_rectangle (0, 0, (x_2 - x_1), (y_2 - y_1));
}

static void
black_texture_get_property (GObject *object, guint property_id,
                                 GValue *value, GParamSpec *pspec)
{
  BlackTexture *self = TEST_BLACK_TEXTURE (object);

  switch (property_id)
    {
    case PROP_SHADE:
      g_value_set_uint (value, self->shade);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
black_texture_set_property (GObject *object, guint property_id,
                                 const GValue *value, GParamSpec *pspec)
{
  BlackTexture *self = TEST_BLACK_TEXTURE (object);

  switch (property_id)
    {
    case PROP_SHADE:
      self->shade = g_value_get_uint (value);
      clutter_actor_queue_redraw (CLUTTER_ACTOR (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
black_texture_class_init (BlackTextureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = (ClutterActorClass *) klass;
  GParamSpec *pspec;

  object_class->get_property = black_texture_get_property;
  object_class->set_property = black_texture_set_property;

  actor_class->paint = black_texture_paint;

  pspec = g_param_spec_uint ("shade", "Shade",
                             "Amount to shade the texture",
                             0, 255, 255,
                             (GParamFlags)(G_PARAM_READABLE
                             | G_PARAM_WRITABLE
                             | G_PARAM_STATIC_NAME
                             | G_PARAM_STATIC_NICK
                             | G_PARAM_STATIC_BLURB));
  g_object_class_install_property (object_class, PROP_SHADE, pspec);
}

static void
black_texture_init (BlackTexture *self)
{
  /*self->shade = 0xff;*/
  /*
  Create the texture as totaly black
  this way, when the first fadeout is called, the item appears from black
  */
  self->shade = 0;			
}

ClutterActor *
black_texture_new (void)
{
  ClutterActor *self = (ClutterActor*)g_object_new (TEST_TYPE_BLACK_TEXTURE, NULL);

  return self;
}
