/*************************************************************************/
/* Copyright (C) 2007-2009 sujith <m.sujith@gmail.com>                   */
/* Copyright (C) 2009-2013 matias <mati86dl@gmail.com>                   */
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

#ifndef PRAGHA_PLAYLIST_H
#define PRAGHA_PLAYLIST_H

#include <gtk/gtk.h>
#include "pragha-backend.h"
#include "pragha-database.h"

/* pragha.h */
struct con_win;

typedef struct _PraghaPlaylist PraghaPlaylist;

/* Columns in current playlist view */

#define P_TRACK_NO_STR      "#"
#define P_TNO_FULL_STR      N_("Track No")
#define P_TITLE_STR         N_("Title")
#define P_ARTIST_STR        N_("Artist")
#define P_ALBUM_STR         N_("Album")
#define P_GENRE_STR         N_("Genre")
#define P_BITRATE_STR       N_("Bitrate")
#define P_YEAR_STR          N_("Year")
#define P_COMMENT_STR       N_("Comment")
#define P_LENGTH_STR        N_("Length")
#define P_FILENAME_STR      N_("Filename")

enum curplaylist_columns {
	P_MOBJ_PTR,
	P_QUEUE,
	P_BUBBLE,
	P_STATUS_PIXBUF,
	P_TRACK_NO,
	P_TITLE,
	P_ARTIST,
	P_ALBUM,
	P_GENRE,
	P_BITRATE,
	P_YEAR,
	P_COMMENT,
	P_LENGTH,
	P_FILENAME,
	P_PLAYED,
	N_P_COLUMNS
};

/* Current playlist movement */

enum playlist_action {
	PLAYLIST_NONE,
	PLAYLIST_CURR,
	PLAYLIST_NEXT,
	PLAYLIST_PREV
};

void jump_to_path_on_current_playlist(PraghaPlaylist *cplaylist, GtkTreePath *path, gboolean center);
void select_numered_path_of_current_playlist(PraghaPlaylist *cplaylist, gint path_number, gboolean center);
void pragha_playlist_update_statusbar_playtime(PraghaPlaylist *cplaylist);
enum playlist_action pragha_playlist_get_current_update_action(PraghaPlaylist* cplaylist);
void pragha_playlist_report_finished_action(PraghaPlaylist* cplaylist);
void pragha_playlist_set_current_update_action(PraghaPlaylist* cplaylist, enum playlist_action action);
void pragha_playlist_update_current_playlist_state(PraghaPlaylist* cplaylist, GtkTreePath *path);
void update_current_playlist_view_new_track(PraghaPlaylist *cplaylist, PraghaBackend *backend);
void update_current_playlist_view_track(PraghaPlaylist *cplaylist, PraghaBackend *backend);
PraghaMusicobject * current_playlist_mobj_at_path(GtkTreePath *path,
						  PraghaPlaylist *cplaylist);
GtkTreePath* current_playlist_path_at_mobj(PraghaMusicobject *mobj,
					   PraghaPlaylist *cplaylist);
void
pragha_playlist_set_first_rand_ref(PraghaPlaylist *cplaylist, GtkTreePath *path);
GtkTreePath* current_playlist_get_selection(PraghaPlaylist *cplaylist);
GtkTreePath* current_playlist_get_next(PraghaPlaylist *cplaylist);
GtkTreePath* current_playlist_get_prev(PraghaPlaylist *cplaylist);
GtkTreePath* current_playlist_get_actual(PraghaPlaylist *cplaylist);
GtkTreePath* get_first_random_track(PraghaPlaylist *cplaylist);
GtkTreePath* current_playlist_nth_track(gint n, PraghaPlaylist *cplaylist);
GtkTreePath* get_next_queue_track(PraghaPlaylist *cplaylist);
gchar* get_ref_current_track(struct con_win *cwin);
void dequeue_current_playlist(GtkAction *action, struct con_win *cwin);
void queue_current_playlist(GtkAction *action, struct con_win *cwin);
void toggle_queue_selected_current_playlist (PraghaPlaylist *cplaylist);
void remove_from_playlist(GtkAction *action, struct con_win *cwin);
void crop_current_playlist(GtkAction *action, struct con_win *cwin);
void edit_tags_playing_action(GtkAction *action, struct con_win *cwin);
void pragha_playlist_remove_all (PraghaPlaylist *cplaylist);
void current_playlist_clear_action(GtkAction *action, struct con_win *cwin);
gboolean pragha_playlist_update_ref_list_change_tags(PraghaPlaylist *cplaylist, GList *list, gint changed, PraghaMusicobject *nmobj);
void pragha_playlist_update_current_track(PraghaPlaylist *cplaylist, gint changed, PraghaMusicobject *nmobj);
void append_current_playlist(PraghaPlaylist *cplaylist, PraghaMusicobject *mobj);
void
pragha_playlist_append_single_song(PraghaPlaylist *cplaylist, PraghaMusicobject *mobj);
void
pragha_playlist_append_mobj_and_play(PraghaPlaylist *cplaylist, PraghaMusicobject *mobj);
void
pragha_playlist_append_mobj_list(PraghaPlaylist *cplaylist, GList *list);
gboolean
pragha_mobj_list_already_has_title_of_artist(GList *list,
					     const gchar *title,
					     const gchar *artist);
gboolean
pragha_playlist_already_has_title_of_artist(PraghaPlaylist *cplaylist,
					    const gchar *title,
					    const gchar *artist);
void clear_sort_current_playlist(GtkAction *action, PraghaPlaylist *cplaylist);
void save_selected_playlist(GtkAction *action, PraghaPlaylist *cplaylist);
void save_current_playlist(GtkAction *action, PraghaPlaylist *cplaylist);
void export_current_playlist(GtkAction *action, PraghaPlaylist *cplaylist);
void export_selected_playlist(GtkAction *action, PraghaPlaylist *cplaylist);
void jump_to_playing_song(struct con_win *cwin);
void copy_tags_to_selection_action(GtkAction *action, struct con_win *cwin);
GList *pragha_playlist_get_mobj_list(PraghaPlaylist* cplaylist);
GList *pragha_playlist_get_selection_mobj_list(PraghaPlaylist* cplaylist);
GList *pragha_playlist_get_selection_ref_list(PraghaPlaylist *cplaylist);
PraghaMusicobject *pragha_playlist_get_selected_musicobject(PraghaPlaylist* cplaylist);
void save_current_playlist_state(PraghaPlaylist *cplaylist);
void init_current_playlist_view(PraghaPlaylist *cplaylist);
void playlist_track_column_change_cb(GtkCheckMenuItem *item,
				     PraghaPlaylist* cplaylist);
void playlist_title_column_change_cb(GtkCheckMenuItem *item,
				     PraghaPlaylist* cplaylist);
void playlist_artist_column_change_cb(GtkCheckMenuItem *item,
				      PraghaPlaylist* cplaylist);
void playlist_album_column_change_cb(GtkCheckMenuItem *item,
				     PraghaPlaylist* cplaylist);
void playlist_genre_column_change_cb(GtkCheckMenuItem *item,
				     PraghaPlaylist* cplaylist);
void playlist_bitrate_column_change_cb(GtkCheckMenuItem *item,
				       PraghaPlaylist* cplaylist);
void playlist_year_column_change_cb(GtkCheckMenuItem *item,
				    PraghaPlaylist* cplaylist);
void playlist_length_column_change_cb(GtkCheckMenuItem *item,
				      PraghaPlaylist* cplaylist);
void playlist_comment_column_change_cb(GtkCheckMenuItem *item,
				     PraghaPlaylist* cplaylist);
void playlist_filename_column_change_cb(GtkCheckMenuItem *item,
					PraghaPlaylist* cplaylist);
void clear_sort_current_playlist_cb(GtkMenuItem *item,
				    PraghaPlaylist *cplaylist);
gint compare_track_no(GtkTreeModel *model, GtkTreeIter *a,
		      GtkTreeIter *b, gpointer data);
gint compare_bitrate(GtkTreeModel *model, GtkTreeIter *a,
		     GtkTreeIter *b, gpointer data);
gint compare_year(GtkTreeModel *model, GtkTreeIter *a,
		  GtkTreeIter *b, gpointer data);
gint compare_length(GtkTreeModel *model, GtkTreeIter *a,
		    GtkTreeIter *b, gpointer data);
gboolean pragha_playlist_propagate_event(PraghaPlaylist* cplaylist, GdkEventKey *event);

void pragha_playlist_activate_path        (PraghaPlaylist* cplaylist, GtkTreePath *path);
void pragha_playlist_activate_unique_mobj (PraghaPlaylist* cplaylist, PraghaMusicobject *mobj);

gint pragha_playlist_get_no_tracks(PraghaPlaylist* cplaylist);

gboolean pragha_playlist_has_queue(PraghaPlaylist* cplaylist);

gboolean pragha_playlist_is_changing (PraghaPlaylist* cplaylist);
void     pragha_playlist_set_changing (PraghaPlaylist* cplaylist, gboolean changing);

GtkWidget    *pragha_playlist_get_view  (PraghaPlaylist* cplaylist);
GtkTreeModel *pragha_playlist_get_model (PraghaPlaylist* cplaylist);

GtkWidget      *pragha_playlist_get_widget (PraghaPlaylist* cplaylist);
GtkUIManager   *pragha_playlist_get_context_menu(PraghaPlaylist* cplaylist);
PraghaDatabase *pragha_playlist_get_database(PraghaPlaylist* cplaylist);

void pragha_playlist_save_preferences (PraghaPlaylist* cplaylist);

void            pragha_playlist_free (PraghaPlaylist *cplaylist);
PraghaPlaylist *pragha_playlist_new  (struct con_win *cwin);


#endif /* PRAGHA_PLAYLIST_H */