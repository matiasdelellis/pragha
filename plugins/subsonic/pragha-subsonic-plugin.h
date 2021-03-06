/*************************************************************************/
/* Copyright (C) 2019 matias <mati86dl@gmail.com>                        */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

#ifndef __PRAGHA_SUBSONIC_PLUGIN_H__
#define __PRAGHA_SUBSONIC_PLUGIN_H__

#include <gtk/gtk.h>
#include <libpeas/peas.h>

G_BEGIN_DECLS

#define PRAGHA_TYPE_SUBSONIC_PLUGIN         (pragha_subsonic_plugin_get_type ())
#define PRAGHA_SUBSONIC_PLUGIN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), PRAGHA_TYPE_SUBSONIC_PLUGIN, PraghaSubsonicPlugin))
#define PRAGHA_SUBSONICP_LUGIN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), PRAGHA_TYPE_SUBSONIC_PLUGIN, PraghaSubsonicPlugin))
#define PRAGHA_IS_SUBSONIC_PLUGIN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), PRAGHA_TYPE_SUBSONIC_PLUGIN))
#define PRAGHA_IS_SUBSONIC_PLUGIN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), PRAGHA_TYPE_SUBSONIC_PLUGIN))
#define PRAGHA_SUBSONIC_PLUGIN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), PRAGHA_TYPE_SUBSONIC_PLUGIN, PraghaSubsonicPluginClass))

GType   pragha_subsonic_plugin_get_type     (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __PRAGHA_SUBSONIC_PLUGIN_H__ */
