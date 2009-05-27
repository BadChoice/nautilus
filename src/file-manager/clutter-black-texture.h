#ifndef _BLACKTEXTURE_
#define _BLACKTEXTURE_

//gcc test-black-tex2.c -Wall `pkg-config --cflags clutter-0.9` `pkg-config --libs clutter-0.9`

#include <clutter/clutter.h>

#define TEST_TYPE_BLACK_TEXTURE                                         \
  (black_texture_get_type())
#define TEST_BLACK_TEXTURE(obj)                                         \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                   \
                               TEST_TYPE_BLACK_TEXTURE,                 \
                               BlackTexture))
#define TEST_BLACK_TEXTURE_CLASS(klass)                                 \
  (G_TYPE_CHECK_CLASS_CAST ((klass),                                    \
                            TEST_TYPE_BLACK_TEXTURE,                    \
                            BlackTextureClass))
#define TEST_IS_BLACK_TEXTURE(obj)                                      \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                   \
                               TEST_TYPE_BLACK_TEXTURE))
#define TEST_IS_BLACK_TEXTURE_CLASS(klass)                              \
  (G_TYPE_CHECK_CLASS_TYPE ((klass),                                    \
                            TEST_TYPE_BLACK_TEXTURE))
#define TEST_BLACK_TEXTURE_GET_CLASS(obj)                               \
  (G_TYPE_INSTANCE_GET_CLASS ((obj),                                    \
                              TEST_TYPE_BLACK_TEXTURE,                  \
                              BlackTextureClass))

typedef struct _BlackTexture     	BlackTexture;

ClutterActor * black_texture_new (void);

#endif

