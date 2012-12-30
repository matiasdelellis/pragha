/*************************************************************************/
/* Copyright (C) 2012 matias <mati86dl@gmail.com>			 */
/* 									 */
/* This program is free software: you can redistribute it and/or modify	 */
/* it under the terms of the GNU General Public License as published by	 */
/* the Free Software Foundation, either version 3 of the License, or	 */
/* (at your option) any later version.					 */
/* 									 */
/* This program is distributed in the hope that it will be useful,	 */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of	 */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	 */
/* GNU General Public License for more details.				 */
/* 									 */
/* You should have received a copy of the GNU General Public License	 */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

#include "pragha-album-art.h"

G_DEFINE_TYPE(PraghaAlbumArt, pragha_album_art, GTK_TYPE_IMAGE)

struct _PraghaAlbumArtPrivate
{
   gchar *path;
   guint size;
   gboolean visible;
};

enum
{
   PROP_0,
   PROP_PATH,
   PROP_SIZE,
   PROP_VISIBLE,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

PraghaAlbumArt *
pragha_album_art_new (void)
{
   return g_object_new(PRAGHA_TYPE_ALBUM_ART, NULL);
}

/**
 * pragha_album_art_update_image:
 *
 */

static void
pragha_album_art_update_image (PraghaAlbumArt *albumart)
{
   PraghaAlbumArtPrivate *priv;
   GdkPixbuf *pixbuf, *album_art, *frame;
   GError *error = NULL;

   g_return_if_fail(PRAGHA_IS_ALBUM_ART(albumart));

   priv = albumart->priv;

   frame = gdk_pixbuf_new_from_file (PIXMAPDIR"/cover.png", &error);
   if(priv->path != NULL) {
      album_art = gdk_pixbuf_new_from_file_at_scale(priv->path,
                                                    112,
                                                    112,
                                                    FALSE,
                                                    &error);
      if (album_art) {
         gdk_pixbuf_copy_area(album_art, 0 ,0 ,112 ,112, frame, 12, 8);
         g_object_unref(G_OBJECT(album_art));
      }
      else {
         g_critical("Unable to open image file: %s\n", priv->path);
         g_error_free(error);
      }
   }

   pixbuf = gdk_pixbuf_scale_simple (frame,
                                     priv->size,
                                     priv->size,
                                     GDK_INTERP_BILINEAR);

   pragha_album_art_set_pixbuf(albumart, pixbuf);

   g_object_unref(G_OBJECT(pixbuf));
   g_object_unref(G_OBJECT(frame));
}

/**
 * album_art_get_path:
 *
 */
const gchar *
pragha_album_art_get_path (PraghaAlbumArt *albumart)
{
   g_return_val_if_fail(PRAGHA_IS_ALBUM_ART(albumart), NULL);
   return albumart->priv->path;
}

/**
 * album_art_set_path:
 *
 */
void
pragha_album_art_set_path (PraghaAlbumArt *albumart,
                          const gchar *path)
{
   PraghaAlbumArtPrivate *priv;

   g_return_if_fail(PRAGHA_IS_ALBUM_ART(albumart));

   priv = albumart->priv;

   g_free(priv->path);
   priv->path = g_strdup(path);

   pragha_album_art_update_image(albumart);

   g_object_notify_by_pspec(G_OBJECT(albumart), gParamSpecs[PROP_PATH]);
}

/**
 * album_art_get_size:
 *
 */
guint
pragha_album_art_get_size (PraghaAlbumArt *albumart)
{
   g_return_val_if_fail(PRAGHA_IS_ALBUM_ART(albumart), 0);
   return albumart->priv->size;
}

/**
 * album_art_set_size:
 *
 */
void
pragha_album_art_set_size (PraghaAlbumArt *albumart,
                           guint size)
{
   PraghaAlbumArtPrivate *priv;

   g_return_if_fail(PRAGHA_IS_ALBUM_ART(albumart));

   priv = albumart->priv;

   priv->size = size;

   pragha_album_art_update_image(albumart);

   g_object_notify_by_pspec(G_OBJECT(albumart), gParamSpecs[PROP_SIZE]);
}

/**
 * album_art_set_pixbuf:
 *
 */
void
pragha_album_art_set_pixbuf (PraghaAlbumArt *albumart, GdkPixbuf *pixbuf)
{
   g_return_if_fail(PRAGHA_IS_ALBUM_ART(albumart));

   gtk_image_clear(GTK_IMAGE(albumart));
   gtk_image_set_from_pixbuf(GTK_IMAGE(albumart), pixbuf);
}

/**
 * album_art_get_pixbuf:
 *
 */
GdkPixbuf *
pragha_album_art_get_pixbuf (PraghaAlbumArt *albumart)
{
   GdkPixbuf *pixbuf = NULL;

   g_return_val_if_fail(PRAGHA_IS_ALBUM_ART(albumart), NULL);

   if(gtk_image_get_storage_type(GTK_IMAGE(albumart)) == GTK_IMAGE_PIXBUF)
      pixbuf = gtk_image_get_pixbuf(GTK_IMAGE(albumart));

   return pixbuf;
}

/**
 * album_art_get_visible:
 *
 */
gboolean
pragha_album_art_get_visible (PraghaAlbumArt *albumart)
{
   g_return_val_if_fail(PRAGHA_IS_ALBUM_ART(albumart), FALSE);
   return albumart->priv->visible;
}

/**
 * album_art_set_visible:
 *
 */
void
pragha_album_art_set_visible (PraghaAlbumArt *albumart,
                              gboolean visible)
{
   PraghaAlbumArtPrivate *priv;

   g_return_if_fail(PRAGHA_IS_ALBUM_ART(albumart));

   priv = albumart->priv;

   gtk_widget_set_visible (GTK_WIDGET(albumart), visible);
   priv->visible = visible;

   g_object_notify_by_pspec(G_OBJECT(albumart), gParamSpecs[PROP_VISIBLE]);
}

static void
pragha_album_art_finalize (GObject *object)
{
   PraghaAlbumArtPrivate *priv;

   priv = PRAGHA_ALBUM_ART(object)->priv;

   g_free(priv->path);

   G_OBJECT_CLASS(pragha_album_art_parent_class)->finalize(object);
}

static void
pragha_album_art_get_property (GObject *object,
                               guint prop_id,
                               GValue *value,
                               GParamSpec *pspec)
{
   PraghaAlbumArt *albumart = PRAGHA_ALBUM_ART(object);

   switch (prop_id) {
   case PROP_PATH:
      g_value_set_string(value, pragha_album_art_get_path(albumart));
      break;
   case PROP_SIZE:
      g_value_set_uint (value, pragha_album_art_get_size(albumart));
      break;
   case PROP_VISIBLE:
      g_value_set_boolean (value, pragha_album_art_get_visible(albumart));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
pragha_album_art_set_property (GObject *object,
                               guint prop_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
   PraghaAlbumArt *albumart = PRAGHA_ALBUM_ART(object);

   switch (prop_id) {
   case PROP_PATH:
      pragha_album_art_set_path(albumart, g_value_get_string(value));
      break;
   case PROP_SIZE:
      pragha_album_art_set_size(albumart, g_value_get_uint(value));
      break;
   case PROP_VISIBLE:
      pragha_album_art_set_visible(albumart, g_value_get_boolean(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

static void
pragha_album_art_class_init (PraghaAlbumArtClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = pragha_album_art_finalize;
   object_class->get_property = pragha_album_art_get_property;
   object_class->set_property = pragha_album_art_set_property;
   g_type_class_add_private(object_class, sizeof(PraghaAlbumArtPrivate));

   /**
    * PraghaAlbumArt:path:
    *
    */
   gParamSpecs[PROP_PATH] =
      g_param_spec_string("path",
                          "Path",
                          "The album art path",
                          NULL,
                          G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS);

   /**
    * PraghaAlbumArt:size:
    *
    */
   gParamSpecs[PROP_SIZE] =
      g_param_spec_uint("size",
                        "Size",
                        "The album art size",
                        36, 128,
                        48,
                        G_PARAM_READWRITE |
                        G_PARAM_STATIC_STRINGS);

   /**
    * PraghaAlbumArt:visible:
    *
    */
   gParamSpecs[PROP_VISIBLE] =
      g_param_spec_boolean("visible",
                           "Visible",
                           "The album art visibility state",
                           FALSE,
                           G_PARAM_READWRITE |
                           G_PARAM_STATIC_STRINGS);
 
   g_object_class_install_properties(object_class, LAST_PROP, gParamSpecs);
}

static void
pragha_album_art_init (PraghaAlbumArt *albumart)
{
   albumart->priv = G_TYPE_INSTANCE_GET_PRIVATE(albumart,
                                               PRAGHA_TYPE_ALBUM_ART,
                                               PraghaAlbumArtPrivate);
}
