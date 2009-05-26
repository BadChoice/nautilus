/* clutter-cover-flow.c */

#include <glib.h>
#include "clutter-cover-flow.h"

G_DEFINE_TYPE (ClutterCoverFlow, clutter_cover_flow, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CLUTTER_TYPE_COVER_FLOW, ClutterCoverFlowPrivate))

#define ICON_SIZE           200
#define VISIBLE_ITEMS		8
#define FRAMES				40
#define FPS					40
#define MAX_ANGLE			70
#define COVER_SPACE			50
#define FRONT_COVER_SPACE 	200
#define DEPTH				450

#define MAX_ITEM_HEIGHT		240

#define DEFAULT_WIDTH  		1200
#define DEFAULT_HEIGHT		500

struct _CoverflowItem
{
	int x;	
	int y;
	int depth;
	int angle;
	int opacity;
	
	ClutterActor		*container;
	ClutterActor		*texture;
	ClutterActor		*reflection;
	char				*filename;
	
	ClutterBehaviour	*rotateBehaviour;
	ClutterAnimation	*animation;
};

struct _ClutterCoverFlowPrivate {
    GList                       *m_items;
    int                         nitems;
    ClutterActor 				*m_stage;					//stage (Window)
    ClutterActor				*m_text;					//Text to display
    int       	 				m_actualItem;				//Item now in front
    int         				m_nextItem;					//Next item to be in front
    ClutterAlpha 				*m_alpha;					//Alpha function
    ClutterTimeline 			*m_timeline;				//Timeline (Values in defines.h)
    int 						m_middle_x;
    int 						m_middle_y;
    ClutterActor				*m_container;
    int   						m_loaded;					//Pixbuf Loadeds
};

typedef struct _CoverflowItem CoverFlowItem;

void fade_in(ClutterActor *container);
static void scale_to_fit(ClutterActor *actor);
static void add_file(ClutterCoverFlow *coverflow, GdkPixbuf *pb, char *filename);

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
}

void fade_in(ClutterActor *container)
{
    int opacity;
	ClutterTimeline  *timeline;
	ClutterAlpha 	 *alpha;

	timeline 	= clutter_timeline_new(FRAMES /* frames */, FPS /* frames per second. */);
	alpha 	= clutter_alpha_new_full (timeline,CLUTTER_EASE_OUT_EXPO);
    opacity = 125;
	
//	int distance = item - m_actualItem;
//	
//	int opacity = 255*(VISIBLE_ITEMS - distance)/VISIBLE_ITEMS;
//	if(opacity<0) opacity = 0;
	
	ClutterBehaviour *beh = clutter_behaviour_opacity_new (alpha, 0, opacity);
	clutter_behaviour_apply (beh, container);
	clutter_timeline_start	(timeline);
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
add_file(ClutterCoverFlow *coverflow, GdkPixbuf *pb, char *filename)
{
    int bps;
    CoverFlowItem *item;
    ClutterCoverFlowPrivate *priv;

    priv = coverflow->priv;
    item = g_new0 (CoverFlowItem, 1);

    item->texture = clutter_texture_new();

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

	/* Text */
	item->filename = filename;
 
 	
	/* Container */
	item->container	= clutter_group_new();
	clutter_group_add_many	(
                CLUTTER_GROUP(item->container),
                item->texture, item->reflection, NULL );
	clutter_container_add_actor	(
                CLUTTER_CONTAINER(priv->m_container),
                item->container);

//	if(item_pos > 0)	
//	{
//		clutter_actor_set_rotation( temp.container, CLUTTER_Y_AXIS, 360 - MAX_ANGLE,clutter_actor_get_width(temp.texture)/2,0,0);
//		
//		int pos = (item_pos - 1) * COVER_SPACE + FRONT_COVER_SPACE;
//		clutter_actor_set_position ( temp.container, 
//									pos - clutter_actor_get_width(temp.texture)/2, 
//									110 - clutter_actor_get_height(temp.texture));		
//		clutter_actor_set_depth	    ( temp.container, 0 );
//	}
//	if(item_pos < 0 )
//	{
//		clutter_actor_set_rotation(temp.container, CLUTTER_Y_AXIS, MAX_ANGLE,clutter_actor_get_width(temp.texture)/2,0,0);
//		clutter_actor_set_depth	    ( temp.container, 0 );
//	}
	
	if( priv->nitems == 0)
	{
		clutter_actor_set_rotation	(
                item->container,
                CLUTTER_Y_AXIS,0,
                clutter_actor_get_width(item->texture)/2,
                0,0);
		clutter_actor_set_depth	( item->container, DEPTH );
		clutter_actor_set_position 	( 
                item->container, 
                0 - clutter_actor_get_width(item->texture)/2, 
                110 - clutter_actor_get_height(item->texture));
	}
	
	//SET BEHAVIOURS AS NULL
	//temp.rotateBehaviour  = NULL;
	
	//if(item_pos > 1)	clutter_actor_lower_bottom (m_items[item_pos-1].container  );		//Put back
	//					clutter_actor_lower_bottom (temp.container  );						//Put back
	//m_items.push_back   (temp);
	
	fade_in	(item->container);

#if 0
printf("************************************ ADD\n");
	CoverflowItem temp;
	
	int item_pos = m_items.size();
	
	temp.texture = clutter_texture_new();
	//temp.texture = 	test_black_texture_new ();

	int bps;
	if(gdk_pixbuf_get_has_alpha	(px)) bps = 4;
	else							  bps = 3;
		
	//printf("PXBUF : %ix%i\n", gdk_pixbuf_get_width(px) , gdk_pixbuf_get_height(px));
	
	clutter_texture_set_from_rgb_data(	CLUTTER_TEXTURE(temp.texture), 
										gdk_pixbuf_get_pixels		(px),
										gdk_pixbuf_get_has_alpha	(px),
										gdk_pixbuf_get_width		(px),
										gdk_pixbuf_get_height		(px),
										gdk_pixbuf_get_rowstride	(px),
										bps,
										(ClutterTextureFlags)0,
										NULL);
										
	
	scale_to_fit				( temp.texture );
	clutter_actor_set_position 	( temp.texture,  0, 0);	//Set position depending on the item
  
  	//REFLECTION
  	temp.reflection = clutter_clone_new			( temp.texture );
  	clutter_actor_set_opacity           		( temp.reflection, 60);
	scale_to_fit								( temp.reflection 	 );
  	clutter_actor_set_position 					( temp.reflection,  0, clutter_actor_get_height(temp.reflection));
  												  
	clutter_actor_set_rotation					( temp.reflection,CLUTTER_Z_AXIS,180,
												clutter_actor_get_width(temp.reflection)/2,
												clutter_actor_get_height(temp.reflection)/2,
												0);
	clutter_actor_set_rotation					(temp.reflection,CLUTTER_Y_AXIS,180,	
												clutter_actor_get_width(temp.reflection)/2,
												clutter_actor_get_height(temp.reflection)/2,
												0);
	
	
	//TEXT
	temp.filename = filename;
 
 	
	//CONTAINER
	temp.container			     = clutter_group_new();
	clutter_group_add_many		( CLUTTER_GROUP		(temp.container), temp.texture, temp.reflection, NULL );
	clutter_container_add_actor	( CLUTTER_CONTAINER (m_container), temp.container);
									  
	if(item_pos > 0)	
	{
		clutter_actor_set_rotation( temp.container, CLUTTER_Y_AXIS, 360 - MAX_ANGLE,clutter_actor_get_width(temp.texture)/2,0,0);
		
		int pos = (item_pos - 1) * COVER_SPACE + FRONT_COVER_SPACE;
		clutter_actor_set_position ( temp.container, 
									pos - clutter_actor_get_width(temp.texture)/2, 
									110 - clutter_actor_get_height(temp.texture));		
		clutter_actor_set_depth	    ( temp.container, 0 );
	}
	if(item_pos < 0 )
	{
		clutter_actor_set_rotation(temp.container, CLUTTER_Y_AXIS, MAX_ANGLE,clutter_actor_get_width(temp.texture)/2,0,0);
		clutter_actor_set_depth	    ( temp.container, 0 );
	}
	
	if(item_pos ==0 )
	{
		clutter_actor_set_rotation	( temp.container, CLUTTER_Y_AXIS,0,clutter_actor_get_width(temp.texture)/2,0,0);
		clutter_actor_set_depth	    ( temp.container, DEPTH );
		clutter_actor_set_position 	( temp.container, 
									0 - clutter_actor_get_width(temp.texture)/2, 
									110 - clutter_actor_get_height(temp.texture));
	}
	
	//SET BEHAVIOURS AS NULL
	temp.rotateBehaviour  = NULL;
	
	if(item_pos > 1)	clutter_actor_lower_bottom (m_items[item_pos-1].container  );		//Put back
						clutter_actor_lower_bottom (temp.container  );						//Put back
	m_items.push_back   (temp);
	
	fade_in			((int)m_items.size()-1);
	

#endif
}

ClutterCoverFlow*
clutter_cover_flow_new (ClutterActor *stage)
{
  ClutterCoverFlow *self;
  int w,h;
  ClutterColor color = { 255, 255, 255, 255 };

  g_return_val_if_fail(CLUTTER_IS_STAGE(stage), NULL);

  self = g_object_new (CLUTTER_TYPE_COVER_FLOW, NULL);
  self->priv->m_stage = stage;

  /* Add ourselves to the stage */
  clutter_container_add_actor ( CLUTTER_CONTAINER (stage), CLUTTER_ACTOR(self) );

  w = clutter_actor_get_width(CLUTTER_ACTOR(stage));
  h = clutter_actor_get_height(CLUTTER_ACTOR(stage));

  /* Add a container, that will hold all covers, as our child */
  self->priv->m_container = clutter_group_new();
  self->priv->m_middle_x = w/2;
  self->priv->m_middle_y = h/2;

  g_debug("%dx%d", self->priv->m_middle_x, self->priv->m_middle_y);

  clutter_container_add_actor ( CLUTTER_CONTAINER (self), self->priv->m_container );
  clutter_actor_set_position ( self->priv->m_container, self->priv->m_middle_x , self->priv->m_middle_y );
	
  /* Add some text as our child */	
  self->priv->m_text = clutter_text_new_full ("Lucida Grande 11", "Hello", &color);
  clutter_container_add_actor (CLUTTER_CONTAINER (self), self->priv->m_text);
  clutter_actor_set_position (
                    self->priv->m_text, 
                    w/2 - clutter_actor_get_width(self->priv->m_text)/2,
                    h/2 + 200 );
  clutter_actor_raise_top (self->priv->m_text);

  return self;
}


void clutter_cover_flow_add_gfile(ClutterCoverFlow *coverflow, GFile *file)
{
    GIcon *icon;
    GtkIconInfo *icon_info;
    GFileInfo *file_info;
    GtkIconTheme *icon_theme;
    GdkPixbuf *pb;

    icon_theme = gtk_icon_theme_get_default();
	file_info = g_file_query_info(
                    file,
                    G_FILE_ATTRIBUTE_STANDARD_ICON "," G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
                    G_FILE_QUERY_INFO_NONE, NULL , NULL);
    icon = g_file_info_get_icon(file_info);
    icon_info = gtk_icon_theme_lookup_by_gicon(
                    icon_theme,
                    icon,
                    ICON_SIZE,
                    GTK_ICON_LOOKUP_USE_BUILTIN | GTK_ICON_LOOKUP_GENERIC_FALLBACK | GTK_ICON_LOOKUP_FORCE_SIZE);
    pb = gtk_icon_info_load_icon(icon_info, NULL);

    //FIXME: Leaks, error checking

    add_file(coverflow, pb, g_strdup("test"));
}

void clutter_cover_flow_add_gicon(ClutterCoverFlow *coverflow, GIcon *icon, const gchar *filename)
{

}
