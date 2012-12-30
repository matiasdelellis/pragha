/*************************************************************************/
/* Copyright (C) 2011-2012 matias <mati86dl@gmail.com>			 */
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

#include <stdio.h>
#include "pragha.h"

#ifdef HAVE_LIBCLASTFM

enum LASTFM_QUERY_TYPE {
	LASTFM_NONE = 0,
	LASTFM_GET_SIMILAR,
	LASTFM_GET_LOVED
};

typedef struct {
	GList *list;
	guint query_type;
	guint query_count;
	struct con_win *cwin;
} AddMusicObjectListData;

void
update_menubar_lastfm_state (struct con_win *cwin)
{
	GtkAction *action;

	gboolean playing = pragha_backend_get_state (cwin->backend) != ST_STOPPED;
	gboolean logged = cwin->clastfm->status == LASTFM_STATUS_OK;
	gboolean lfm_inited = cwin->clastfm->session_id != NULL;
	gboolean has_user = lfm_inited && string_is_not_empty(cwin->cpref->lastfm_user);

	action = gtk_ui_manager_get_action(cwin->bar_context_menu, "/Menubar/ToolsMenu/Lastfm/Love track");
	gtk_action_set_sensitive (GTK_ACTION (action), playing && logged);

	action = gtk_ui_manager_get_action(cwin->bar_context_menu, "/Menubar/ToolsMenu/Lastfm/Unlove track");
	gtk_action_set_sensitive (GTK_ACTION (action), playing && logged);

	action = gtk_ui_manager_get_action(cwin->bar_context_menu, "/Menubar/ToolsMenu/Lastfm/Add favorites");
	gtk_action_set_sensitive (GTK_ACTION (action), has_user);

	action = gtk_ui_manager_get_action(cwin->bar_context_menu, "/Menubar/ToolsMenu/Lastfm/Add similar");
	gtk_action_set_sensitive (GTK_ACTION (action), playing && lfm_inited);

	action = gtk_ui_manager_get_action(cwin->cplaylist->cp_context_menu, "/popup/ToolsMenu/Love track");
	gtk_action_set_sensitive (GTK_ACTION (action), logged);

	action = gtk_ui_manager_get_action(cwin->cplaylist->cp_context_menu, "/popup/ToolsMenu/Unlove track");
	gtk_action_set_sensitive (GTK_ACTION (action), logged);

	action = gtk_ui_manager_get_action(cwin->cplaylist->cp_context_menu, "/popup/ToolsMenu/Add similar");
	gtk_action_set_sensitive (GTK_ACTION (action), lfm_inited);
}

/* Set correction basedm on lastfm now playing segestion.. */

void
edit_tags_corrected_by_lastfm(GtkButton *button, struct con_win *cwin)
{
	PraghaMusicobject *omobj, *nmobj, *tmobj;
	const gchar *file, *otitle, *oartist, *oalbum;
	const gchar *ntitle, *nartist, *nalbum;
	gchar *sfile = NULL;
	gint location_id, changed = 0, prechanged = 0;
	GPtrArray *file_arr = NULL;
	GArray *loc_arr = NULL;

	if(pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return;

	g_object_ref(cwin->cstate->curr_mobj);
	omobj = cwin->cstate->curr_mobj;

	file = pragha_musicobject_get_file(omobj);
	otitle = pragha_musicobject_get_title(omobj);
	oartist = pragha_musicobject_get_artist(omobj);
	oalbum = pragha_musicobject_get_album(omobj);

	g_object_ref(cwin->clastfm->nmobj);
	nmobj = cwin->clastfm->nmobj;

	ntitle = pragha_musicobject_get_title(nmobj);
	nartist = pragha_musicobject_get_artist(nmobj);
	nalbum = pragha_musicobject_get_album(nmobj);

	if(g_ascii_strcasecmp(otitle, ntitle))
		prechanged |= TAG_TITLE_CHANGED;
	if(g_ascii_strcasecmp(oartist, nartist))
		prechanged |= TAG_ARTIST_CHANGED;
	if(g_ascii_strcasecmp(oalbum, nalbum))
		prechanged |= TAG_ALBUM_CHANGED;

	tmobj = pragha_musicobject_new();
	changed = tag_edit_dialog(nmobj, prechanged, tmobj, cwin);

	if (!changed)
		goto exit;

	/* Store the new tags */

	if (G_LIKELY(pragha_musicobject_get_file_type(omobj) != FILE_CDDA &&
	    pragha_musicobject_get_file_type(omobj) != FILE_HTTP)) {
		loc_arr = g_array_new(TRUE, TRUE, sizeof(gint));
		sfile = sanitize_string_to_sqlite3(file);
		location_id = find_location_db(sfile, cwin->cdbase);
		if (location_id) {
			g_array_append_val(loc_arr, location_id);
			pragha_db_update_local_files_change_tag(cwin->cdbase, loc_arr, changed, tmobj);
			init_library_view(cwin);
		}
		g_array_free(loc_arr, TRUE);
		g_free(sfile);

		file_arr = g_ptr_array_new();
		g_ptr_array_add(file_arr, g_strdup(file));
		pragha_update_local_files_change_tag(file_arr, changed, tmobj);
		g_ptr_array_free(file_arr, TRUE);
	}

	/* Update the musicobject, the gui and them mpris */

	pragha_update_musicobject_change_tag(omobj, changed, tmobj);

	/* While the dialog is open, the song may have changed or stopped */

	if(pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		goto exit;

	if(omobj == cwin->cstate->curr_mobj) {
		pragha_playlist_update_current_track(cwin->cplaylist, changed);
		__update_current_song_info(cwin);
		mpris_update_metadata_changed(cwin);
	}

exit:
	gtk_widget_hide(cwin->ntag_lastfm_button);
	g_object_unref(omobj);
	g_object_unref(nmobj);
	g_object_unref(tmobj);
}

/* Love and unlove music object */

gpointer
do_lastfm_love_mobj (PraghaMusicobject *mobj, struct con_win *cwin)
{
	AsycMessageData *msg_data = NULL;
	gint rv;

	CDEBUG(DBG_LASTFM, "Love mobj on thread");

	rv = LASTFM_track_love (cwin->clastfm->session_id,
				pragha_musicobject_get_title(mobj),
				pragha_musicobject_get_artist(mobj));

	if (rv != LASTFM_STATUS_OK)
		msg_data = async_finished_message_new(_("Love song on Last.fm failed."), cwin);

	return msg_data;
}

gpointer
do_lastfm_unlove_mobj (PraghaMusicobject *mobj, struct con_win *cwin)
{
	AsycMessageData *msg_data = NULL;
	gint rv;

	CDEBUG(DBG_LASTFM, "Unlove mobj on thread");

	rv = LASTFM_track_unlove(cwin->clastfm->session_id,
				 pragha_musicobject_get_title(mobj),
				 pragha_musicobject_get_artist(mobj));

	if (rv != LASTFM_STATUS_OK)
		msg_data = async_finished_message_new(_("Unlove song on Last.fm failed."), cwin);

	return msg_data;
}


/* Functions related to current playlist. */

gpointer
do_lastfm_current_playlist_love (gpointer data)
{
	PraghaMusicobject *mobj = NULL;
	AsycMessageData *msg_data = NULL;

	struct con_win *cwin = data;

	mobj = pragha_playlist_get_selected_musicobject(cwin->cplaylist);

	msg_data = do_lastfm_love_mobj(mobj, cwin);

	return msg_data;
}

void
lastfm_track_current_playlist_love_action (GtkAction *action, struct con_win *cwin)
{
	CDEBUG(DBG_LASTFM, "Love handler to current playlist");

	if(cwin->clastfm->status != LASTFM_STATUS_OK) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	pragha_async_launch(do_lastfm_current_playlist_love,
			    set_async_finished_message,
			    cwin);
}

gpointer
do_lastfm_current_playlist_unlove (gpointer data)
{
	PraghaMusicobject *mobj = NULL;
	AsycMessageData *msg_data = NULL;

	struct con_win *cwin = data;

	mobj = pragha_playlist_get_selected_musicobject(cwin->cplaylist);

	msg_data = do_lastfm_unlove_mobj(mobj, cwin);

	return msg_data;
}

void lastfm_track_current_playlist_unlove_action (GtkAction *action, struct con_win *cwin)
{
	CDEBUG(DBG_LASTFM, "Unlove Handler to current playlist");

	if(cwin->clastfm->status != LASTFM_STATUS_OK) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	pragha_async_launch(do_lastfm_current_playlist_unlove,
			    set_async_finished_message,
			    cwin);
}

static gboolean
append_mobj_list_current_playlist_idle(gpointer user_data)
{
	gchar *summary = NULL;
	guint songs_added = 0;
	gint prev_tracks = 0;

	AddMusicObjectListData *data = user_data;

	GList *list = data->list;
	struct con_win *cwin = data->cwin;

	if(list == NULL)
		goto empty;

	prev_tracks = pragha_playlist_get_no_tracks(cwin->cplaylist);

	pragha_playlist_append_mobj_list(cwin->cplaylist,
					 list);

	songs_added = g_list_length(list);
	g_list_free(list);

empty:
	switch(data->query_type) {
		case LASTFM_GET_SIMILAR:
			if(data->query_count > 0)
				summary = g_strdup_printf(_("Added %d songs of %d sugested from Last.fm."),
							  songs_added, data->query_count);
			else
				summary = g_strdup_printf(_("Last.fm not suggest any similar song."));
			break;
		case LASTFM_GET_LOVED:
			if(data->query_count > 0)
				summary = g_strdup_printf(_("Added %d songs of the last %d loved on Last.fm."),
							  songs_added, data->query_count);
			else
				summary = g_strdup_printf(_("You had no favorite songs on Last.fm."));
			break;
		case LASTFM_NONE:
		default:
			break;
	}

	if(songs_added > 0)
		select_numered_path_of_current_playlist(cwin->cplaylist, prev_tracks, TRUE);

	if (summary != NULL) {
		set_status_message(summary, cwin);
		g_free(summary);
	}
	remove_watch_cursor (cwin->mainwindow);

	g_slice_free (AddMusicObjectListData, data);

	return FALSE;
}

gpointer
do_lastfm_get_similar(PraghaMusicobject *mobj, struct con_win *cwin)
{
	LFMList *results = NULL, *li;
	LASTFM_TRACK_INFO *track = NULL;
	guint query_count = 0;
	GList *list = NULL;
	gint rv;
	const gchar *title, *artist;

	AddMusicObjectListData *data;

	title = pragha_musicobject_get_title(mobj);
	artist = pragha_musicobject_get_artist(mobj);

	if(string_is_not_empty(title) && string_is_not_empty(artist)) {
		rv = LASTFM_track_get_similar(cwin->clastfm->session_id,
					      title,
					      artist,
					      50, &results);

		for(li=results; li && rv == LASTFM_STATUS_OK; li=li->next) {
			track = li->data;
			list = prepend_song_with_artist_and_title_to_mobj_list(track->artist, track->name, list, cwin);
			query_count += 1;
		}
	}

	data = g_slice_new (AddMusicObjectListData);
	data->list = list;
	data->query_type = LASTFM_GET_SIMILAR;
	data->query_count = query_count;
	data->cwin = cwin;

	LASTFM_free_track_info_list (results);

	return data;
}

gpointer
do_lastfm_get_similar_current_playlist_action (gpointer user_data)
{
	PraghaMusicobject *mobj = NULL;

	AddMusicObjectListData *data;

	struct con_win *cwin = user_data;

	mobj = pragha_playlist_get_selected_musicobject(cwin->cplaylist);

	data = do_lastfm_get_similar(mobj, cwin);

	return data;
}

void
lastfm_get_similar_current_playlist_action (GtkAction *action, struct con_win *cwin)
{
	CDEBUG(DBG_LASTFM, "Get similar action to current playlist");

	if(cwin->clastfm->session_id == NULL) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	set_watch_cursor (cwin->mainwindow);
	pragha_async_launch(do_lastfm_get_similar_current_playlist_action,
			    append_mobj_list_current_playlist_idle,
			    cwin);
}

/* Functions that respond to menu options. */

static void
lastfm_import_xspf_response(GtkDialog *dialog,
				gint response,
				struct con_win *cwin)
{
	XMLNode *xml = NULL, *xi, *xc, *xt;
	gchar *contents, *summary;
	gint try = 0, added = 0, prev_tracks = 0;
	GList *list = NULL;

	GFile *file;
	gsize size;

	if(response != GTK_RESPONSE_ACCEPT)
		goto cancel;

	file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));

	if (!g_file_load_contents (file, NULL, &contents, &size, NULL, NULL)) {
		goto out;
    	}

	if (g_utf8_validate (contents, -1, NULL) == FALSE) {
		gchar *fixed;
		fixed = g_convert (contents, -1, "UTF-8", "ISO8859-1", NULL, NULL, NULL);
		if (fixed != NULL) {
			g_free (contents);
			contents = fixed;
		}
	}

	set_watch_cursor (cwin->mainwindow);

	prev_tracks = pragha_playlist_get_no_tracks(cwin->cplaylist);

	xml = tinycxml_parse(contents);

	xi = xmlnode_get(xml,CCA { "playlist","trackList","track",NULL},NULL,NULL);
	for(;xi;xi= xi->next) {
		try++;
		xt = xmlnode_get(xi,CCA {"track","title",NULL},NULL,NULL);
		xc = xmlnode_get(xi,CCA {"track","creator",NULL},NULL,NULL);

		if (xt && xc)
			list = prepend_song_with_artist_and_title_to_mobj_list(xc->content, xt->content, list, cwin);
	}

	added = g_list_length(list);
	if(added > 0) {
		pragha_playlist_append_mobj_list(cwin->cplaylist,
						 list);
		select_numered_path_of_current_playlist(cwin->cplaylist, prev_tracks, TRUE);
	}

	remove_watch_cursor (cwin->mainwindow);

	summary = g_strdup_printf(_("Added %d songs from %d of the imported playlist."), added, try);

	set_status_message(summary, cwin);

	xmlnode_free(xml);
	g_list_free(list);
	g_free (contents);
	g_free(summary);
out:
	g_object_unref (file);
cancel:
	gtk_widget_destroy (GTK_WIDGET(dialog));
}

void
lastfm_import_xspf_action (GtkAction *action, struct con_win *cwin)
{
	GtkWidget *dialog;
	GtkFileFilter *media_filter;

	dialog = gtk_file_chooser_dialog_new (_("Import a XSPF playlist"),
				      GTK_WINDOW(cwin->mainwindow),
				      GTK_FILE_CHOOSER_ACTION_OPEN,
				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				      GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				      NULL);

	media_filter = gtk_file_filter_new();
	gtk_file_filter_set_name(GTK_FILE_FILTER(media_filter), _("Supported media"));
	gtk_file_filter_add_mime_type(GTK_FILE_FILTER(media_filter), "application/xspf+xml");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), GTK_FILE_FILTER(media_filter));

	g_signal_connect(G_OBJECT(dialog), "response",
			G_CALLBACK(lastfm_import_xspf_response), cwin);

	gtk_widget_show_all (dialog);
}

gpointer
do_lastfm_add_favorites_action (gpointer user_data)
{
	LFMList *results = NULL, *li;
	LASTFM_TRACK_INFO *track;
	gint rpages = 0, cpage = 0;
	AddMusicObjectListData *data;
	guint query_count = 0;
	GList *list = NULL;

	struct con_win *cwin = user_data;

	do {
		rpages = LASTFM_user_get_loved_tracks(cwin->clastfm->session_id,
						     cwin->cpref->lastfm_user,
						     cpage,
						     &results);

		for(li=results; li; li=li->next) {
			track = li->data;
			list = prepend_song_with_artist_and_title_to_mobj_list(track->artist, track->name, list, cwin);
			query_count += 1;
		}
		LASTFM_free_track_info_list (results);
		cpage++;
	} while(rpages != 0);

	data = g_slice_new (AddMusicObjectListData);
	data->list = list;
	data->query_type = LASTFM_GET_LOVED;
	data->query_count = query_count;
	data->cwin = cwin;

	return data;
}

void
lastfm_add_favorites_action (GtkAction *action, struct con_win *cwin)
{
	CDEBUG(DBG_LASTFM, "Add Favorites action");

	if ((cwin->clastfm->session_id == NULL) ||
	    string_is_empty(cwin->cpref->lastfm_user)) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	set_watch_cursor (cwin->mainwindow);
	pragha_async_launch(do_lastfm_add_favorites_action,
			    append_mobj_list_current_playlist_idle,
			    cwin);
}

gpointer
do_lastfm_get_similar_action (gpointer user_data)
{
	AddMusicObjectListData *data;

	struct con_win *cwin = user_data;

	data = do_lastfm_get_similar(cwin->cstate->curr_mobj, cwin);

	return data;
}

void
lastfm_get_similar_action (GtkAction *action, struct con_win *cwin)
{
	CDEBUG(DBG_LASTFM, "Get similar action");

	if(pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return;

	if(cwin->clastfm->session_id == NULL) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	set_watch_cursor (cwin->mainwindow);
	pragha_async_launch(do_lastfm_get_similar_action,
			    append_mobj_list_current_playlist_idle,
			    cwin);
}

gpointer
do_lastfm_current_song_love (gpointer data)
{
	AsycMessageData *msg_data = NULL;

	struct con_win *cwin = data;

	msg_data = do_lastfm_love_mobj(cwin->cstate->curr_mobj, cwin);

	return msg_data;
}

void
lastfm_track_love_action (GtkAction *action, struct con_win *cwin)
{
	CDEBUG(DBG_LASTFM, "Love Handler");

	if(pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return;

	if(cwin->clastfm->status != LASTFM_STATUS_OK) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	pragha_async_launch(do_lastfm_current_song_love,
			    set_async_finished_message,
			    cwin);
}

gpointer
do_lastfm_current_song_unlove (gpointer data)
{
	struct con_win *cwin = data;
	AsycMessageData *msg_data = NULL;

	msg_data = do_lastfm_unlove_mobj(cwin->cstate->curr_mobj, cwin);

	return msg_data;
}

void
lastfm_track_unlove_action (GtkAction *action, struct con_win *cwin)
{
	CDEBUG(DBG_LASTFM, "Unlove Handler");

	if(pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return;

	if(cwin->clastfm->status != LASTFM_STATUS_OK) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	pragha_async_launch(do_lastfm_current_song_unlove,
			    set_async_finished_message,
			    cwin);
}

gpointer
do_lastfm_scrob (gpointer data)
{
	gint rv;
	struct con_win *cwin = data;
	PraghaMusicobject *mobj = NULL;
	gchar *title = NULL, *artist = NULL, *album = NULL;
	gint track_no, length;
	AsycMessageData *msg_data;

	CDEBUG(DBG_LASTFM, "Scrobbler thread");

	mobj = cwin->cstate->curr_mobj;
	g_object_ref(mobj);
	g_object_get(mobj,
	             "title", &title,
	             "artist", &artist,
	             "album", &album,
	             "track-no", &track_no,
	             "length", &length,
	             NULL);

	rv = LASTFM_track_scrobble (cwin->clastfm->session_id,
	                            (char*)title,
	                            (char*)album,
	                            (char*)artist,
	                            cwin->clastfm->playback_started,
	                            length,
	                            track_no,
	                            0, NULL);

	msg_data = async_finished_message_new((rv != LASTFM_STATUS_OK) ?
					     _("Last.fm submission failed") :
					     _("Track scrobbled on Last.fm"),
					     cwin);

	g_object_unref(mobj);
	g_free(title);
	g_free(artist);
	g_free(album);

	return msg_data;
}

gboolean
lastfm_scrob_handler(gpointer data)
{
	struct con_win *cwin = data;

	CDEBUG(DBG_LASTFM, "Scrobbler Handler");

	if(pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return FALSE;

	if(cwin->clastfm->status != LASTFM_STATUS_OK) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return FALSE;
	}

	pragha_async_launch(do_lastfm_scrob,
			    set_async_finished_message,
			    cwin);
	return FALSE;
}

static gboolean
show_lastfm_sugest_corrrection_button (gpointer user_data)
{
	struct con_win *cwin = user_data;

	gtk_widget_show(cwin->ntag_lastfm_button);

	return FALSE;
}

gpointer
do_lastfm_now_playing (gpointer data)
{
	AsycMessageData *msg_data = NULL;
	PraghaMusicobject *omobj, *nmobj;
	const gchar *title, *artist, *album;
	gint track_no, length;
	LFMList *list = NULL;
	LASTFM_TRACK_INFO *ntrack = NULL;
	gint changed = 0, rv;

	struct con_win *cwin = data;

	CDEBUG(DBG_LASTFM, "Update now playing thread");

	g_object_ref(cwin->cstate->curr_mobj);
	omobj = cwin->cstate->curr_mobj;

	title = pragha_musicobject_get_title(omobj);
	artist = pragha_musicobject_get_artist(omobj);
	album = pragha_musicobject_get_album(omobj);
	length = pragha_musicobject_get_length(omobj);
	track_no = pragha_musicobject_get_track_no(omobj);

	rv = LASTFM_track_update_now_playing (cwin->clastfm->session_id,
	                                      (char*)title,
	                                      (char*)album,
	                                      (char*)artist,
	                                      length,
	                                      track_no,
	                                      0,
	                                      &list);

	g_object_ref(cwin->clastfm->nmobj);
	nmobj = cwin->clastfm->nmobj;

	if (rv == LASTFM_STATUS_OK) {
		/* Fist check lastfm response, and compare tags. */
		if(list != NULL) {
			ntrack = list->data;
			if(ntrack->name && g_ascii_strcasecmp(title, ntrack->name))
				changed |= TAG_TITLE_CHANGED;
			if(ntrack->artist && g_ascii_strcasecmp(artist, ntrack->artist))
				changed |= TAG_ARTIST_CHANGED;
			if(ntrack->album && g_ascii_strcasecmp(album, ntrack->album))
				changed |= TAG_ALBUM_CHANGED;
		}

		if (changed) {
			g_object_set (nmobj,
			              "file", pragha_musicobject_get_file(omobj),
			              "file-type", pragha_musicobject_get_file_type(omobj),
			              "title", (changed & TAG_TITLE_CHANGED) ? ntrack->name : title,
			              "artist", (changed & TAG_ARTIST_CHANGED) ? ntrack->artist : artist,
			              "album", (changed & TAG_ALBUM_CHANGED) ? ntrack->album : album,
			              "genre", pragha_musicobject_get_genre(omobj),
			              "comment", pragha_musicobject_get_comment(omobj),
			              "year", pragha_musicobject_get_year(omobj),
			              "track-no", track_no,
			              "length", length,
			              "bitrate", pragha_musicobject_get_bitrate(omobj),
			              "channels", pragha_musicobject_get_channels(omobj),
			              "samplerate", pragha_musicobject_get_samplerate(omobj),
			              NULL);

			g_idle_add (show_lastfm_sugest_corrrection_button, cwin);
		}
		else {
			pragha_musicobject_clean(nmobj);
		}
	}
	else {
		pragha_musicobject_clean(nmobj);
		msg_data = async_finished_message_new(_("Update current song on Last.fm failed."), cwin);
	}

	LASTFM_free_track_info_list(list);

	g_object_unref(omobj);
	g_object_unref(nmobj);

	return msg_data;
}

void
lastfm_now_playing_handler (struct con_win *cwin)
{
	gint length;

	CDEBUG(DBG_LASTFM, "Update now playing Handler");

	if(pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return;

	if(string_is_empty(cwin->cpref->lastfm_user) ||
	   string_is_empty(cwin->cpref->lastfm_pass))
		return;

	if(cwin->clastfm->status != LASTFM_STATUS_OK) {
		set_status_message(_("No connection Last.fm has been established."), cwin);
		return;
	}

	if (!pragha_musicobject_get_artist(cwin->cstate->curr_mobj) ||
	    !pragha_musicobject_get_title(cwin->cstate->curr_mobj))
		return;

	if(pragha_musicobject_get_length(cwin->cstate->curr_mobj) < 30)
		return;

	pragha_async_launch(do_lastfm_now_playing,
			    set_async_finished_message,
			    cwin);

	/* Kick the lastfm scrobbler on
	 * Note: Only scrob if tracks is more than 30s.
	 * and scrob when track is at 50% or 4mins, whichever comes
	 * first */

	if((pragha_musicobject_get_length(cwin->cstate->curr_mobj) / 2) > (240 - WAIT_UPDATE)) {
		length = 240 - WAIT_UPDATE;
	}
	else {
		length = (pragha_musicobject_get_length(cwin->cstate->curr_mobj) / 2) - WAIT_UPDATE;
	}

	cwin->related_timeout_id = g_timeout_add_seconds_full(
			G_PRIORITY_DEFAULT_IDLE, length,
			lastfm_scrob_handler, cwin, NULL);

	return;
}

/* Init lastfm with a simple thread when change preferences and show error messages. */

gboolean
do_just_init_lastfm(gpointer data)
{
	struct con_win *cwin = data;

	cwin->clastfm->session_id = LASTFM_init(LASTFM_API_KEY, LASTFM_SECRET);

	if (cwin->clastfm->session_id != NULL) {
		if(string_is_not_empty(cwin->cpref->lastfm_user) &&
		   string_is_not_empty(cwin->cpref->lastfm_pass)) {
			cwin->clastfm->status = LASTFM_login (cwin->clastfm->session_id,
							      cwin->cpref->lastfm_user,
							      cwin->cpref->lastfm_pass);

			if(cwin->clastfm->status != LASTFM_STATUS_OK) {
				CDEBUG(DBG_INFO, "Failure to login on lastfm");
				set_status_message(_("No connection Last.fm has been established."), cwin);
			}
		}
	}
	else {
		CDEBUG(DBG_INFO, "Failure to init libclastfm");
		set_status_message(_("No connection Last.fm has been established."), cwin);
	}
	update_menubar_lastfm_state (cwin);

	return FALSE;
}

gint
just_init_lastfm (struct con_win *cwin)
{
	if (cwin->cpref->lastfm_support) {
		CDEBUG(DBG_INFO, "Initializing LASTFM");
		g_idle_add (do_just_init_lastfm, cwin);
	}
	return 0;
}

/* When just launch pragha init lastfm immediately if has internet or otherwise waiting 30 seconds.
 * And no show any error. */

gboolean
do_init_lastfm_idle(gpointer data)
{
	struct con_win *cwin = data;

	cwin->clastfm->session_id = LASTFM_init(LASTFM_API_KEY, LASTFM_SECRET);

	if (cwin->clastfm->session_id != NULL) {
		if(string_is_not_empty(cwin->cpref->lastfm_user) &&
		   string_is_not_empty(cwin->cpref->lastfm_pass)) {
			cwin->clastfm->status = LASTFM_login (cwin->clastfm->session_id,
							      cwin->cpref->lastfm_user,
							      cwin->cpref->lastfm_pass);

			if(cwin->clastfm->status != LASTFM_STATUS_OK)
				CDEBUG(DBG_INFO, "Failure to login on lastfm");
		}
	}
	else {
		CDEBUG(DBG_INFO, "Failure to init libclastfm");
	}

	update_menubar_lastfm_state (cwin);

	return FALSE;
}

gint
init_lastfm(struct con_win *cwin)
{
	/* Init struct flags */

	cwin->clastfm->session_id = NULL;
	cwin->clastfm->status = LASTFM_STATUS_INVALID;
	cwin->clastfm->nmobj = pragha_musicobject_new();

	/* Test internet and launch threads.*/

	if (cwin->cpref->lastfm_support) {
		CDEBUG(DBG_INFO, "Initializing LASTFM");

#if GLIB_CHECK_VERSION(2,32,0)
		if (g_network_monitor_get_network_available (g_network_monitor_get_default ()))
#else
		if(nm_is_online () == TRUE)
#endif
			g_idle_add (do_init_lastfm_idle, cwin);
		else
			g_timeout_add_seconds_full(
					G_PRIORITY_DEFAULT_IDLE, 30,
					do_init_lastfm_idle, cwin, NULL);
	}

	return 0;
}

void
lastfm_free(struct con_lastfm *clastfm)
{
	if (clastfm->session_id)
		LASTFM_dinit(clastfm->session_id);

	g_object_unref(clastfm->nmobj);

	g_slice_free(struct con_lastfm, clastfm);
}
#endif
