/*************************************************************************/
/* Copyright (C) 2007-2009 sujith <m.sujith@gmail.com>			 */
/* Copyright (C) 2009-2012 matias <mati86dl@gmail.com>			 */
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

#include "pragha.h"
#include <locale.h> /* require LC_ALL */

gint debug_level;

/* FIXME: Cleanup track refs */
void common_cleanup(struct con_win *cwin)
{
	CDEBUG(DBG_INFO, "Cleaning up");

	backend_quit(cwin);

	g_object_unref(cwin->library_store);
	g_object_unref(cwin->pixbuf->image_play);
	g_object_unref(cwin->pixbuf->image_pause);

	g_object_unref(cwin->pixbuf->pixbuf_playing);
	g_object_unref(cwin->pixbuf->pixbuf_paused);

	if (cwin->pixbuf->pixbuf_app)
		g_object_unref(cwin->pixbuf->pixbuf_app);
	if (cwin->pixbuf->pixbuf_dir)
		g_object_unref(cwin->pixbuf->pixbuf_dir);
	if (cwin->pixbuf->pixbuf_artist)
		g_object_unref(cwin->pixbuf->pixbuf_artist);
	if (cwin->pixbuf->pixbuf_album)
		g_object_unref(cwin->pixbuf->pixbuf_album);
	if (cwin->pixbuf->pixbuf_track)
		g_object_unref(cwin->pixbuf->pixbuf_track);
	if (cwin->pixbuf->pixbuf_genre)
		g_object_unref(cwin->pixbuf->pixbuf_genre);

	g_slice_free(struct pixbuf, cwin->pixbuf);

	if (cwin->album_art)
		gtk_widget_destroy(cwin->album_art);

	if (cwin->cstate->cdda_drive)
		cdio_cddap_close(cwin->cstate->cdda_drive);
	if (cwin->cstate->cddb_disc)
		cddb_disc_destroy(cwin->cstate->cddb_disc);
	if (cwin->cstate->cddb_conn) {
		cddb_destroy(cwin->cstate->cddb_conn);
		libcddb_shutdown();
	}
#ifdef HAVE_LIBCLASTFM
	g_free(cwin->cpref->lw.lastfm_user);
	g_free(cwin->cpref->lw.lastfm_pass);
#endif
#ifdef HAVE_LIBGLYR
	g_free(cwin->cpref->cache_folder);
#endif
	g_free(cwin->cpref->configrc_file);
	g_free(cwin->cpref->installed_version);
	g_free(cwin->cpref->album_art_pattern);
	g_free(cwin->cpref->audio_cd_device);
	g_free(cwin->cpref->start_mode);
	g_free(cwin->cpref->sidebar_pane);
	g_key_file_free(cwin->cpref->configrc_keyfile);
	free_str_list(cwin->cpref->library_dir);
	free_str_list(cwin->cpref->lib_add);
	free_str_list(cwin->cpref->lib_delete);
	free_str_list(cwin->cpref->library_tree_nodes);
	free_str_list(cwin->cpref->playlist_columns);
	g_slist_free(cwin->cpref->playlist_column_widths);
	g_slice_free(struct con_pref, cwin->cpref);

	g_rand_free(cwin->cstate->rand);
	g_free(cwin->cstate->last_folder);

	g_slice_free(struct con_state, cwin->cstate);

#ifdef HAVE_LIBGLYR
	uninit_glyr_related (cwin);
#endif
	g_free(cwin->cdbase->db_file);
	sqlite3_close(cwin->cdbase->db);
	g_slice_free(struct con_dbase, cwin->cdbase);

#ifdef HAVE_LIBCLASTFM
	if (cwin->clastfm->session_id)
		LASTFM_dinit(cwin->clastfm->session_id);

	g_slice_free(struct tags, cwin->clastfm->ntags);
	g_slice_free(struct con_lastfm, cwin->clastfm);
#endif

	dbus_connection_remove_filter(cwin->con_dbus,
				      dbus_filter_handler,
				      cwin);
	dbus_bus_remove_match(cwin->con_dbus,
			      "type='signal',path='/org/pragha/DBus'",
			      NULL);
	dbus_connection_unref(cwin->con_dbus);

#if GLIB_CHECK_VERSION(2,26,0)
	mpris_cleanup(cwin);
#endif

	if (notify_is_initted())
		notify_uninit();

#ifdef HAVE_LIBKEYBINDER
	cleanup_keybinder(cwin);
#elif GLIB_CHECK_VERSION(2,26,0)
	cleanup_gnome_media_keys(cwin);
#endif

	g_option_context_free(cwin->cmd_context);

	g_slice_free(struct con_win, cwin);
}

void exit_pragha(GtkWidget *widget, struct con_win *cwin)
{
	if (cwin->cpref->save_playlist)
		save_current_playlist_state(cwin);
	save_preferences(cwin);
	common_cleanup(cwin);

	gtk_main_quit();

	CDEBUG(DBG_INFO, "Halt.");
}

gint main(gint argc, gchar *argv[])
{
	struct con_win *cwin;

	cwin = g_slice_new0(struct con_win);
	cwin->pixbuf = g_slice_new0(struct pixbuf);
	cwin->cpref = g_slice_new0(struct con_pref);
	cwin->cstate = g_slice_new0(struct con_state);
	cwin->cdbase = g_slice_new0(struct con_dbase);
	cwin->cgst = g_slice_new0(struct con_gst);
#ifdef HAVE_LIBCLASTFM
	cwin->clastfm = g_slice_new0(struct con_lastfm);
	cwin->clastfm->ntags = g_slice_new0(struct tags);
#endif
#if GLIB_CHECK_VERSION(2,26,0)
	cwin->cmpris2 = g_slice_new0(struct con_mpris2);
#endif

	if(init_first_state(cwin) == -1)
		return -1;

	debug_level = 0;

	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	if (init_threads(cwin) == -1) {
		g_critical("Unable to init threads");
		return -1;
	}

	if (init_dbus(cwin) == -1) {
		g_critical("Unable to init dbus connection");
		return -1;
	}

	if (init_dbus_handlers(cwin) == -1) {
		g_critical("Unable to initialize DBUS filter handlers");
		return -1;
	}

	if (init_options(cwin, argc, argv) == -1)
		return -1;

	/* Allow only one instance */

	if (!cwin->cstate->unique_instance)
		return 0;

	if (init_config(cwin) == -1) {
		g_critical("Unable to init configuration");
		return -1;
	}

	if (init_musicdbase(cwin) == -1) {
		g_critical("Unable to init music dbase");
		return -1;
	}

	if (init_notify(cwin) == -1) {
		g_critical("Unable to initialize libnotify");
		return -1;
	}

	#ifdef HAVE_LIBCLASTFM
	if (init_lastfm_idle(cwin) == -1) {
		g_critical("Unable to initialize lastfm");
	}
	#endif

	#ifdef HAVE_LIBGLYR
	if (init_glyr_related(cwin) == -1) {
		g_critical("Unable to initialize libglyr");
	}
	#endif

	#if GLIB_CHECK_VERSION(2,26,0)
	if (mpris_init(cwin) == -1) {
		g_critical("Unable to initialize MPRIS");
		return -1;
	}
	#endif

	if(backend_init(cwin) == -1) {
		g_critical("Unable to initialize gstreamer");
		return -1;
	}

	/* Init the gui after bancked to sink volume. */
	gdk_threads_enter();
	init_gui(argc, argv, cwin);

	/* Init_gnome_media_keys requires constructed main window. */
	#ifdef HAVE_LIBKEYBINDER
	if (init_keybinder(cwin) == -1) {
		g_critical("Unable to initialize keybinder");
		return -1;
	}
	#elif GLIB_CHECK_VERSION(2,26,0)
	if (init_gnome_media_keys(cwin) == -1) {
		g_critical("Unable to initialize gnome media keys");
		return -1;
	}
	#endif

	CDEBUG(DBG_INFO, "Init done. Running ...");

	gtk_main();
	gdk_threads_leave();

	return 0;
}
