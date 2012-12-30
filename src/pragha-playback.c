/*************************************************************************/
/* Copyright (C) 2010-2012 matias <mati86dl@gmail.com>			 */
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

/* Update gui, mpris on new track playback */

void
pragha_playback_notificate_new_track (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	struct con_win *cwin = user_data;

	enum player_state state = pragha_backend_get_state (cwin->backend);

	if(state != ST_PLAYING)
		return;

	/* New song playback. */
	if(pragha_playlist_get_current_update_action(cwin->cplaylist) != PLAYLIST_NONE) {
		CDEBUG(DBG_BACKEND, "Definitely play a new song: %s", pragha_musicobject_get_file(cwin->cstate->curr_mobj));

		/* Update current song info */
		__update_current_song_info(cwin);
		__update_progress_song_info(cwin, 0);

		/* Update and jump in current playlist */
		update_current_playlist_view_new_track(cwin);

		/* Update album art */
		update_album_art(cwin->cstate->curr_mobj, cwin);

		/* Show osd, and inform new album art. */
		show_osd(cwin);
		mpris_update_metadata_changed(cwin);

		pragha_playlist_report_finished_action(cwin->cplaylist);
	}
}

/**********************/
/* Playback functions */
/**********************/

/* Play prev track in current playlist */

void pragha_playback_prev_track(struct con_win *cwin)
{
	GtkTreePath *path;
	PraghaMusicobject *mobj = NULL;

	CDEBUG(DBG_BACKEND, "Want to play a song previously played");

	/* Get the next (prev) track to be played */
	path = current_playlist_get_prev(cwin->cplaylist);

	/* No more tracks */
	if (!path)
		return;

	/* Stop currently playing track */
	pragha_backend_stop(cwin->backend);

	/* Start playing new track */
	pragha_playlist_set_current_update_action(cwin->cplaylist, PLAYLIST_PREV);
	pragha_playlist_update_current_playlist_state(cwin->cplaylist, path);

	mobj = current_playlist_mobj_at_path(path, cwin->cplaylist);
	pragha_backend_start(cwin->backend, mobj);

	gtk_tree_path_free(path);
}

/* Start playback of a new track, or resume playback of current track */

void pragha_playback_play_pause_resume(struct con_win *cwin)
{
	PraghaMusicobject *mobj = NULL;
	GtkTreePath *path=NULL;

	CDEBUG(DBG_BACKEND, "Play pause or resume a track based on the current state");

	/* New action is based on the current state */

	/************************************/
        /* State     Action		    */
	/* 				    */
	/* Playing   Pause playback	    */
	/* Paused    Resume playback	    */
	/* Stopped   Start playback	    */
        /************************************/

	switch (pragha_backend_get_state (cwin->backend)) {
	case ST_PLAYING:
		pragha_backend_pause(cwin->backend);
		break;
	case ST_PAUSED:
		pragha_backend_resume(cwin->backend);
		break;
	case ST_STOPPED:
		if(pragha_playlist_is_changing(cwin->cplaylist))
			break;
		if(pragha_playlist_has_queue(cwin->cplaylist))
			path = get_next_queue_track(cwin->cplaylist);
		if (!path)
			path = current_playlist_get_selection(cwin->cplaylist);

		if(!path) {
			if(pragha_preferences_get_shuffle(cwin->preferences))
				path = get_first_random_track(cwin->cplaylist);
			else
				path = gtk_tree_path_new_first();
		}

		if (pragha_preferences_get_shuffle(cwin->preferences))
			pragha_playlist_set_first_rand_ref(cwin->cplaylist, path);

		pragha_playlist_set_current_update_action(cwin->cplaylist, PLAYLIST_CURR);
		pragha_playlist_update_current_playlist_state(cwin->cplaylist, path);

		mobj = current_playlist_mobj_at_path(path, cwin->cplaylist);
		pragha_backend_start(cwin->backend, mobj);
		gtk_tree_path_free(path);
		break;
	default:
		break;
	}
}

/* Stop the playback */

void pragha_playback_stop(struct con_win *cwin)
{
	CDEBUG(DBG_BACKEND, "Stopping the current song");

	if (pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return;

	pragha_backend_stop(cwin->backend);
}

/* Play next song when terminate a song. */

void pragha_advance_playback (struct con_win *cwin)
{
	GtkTreePath *path = NULL;
	PraghaMusicobject *mobj = NULL;

	CDEBUG(DBG_BACKEND, "Advancing to next track");

	/* Stop to set ready and clear all info */
	pragha_backend_stop(cwin->backend);

	if(pragha_playlist_is_changing(cwin->cplaylist))
		return;

	/* Get the next track to be played */
	path = current_playlist_get_next (cwin->cplaylist);

	/* No more tracks */
	if (!path)
		return;

	/* Start playing new track */
	pragha_playlist_set_current_update_action(cwin->cplaylist, PLAYLIST_NEXT);
	pragha_playlist_update_current_playlist_state(cwin->cplaylist, path);

	mobj = current_playlist_mobj_at_path (path, cwin->cplaylist);
	pragha_backend_start (cwin->backend, mobj);

	gtk_tree_path_free (path);
}

/* Play next track in current_playlist */

void pragha_playback_next_track(struct con_win *cwin)
{
	CDEBUG(DBG_BACKEND, "Want to advancing to next track");

	/* Are we playing right now ? */
	if (pragha_backend_get_state (cwin->backend) == ST_STOPPED)
		return;

	/* Play a new song */
	pragha_advance_playback(cwin);
}
