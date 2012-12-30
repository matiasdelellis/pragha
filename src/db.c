/*************************************************************************/
/* Copyright (C) 2007-2009 sujith <m.sujith@gmail.com>			 */
/* Copyright (C) 2009-2010 matias <mati86dl@gmail.com>			 */
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

struct con_dbase {
	sqlite3 *db;	/* SQLITE3 handle of the opened DB */
};

static void add_new_track_db(gint location_id,
			     gint artist_id,
			     gint album_id,
			     gint genre_id,
			     gint year_id,
			     gint comment_id,
			     guint track_no,
			     gint length,
			     gint channels,
			     gint bitrate,
			     gint samplerate,
			     gint file_type,
			     gchar *title,
			     struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("INSERT INTO TRACK ("
				"location, "
				"track_no, "
				"artist, "
				"album, "
				"genre, "
				"year, "
				"comment, "
				"bitrate, "
				"samplerate, "
				"length, "
				"channels, "
				"file_type, "
				"title) "
				"VALUES "
				"('%d', '%d', '%d', '%d', '%d', '%d', '%d', "
				"'%d', '%d', '%d', %d, '%d', '%s')",
				location_id,
				track_no,
				artist_id,
				album_id,
				genre_id,
				year_id,
				comment_id,
				bitrate,
				samplerate,
				length,
				channels,
				file_type,
				title);
	exec_sqlite_query(query, cdbase, NULL);
}

static void import_playlist_from_file_db(const gchar *playlist_file, struct con_dbase *cdbase)
{
	gchar *s_playlist, *playlist = NULL, *s_file;
	gint playlist_id = 0;
	GSList *list = NULL, *i = NULL;

	playlist = get_display_filename(playlist_file, FALSE);

	s_playlist = sanitize_string_to_sqlite3(playlist);

	if (find_playlist_db(s_playlist, cdbase))
		goto bad;

	playlist_id = add_new_playlist_db(s_playlist, cdbase);

#ifdef HAVE_PLPARSER
	gchar *uri = g_filename_to_uri (playlist_file, NULL, NULL);
	list = pragha_totem_pl_parser_parse_from_uri(uri);
	g_free (uri);
#else
	list = pragha_pl_parser_parse_from_file_by_extension (playlist_file);
#endif

	if(list) {
		for (i=list; i != NULL; i = i->next) {
			s_file = sanitize_string_to_sqlite3(i->data);
			add_track_playlist_db(s_file, playlist_id, cdbase);
			g_free(s_file);
			g_free(i->data);
		}
		g_slist_free(list);
	}

bad:
	g_free(s_playlist);
	g_free(playlist);
}

static void add_new_musicobject_from_file_db(const gchar *file, struct con_dbase *cdbase)
{
	PraghaMusicobject *mobj;
	gchar *sfile, *stitle, *sartist, *salbum, *sgenre, *scomment;
	gint location_id = 0, artist_id = 0, album_id = 0, genre_id = 0, year_id = 0, comment_id;

	mobj = new_musicobject_from_file(file);
	if (mobj) {
		sfile = sanitize_string_to_sqlite3(file);
		stitle = sanitize_string_to_sqlite3(pragha_musicobject_get_title(mobj));
		sartist = sanitize_string_to_sqlite3(pragha_musicobject_get_artist(mobj));
		salbum = sanitize_string_to_sqlite3(pragha_musicobject_get_album(mobj));
		sgenre = sanitize_string_to_sqlite3(pragha_musicobject_get_genre(mobj));
		scomment = sanitize_string_to_sqlite3(pragha_musicobject_get_comment(mobj));

		/* Write location */

		if ((location_id = find_location_db(sfile, cdbase)) == 0)
			location_id = add_new_location_db(sfile, cdbase);

		/* Write artist */

		if ((artist_id = find_artist_db(sartist, cdbase)) == 0)
			artist_id = add_new_artist_db(sartist, cdbase);

		/* Write album */

		if ((album_id = find_album_db(salbum, cdbase)) == 0)
			album_id = add_new_album_db(salbum, cdbase);

		/* Write genre */

		if ((genre_id = find_genre_db(sgenre, cdbase)) == 0)
			genre_id = add_new_genre_db(sgenre, cdbase);

		/* Write year */

		if ((year_id = find_year_db(pragha_musicobject_get_year(mobj), cdbase)) == 0)
			year_id = add_new_year_db(pragha_musicobject_get_year(mobj), cdbase);

		/* Write comment */

		if ((comment_id = find_comment_db(scomment, cdbase)) == 0)
			comment_id = add_new_comment_db(scomment, cdbase);

		/* Write track */

		add_new_track_db(location_id,
				 artist_id,
				 album_id,
				 genre_id,
				 year_id,
				 comment_id,
				 pragha_musicobject_get_track_no(mobj),
				 pragha_musicobject_get_length(mobj),
				 pragha_musicobject_get_channels(mobj),
				 pragha_musicobject_get_bitrate(mobj),
				 pragha_musicobject_get_samplerate(mobj),
				 pragha_musicobject_get_file_type(mobj),
				 stitle,
				 cdbase);

		g_free(sfile);
		g_free(stitle);
		g_free(sartist);
		g_free(salbum);
		g_free(sgenre);
		g_free(scomment);

		g_object_unref(mobj);
	}
}

static void add_entry_db(const gchar *file, struct con_dbase *cdbase)
{
	if (pragha_pl_parser_guess_format_from_extension(file) != PL_FORMAT_UNKNOWN) {
		import_playlist_from_file_db(file, cdbase);
	}
	else {
		add_new_musicobject_from_file_db(file, cdbase);
	}
}

static void delete_track_db(const gchar *file, struct con_dbase *cdbase)
{
	gchar *query, *sfile;
	gint location_id;
	struct db_result result;

	sfile = sanitize_string_to_sqlite3(file);

	query = g_strdup_printf("SELECT id FROM LOCATION WHERE name = '%s';", sfile);
	exec_sqlite_query(query, cdbase, &result);
	if (!result.no_rows) {
		g_warning("File not present in DB: %s", sfile);
		goto bad;
	}

	location_id = atoi(result.resultp[result.no_columns]);
	query = g_strdup_printf("DELETE FROM TRACK WHERE location = %d;", location_id);
	exec_sqlite_query(query, cdbase, NULL);
bad:
	g_free(sfile);
}

/**************/
/* Public API */
/**************/

/* NB: All of the add_* functions require sanitized strings */

gint add_new_artist_db(const gchar *artist, struct con_dbase *cdbase)
{
	gchar *query;
	gint artist_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO ARTIST (name) VALUES ('%s')",
				artist);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM ARTIST WHERE name = '%s'",
				artist);
	if (exec_sqlite_query(query, cdbase, &result)) {
		artist_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return artist_id;
}

gint add_new_album_db(const gchar *album, struct con_dbase *cdbase)
{
	gchar *query;
	gint album_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO ALBUM (name) VALUES ('%s')",
				album);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM ALBUM WHERE name = '%s'",
				album);
	if (exec_sqlite_query(query, cdbase, &result)) {
		album_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return album_id;
}

gint add_new_genre_db(const gchar *genre, struct con_dbase *cdbase)
{
	gchar *query;
	gint genre_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO GENRE (name) VALUES ('%s')",
				genre);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM GENRE WHERE name = '%s'",
				genre);
	if (exec_sqlite_query(query, cdbase, &result)) {
		genre_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return genre_id;
}

gint add_new_year_db(guint year, struct con_dbase *cdbase)
{
	gchar *query;
	gint year_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO YEAR (year) VALUES ('%d')",
				year);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM YEAR WHERE year = '%d'",
				year);
	if (exec_sqlite_query(query, cdbase, &result)) {
		year_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return year_id;
}

gint add_new_comment_db(const gchar *comment, struct con_dbase *cdbase)
{
	gchar *query;
	gint comment_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO COMMENT (name) VALUES ('%s')",
				comment);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM COMMENT WHERE name = '%s'",
				comment);
	if (exec_sqlite_query(query, cdbase, &result)) {
		comment_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return comment_id;
}

gint add_new_location_db(const gchar *location, struct con_dbase *cdbase)
{
	gchar *query;
	gint location_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO LOCATION (name) VALUES ('%s')",
				location);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM LOCATION WHERE name = '%s'",
				location);
	if (exec_sqlite_query(query, cdbase, &result)) {
		location_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return location_id;
}

void add_track_playlist_db(const gchar *file, gint playlist_id, struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("INSERT INTO PLAYLIST_TRACKS (file, playlist) "
				"VALUES ('%s', %d);",
				file,
				playlist_id);
	exec_sqlite_query(query, cdbase, NULL);
}

void add_track_radio_db(const gchar *uri, gint radio_id, struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("INSERT INTO RADIO_TRACKS (uri, radio) "
				"VALUES ('%s', %d);",
				uri,
				radio_id);
	exec_sqlite_query(query, cdbase, NULL);
}

/* NB: All of the find_* functions require sanitized strings. */

gint find_artist_db(const gchar *artist, struct con_dbase *cdbase)
{
	gint artist_id = 0;
	gchar *query;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM ARTIST WHERE name = '%s';", artist);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if(result.no_rows)
			artist_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return artist_id;
}

gint find_album_db(const gchar *album, struct con_dbase *cdbase)
{
	gint album_id = 0;
	gchar *query;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM ALBUM WHERE name = '%s';", album);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_rows)
			album_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return album_id;
}

gint find_genre_db(const gchar *genre, struct con_dbase *cdbase)
{
	gint genre_id = 0;
	gchar *query;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM GENRE WHERE name = '%s';", genre);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_rows)
			genre_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return genre_id;
}

gint find_year_db(gint year, struct con_dbase *cdbase)
{
	gint year_id = 0;
	gchar *query;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM YEAR WHERE year = '%d';", year);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_rows)
			year_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return year_id;
}

gint find_comment_db(const gchar *comment, struct con_dbase *cdbase)
{
	gint comment_id = 0;
	gchar *query;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM COMMENT WHERE name = '%s';", comment);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_rows)
			comment_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return comment_id;
}

gint find_location_db(const gchar *location, struct con_dbase *cdbase)
{
	gchar *query;
	gint location_id = 0;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM LOCATION WHERE name = '%s'",
				location);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_columns)
			location_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return location_id;
}

gint find_playlist_db(const gchar *playlist, struct con_dbase *cdbase)
{
	gchar *query;
	gint playlist_id = 0;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM PLAYLIST WHERE name = '%s'",
				playlist);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_columns)
			playlist_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return playlist_id;
}

gint find_radio_db(const gchar *radio, struct con_dbase *cdbase)
{
	gchar *query;
	gint radio_id = 0;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM RADIO WHERE name = '%s'",
				radio);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_columns)
			radio_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return radio_id;
}

void delete_location_db(gint location_id, struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("DELETE FROM TRACK WHERE location = %d;", location_id);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM LOCATION WHERE id = %d;", location_id);
	exec_sqlite_query(query, cdbase, NULL);
}

gint delete_location_hdd(gint location_id, struct con_dbase *cdbase)
{
	gint ret = 0;
	gchar *query, *file;
	struct db_result result;

	query = g_strdup_printf("SELECT name FROM LOCATION WHERE id = %d;", location_id);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_columns) {
			file = result.resultp[result.no_columns];
			ret = g_unlink(file);
			if (ret != 0)
				g_warning("%s", strerror(ret));
			else
				CDEBUG(DBG_VERBOSE, "Deleted file: %s", file);
		}
		sqlite3_free_table(result.resultp);
	} else {
		g_warning("Unable to find filename for location id: %d", location_id);
		ret = -1;
	}

	return ret;
}

/* Arg. title has to be sanitized */

void update_track_db(gint location_id, gint changed,
		     gint track_no, const gchar *title,
		     gint artist_id, gint album_id, gint genre_id, gint year_id, gint comment_id,
		     struct con_dbase *cdbase)
{
	gchar *query = NULL;

	if (changed & TAG_TNO_CHANGED) {
		query = g_strdup_printf("UPDATE TRACK SET track_no = '%d' "
					"WHERE LOCATION = '%d';",
					track_no, location_id);
		exec_sqlite_query(query, cdbase, NULL);

	}
	if (changed & TAG_TITLE_CHANGED) {
		query = g_strdup_printf("UPDATE TRACK SET title = '%s' "
					"WHERE LOCATION = '%d';",
					title, location_id);
		exec_sqlite_query(query, cdbase, NULL);
	}
	if (changed & TAG_ARTIST_CHANGED) {
		query = g_strdup_printf("UPDATE TRACK SET artist = '%d' "
					"WHERE LOCATION = '%d';",
					artist_id, location_id);
		exec_sqlite_query(query, cdbase, NULL);
	}
	if (changed & TAG_ALBUM_CHANGED) {
		query = g_strdup_printf("UPDATE TRACK SET album = '%d' "
					"WHERE LOCATION = '%d';",
					album_id, location_id);
		exec_sqlite_query(query, cdbase, NULL);
	}
	if (changed & TAG_GENRE_CHANGED) {
		query = g_strdup_printf("UPDATE TRACK SET genre = '%d' "
					"WHERE LOCATION = '%d';",
					genre_id, location_id);
		exec_sqlite_query(query, cdbase, NULL);
	}
	if (changed & TAG_YEAR_CHANGED) {
		query = g_strdup_printf("UPDATE TRACK SET year = '%d' "
					"WHERE LOCATION = '%d';",
					year_id, location_id);
		exec_sqlite_query(query, cdbase, NULL);
	}
	if (changed & TAG_COMMENT_CHANGED) {
		query = g_strdup_printf("UPDATE TRACK SET comment = '%d' "
					"WHERE LOCATION = '%d';",
					comment_id, location_id);
		exec_sqlite_query(query, cdbase, NULL);
	}
}

void
pragha_db_update_local_files_change_tag(struct con_dbase *cdbase, GArray *loc_arr, gint changed, PraghaMusicobject *mobj)
{
	gchar *stitle = NULL, *sartist = NULL, *scomment= NULL, *salbum = NULL, *sgenre = NULL;
	gint track_no = 0, artist_id = 0, album_id = 0, genre_id = 0, year_id = 0, comment_id = 0;
	guint i = 0, elem = 0;

	if (!changed)
		return;

	if (!loc_arr)
		return;

	CDEBUG(DBG_VERBOSE, "Tags Changed: 0x%x", changed);

	if (changed & TAG_TNO_CHANGED) {
		track_no = pragha_musicobject_get_track_no(mobj);
	}
	if (changed & TAG_TITLE_CHANGED) {
		stitle = sanitize_string_to_sqlite3(pragha_musicobject_get_title(mobj));
	}
	if (changed & TAG_ARTIST_CHANGED) {
		sartist = sanitize_string_to_sqlite3(pragha_musicobject_get_artist(mobj));
		artist_id = find_artist_db(sartist, cdbase);
		if (!artist_id)
			artist_id = add_new_artist_db(sartist, cdbase);
	}
	if (changed & TAG_ALBUM_CHANGED) {
		salbum = sanitize_string_to_sqlite3(pragha_musicobject_get_album(mobj));
		album_id = find_album_db(salbum, cdbase);
		if (!album_id)
			album_id = add_new_album_db(salbum, cdbase);
	}
	if (changed & TAG_GENRE_CHANGED) {
		sgenre = sanitize_string_to_sqlite3(pragha_musicobject_get_genre(mobj));
		genre_id = find_genre_db(sgenre, cdbase);
		if (!genre_id)
			genre_id = add_new_genre_db(sgenre, cdbase);
	}
	if (changed & TAG_YEAR_CHANGED) {
		year_id = find_year_db(pragha_musicobject_get_year(mobj), cdbase);
		if (!year_id)
			year_id = add_new_year_db(pragha_musicobject_get_year(mobj), cdbase);
	}
	if (changed & TAG_COMMENT_CHANGED) {
		scomment = sanitize_string_to_sqlite3(pragha_musicobject_get_comment(mobj));
		comment_id = find_comment_db(scomment, cdbase);
		if (!comment_id)
			comment_id = add_new_comment_db(scomment, cdbase);
	}

	db_begin_transaction(cdbase);
	if (loc_arr) {
		elem = 0;
		for(i = 0; i < loc_arr->len; i++) {
			elem = g_array_index(loc_arr, gint, i);
			if (elem) {
				update_track_db(elem, changed,
						track_no,
						stitle,
						artist_id,
						album_id,
						genre_id,
						year_id,
						comment_id,
						cdbase);
			}
		}
	}
	db_commit_transaction(cdbase);

	g_free(stitle);
	g_free(sartist);
	g_free(salbum);
	g_free(sgenre);
	g_free(scomment);
}

/* 'playlist' has to be a sanitized string */

void update_playlist_name_db(const gchar *oplaylist, gchar *nplaylist, struct con_dbase *cdbase)
{
	gchar *query;
	gint playlist_id = 0;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM PLAYLIST WHERE name = '%s'",
				oplaylist);

	if (exec_sqlite_query(query, cdbase, &result)) {
		playlist_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	if(playlist_id != 0) {
		query = g_strdup_printf("UPDATE PLAYLIST SET name = '%s' "
					"WHERE id = '%d';",
					nplaylist, playlist_id);

		exec_sqlite_query(query, cdbase, &result);
	}

}


gint add_new_playlist_db(const gchar *playlist, struct con_dbase *cdbase)
{
	gchar *query;
	gint playlist_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO PLAYLIST (name) VALUES ('%s')",
				playlist);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM PLAYLIST WHERE name = '%s'",
				playlist);
	if (exec_sqlite_query(query, cdbase, &result)) {
		playlist_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return playlist_id;
}

/* Get the names of all the playlists stored in the DB.
   Returned NULL terminated array of strings that has to freed by caller. */

gchar** get_playlist_names_db(struct con_dbase *cdbase)
{
	gchar *query;
	struct db_result result;
	gchar **playlists = NULL;
	gint i, j=0;

	query = g_strdup_printf("SELECT NAME FROM PLAYLIST WHERE NAME != \"%s\";",
				SAVE_PLAYLIST_STATE);
	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_rows) {
			playlists = g_malloc0((result.no_rows+1) * sizeof(gchar *));
			for_each_result_row(result, i) {
				playlists[j] = g_strdup(result.resultp[i]);
				j++;
			}
			playlists[j] = NULL;
		}
		sqlite3_free_table(result.resultp);
	}

	return playlists;
}

/* Get the number of all the playlists stored in the DB. */

gint get_playlist_count_db(struct con_dbase *cdbase)
{
	gchar *query;
	struct db_result result;
	gint n_playlists = 0;

	query = g_strdup_printf("SELECT COUNT() FROM PLAYLIST WHERE NAME != \"%s\";",
				SAVE_PLAYLIST_STATE);
	if (exec_sqlite_query(query, cdbase, &result)) {
		n_playlists = atoi(result.resultp[1]);
		sqlite3_free_table(result.resultp);
	}

	return n_playlists;
}

/* Get the number of all trackslist tracks currently in the DB. */

gint get_tracklist_count_db(struct con_dbase *cdbase)
{
	gchar *query;
	struct db_result result;
	/* this ID should be cached during open */
	gint playlist_id = find_playlist_db(SAVE_PLAYLIST_STATE, cdbase);
	gint n_playlists = 0;
	query = g_strdup_printf("SELECT COUNT() FROM PLAYLIST_TRACKS WHERE PLAYLIST=%d;", playlist_id);
	if (exec_sqlite_query(query, cdbase, &result)) {
		n_playlists = atoi(result.resultp[1]);
		sqlite3_free_table(result.resultp);
	}

	return n_playlists;
}
/* 'playlist' has to be a sanitized string */

void delete_playlist_db(const gchar *playlist, struct con_dbase *cdbase)
{
	gint playlist_id;
	gchar *query;

	if (string_is_empty(playlist)) {
		g_warning("Playlist name is NULL");
		return;
	}

	playlist_id = find_playlist_db(playlist, cdbase);

	if (!playlist_id) {
		g_warning("Playlist doesn't exist");
		return;
	}

	query = g_strdup_printf("DELETE FROM PLAYLIST_TRACKS WHERE PLAYLIST=%d;",
				playlist_id);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM PLAYLIST WHERE ID=%d;",
				playlist_id);
	exec_sqlite_query(query, cdbase, NULL);
}

/* Flushes all the tracks in a given playlist */

void flush_playlist_db(gint playlist_id, struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("DELETE FROM PLAYLIST_TRACKS WHERE PLAYLIST=%d;",
				playlist_id);
	exec_sqlite_query(query, cdbase, NULL);
}

/* 'radio' has to be a sanitized string */

void update_radio_name_db(const gchar *oradio, gchar *nradio, struct con_dbase *cdbase)
{
	gchar *query;
	gint radio_id = 0;
	struct db_result result;

	query = g_strdup_printf("SELECT id FROM RADIO WHERE name = '%s'",
				oradio);

	if (exec_sqlite_query(query, cdbase, &result)) {
		radio_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	if(radio_id != 0) {
		query = g_strdup_printf("UPDATE RADIO SET name = '%s' "
					"WHERE id = '%d';",
					nradio, radio_id);

		exec_sqlite_query(query, cdbase, &result);
	}

}


gint add_new_radio_db(const gchar *radio, struct con_dbase *cdbase)
{
	gchar *query;
	gint radio_id = 0;
	struct db_result result;

	query = g_strdup_printf("INSERT INTO RADIO (name) VALUES ('%s')",
				radio);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("SELECT id FROM RADIO WHERE name = '%s'",
				radio);
	if (exec_sqlite_query(query, cdbase, &result)) {
		radio_id = atoi(result.resultp[result.no_columns]);
		sqlite3_free_table(result.resultp);
	}

	return radio_id;
}

/* Get the names of all the radio stored in the DB.
   Returned NULL terminated array of strings that has to freed by caller. */

gchar** get_radio_names_db(struct con_dbase *cdbase)
{
	gchar *query;
	struct db_result result;
	gchar **radio = NULL;
	gint i, j=0;

	query = g_strdup_printf("SELECT NAME FROM RADIO");

	if (exec_sqlite_query(query, cdbase, &result)) {
		if (result.no_rows) {
			radio = g_malloc0((result.no_rows+1) * sizeof(gchar *));
			for_each_result_row(result, i) {
				radio[j] = g_strdup(result.resultp[i]);
				j++;
			}
			radio[j] = NULL;
		}
		sqlite3_free_table(result.resultp);
	}

	return radio;
}

/* 'radio' has to be a sanitized string */

void delete_radio_db(const gchar *radio, struct con_dbase *cdbase)
{
	gint radio_id;
	gchar *query;

	if (string_is_empty(radio)) {
		g_warning("Radio name is NULL");
		return;
	}

	radio_id = find_radio_db(radio, cdbase);

	if (!radio_id) {
		g_warning("Radio doesn't exist");
		return;
	}

	query = g_strdup_printf("DELETE FROM RADIO_TRACKS WHERE RADIO=%d;",
				radio_id);
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM RADIO WHERE ID=%d;",
				radio_id);
	exec_sqlite_query(query, cdbase, NULL);
}

/* Flushes all the tracks in a given playlist */

void flush_radio_db(gint radio_id, struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("DELETE FROM RADIO_TRACKS WHERE RADIO=%d;",
				radio_id);
	exec_sqlite_query(query, cdbase, NULL);
}

void flush_db(struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("DELETE FROM TRACK");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM LOCATION");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM ARTIST");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM ALBUM");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM GENRE");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM YEAR");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM COMMENT");
	exec_sqlite_query(query, cdbase, NULL);
}

/* Flush unused artists, albums, genres, years */

void flush_stale_entries_db(struct con_dbase *cdbase)
{
	gchar *query;

	query = g_strdup_printf("DELETE FROM ARTIST WHERE id NOT IN "
				"(SELECT artist FROM TRACK);");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM ALBUM WHERE id NOT IN "
				"(SELECT album FROM TRACK);");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM GENRE WHERE id NOT IN "
				"(SELECT genre FROM TRACK);");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM YEAR WHERE id NOT IN "
				"(SELECT year FROM TRACK);");
	exec_sqlite_query(query, cdbase, NULL);

	query = g_strdup_printf("DELETE FROM COMMENT WHERE id NOT IN "
				"(SELECT comment FROM TRACK);");
	exec_sqlite_query(query, cdbase, NULL);
}

gboolean fraction_update(GtkWidget *pbar)
{
	static gdouble fraction = 0.0;
	gint files_scanned = 0;
	gint no_files;

	no_files = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(pbar), "no_files"));
	files_scanned = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(pbar), "files_scanned"));

	if(files_scanned > 0)
		fraction = (gdouble)files_scanned / (gdouble)no_files;

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar), fraction);

	return TRUE;
}

void rescan_db(const gchar *dir_name, gint no_files, GtkWidget *pbar,
	       gint call_recur, GCancellable *cancellable, struct con_dbase *cdbase)
{
	static gint files_scanned = 0;
	gint progress_timeout = 0;
	GDir *dir;
	const gchar *next_file = NULL;
	gchar *ab_file;
	GError *error = NULL;

	/* Reinitialize static variables if called from rescan_library_action */

	if (call_recur)
		files_scanned = 0;

	if (g_cancellable_is_cancelled (cancellable))
		goto exit;

	dir = g_dir_open(dir_name, 0, &error);
	if (!dir) {
		g_critical("Unable to open library : %s", dir_name);
		goto exit;
	}

	if(progress_timeout == 0) {
		g_object_set_data(G_OBJECT(pbar), "no_files", GINT_TO_POINTER(no_files));
		g_object_set_data(G_OBJECT(pbar), "files_scanned", GINT_TO_POINTER(files_scanned));
		progress_timeout = g_timeout_add_seconds(3, (GSourceFunc)fraction_update, pbar);
	}

	next_file = g_dir_read_name(dir);
	while (next_file) {
		if (g_cancellable_is_cancelled (cancellable))
			goto exit;
		ab_file = g_strconcat(dir_name, "/", next_file, NULL);
		if (g_file_test(ab_file, G_FILE_TEST_IS_DIR))
			rescan_db(ab_file, no_files, pbar, 0, cancellable, cdbase);
		else {
			files_scanned++;
			add_entry_db(ab_file, cdbase);
		}

		/* Have to give control to GTK periodically ... */
		pragha_process_gtk_events ();

		g_free(ab_file);
		next_file = g_dir_read_name(dir);
	}
	g_dir_close(dir);
exit:
	if(progress_timeout != 0) {
		g_source_remove(progress_timeout);
		progress_timeout = 0;
	}
}

void update_db (const gchar *dir_name,
		gint no_files,
		GtkWidget *pbar,
		GTimeVal last_rescan_time,
		gint call_recur,
		GCancellable *cancellable,
		struct con_dbase *cdbase)
{
	static gint files_scanned = 0;
	gint progress_timeout = 0;
	GDir *dir;
	const gchar *next_file = NULL;
	gchar *ab_file = NULL, *s_ab_file = NULL;
	GError *error = NULL;
	struct stat sbuf;

	/* Reinitialize static variables if called from rescan_library_action */

	if (call_recur)
		files_scanned = 0;

	if (g_cancellable_is_cancelled (cancellable))
		goto exit;

	dir = g_dir_open(dir_name, 0, &error);
	if (!dir) {
		g_critical("Unable to open library : %s", dir_name);
		goto exit;
	}

	if(progress_timeout == 0) {
		g_object_set_data(G_OBJECT(pbar), "no_files", GINT_TO_POINTER(no_files));
		g_object_set_data(G_OBJECT(pbar), "files_scanned", GINT_TO_POINTER(files_scanned));
		progress_timeout = g_timeout_add_seconds(3, (GSourceFunc)fraction_update, pbar);
	}

	next_file = g_dir_read_name(dir);
	while (next_file) {
		if (g_cancellable_is_cancelled (cancellable))
			goto exit;
		ab_file = g_strconcat(dir_name, "/", next_file, NULL);
		if (g_file_test(ab_file, G_FILE_TEST_IS_DIR))
			update_db(ab_file, no_files, pbar, last_rescan_time, 0, cancellable, cdbase);
		else {
			files_scanned++;
			s_ab_file = sanitize_string_to_sqlite3(ab_file);
			if (!find_location_db(s_ab_file, cdbase)) {
				add_entry_db(ab_file, cdbase);
			} else {
				g_stat(ab_file, &sbuf);
				if (sbuf.st_mtime > last_rescan_time.tv_sec) {
					if (find_location_db(s_ab_file, cdbase))
						delete_track_db(ab_file, cdbase);
					add_entry_db(ab_file, cdbase);
				}
			}
			g_free(s_ab_file);
		}

		/* Have to give control to GTK periodically ... */
		pragha_process_gtk_events ();

		g_free(ab_file);
		next_file = g_dir_read_name(dir);
	}
	g_dir_close(dir);
exit:
	if(progress_timeout != 0) {
		g_source_remove(progress_timeout);
		progress_timeout = 0;
	}
}

/* Delete all tracks falling under the given directory.
   Also, flush the database of unused albums, artists, etc. */

void delete_db(const gchar *dir_name, gint no_files, GtkWidget *pbar,
	       gint call_recur, struct con_dbase *cdbase)
{
	gchar *query, *sdir_name;

	sdir_name = sanitize_string_to_sqlite3(dir_name);

	/* Delete all tracks under the given dir */

	query = g_strdup_printf("DELETE FROM TRACK WHERE location IN "
				"(SELECT id FROM LOCATION WHERE NAME LIKE '%s%%');",
				sdir_name);
	exec_sqlite_query(query, cdbase, NULL);

	/* Delete the location entries */

	query = g_strdup_printf("DELETE FROM LOCATION WHERE name LIKE '%s%%';",
				sdir_name);
	exec_sqlite_query(query, cdbase, NULL);

	/* Delete all entries from PLAYLIST_TRACKS which match given dir */

	query = g_strdup_printf("DELETE FROM PLAYLIST_TRACKS WHERE file LIKE '%s%%';",
				sdir_name);
	exec_sqlite_query(query, cdbase, NULL);

	/* Now flush unused artists, albums, genres, years */

	flush_stale_entries_db(cdbase);

	g_free(sdir_name);
}

gint init_dbase_schema(struct con_dbase *cdbase)
{
	gchar *query;

	/* Set PRAGMA synchronous = OFF */

	query = g_strdup_printf("PRAGMA synchronous=OFF");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'TRACKS' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS TRACK "
				"(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s);",
				"location INT PRIMARY KEY",
				"track_no INT",
				"artist INT",
				"album INT",
				"genre INT",
				"year INT",
				"comment INT",
				"bitrate INT",
				"length INT",
				"channels INT",
				"samplerate INT",
				"file_type INT",
				"title VARCHAR(255)");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'LOCATION' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS LOCATION "
				"(%s, %s, UNIQUE(name));",
				"id INTEGER PRIMARY KEY",
				"name TEXT");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;


	/* Create 'ARTIST' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS ARTIST "
				"(%s, %s, UNIQUE(name));",
				"id INTEGER PRIMARY KEY",
				"name VARCHAR(255)");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;


	/* Create 'ALBUM' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS ALBUM "
				"(%s, %s, UNIQUE(name));",
				"id INTEGER PRIMARY KEY",
				"name VARCHAR(255)");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'GENRE' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS GENRE "
				"(%s, %s, UNIQUE(name));",
				"id INTEGER PRIMARY KEY",
				"name VARCHAR(255)");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;


	/* Create 'YEAR' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS YEAR "
				"(%s, %s, UNIQUE(year));",
				"id INTEGER PRIMARY KEY",
				"year INT");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'COMMENT' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS COMMENT "
				"(%s, %s, UNIQUE(name));",
				"id INTEGER PRIMARY KEY",
				"name VARCHAR(255)");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'PLAYLIST_TRACKS' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS PLAYLIST_TRACKS "
				"(%s, %s);",
				"file TEXT",
				"playlist INT");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'PLAYLIST table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS PLAYLIST "
				"(%s, %s, UNIQUE(name));",
				"id INTEGER PRIMARY KEY",
				"name VARCHAR(255)");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'RADIO_TRACKS' table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS RADIO_TRACKS "
				"(%s, %s);",
				"uri TEXT",
				"radio INT");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	/* Create 'RADIO table */

	query = g_strdup_printf("CREATE TABLE IF NOT EXISTS RADIO "
				"(%s, %s, UNIQUE(name));",
				"id INTEGER PRIMARY KEY",
				"name VARCHAR(255)");
	if (!exec_sqlite_query(query, cdbase, NULL))
		return -1;

	return 0;
}

gint drop_dbase_schema(struct con_dbase *cdbase)
{
	gint ret = 0;
	gchar *query;

	query = g_strdup_printf("DROP TABLE ALBUM");
	if (!exec_sqlite_query(query, cdbase, NULL))
		ret = -1;

	query = g_strdup_printf("DROP TABLE ARTIST");
	if (!exec_sqlite_query(query, cdbase, NULL))
		ret = -1;

	query = g_strdup_printf("DROP TABLE GENRE");
	if (!exec_sqlite_query(query, cdbase, NULL))
		ret = -1;

	query = g_strdup_printf("DROP TABLE LOCATION");
	if (!exec_sqlite_query(query, cdbase, NULL))
		ret = -1;

	query = g_strdup_printf("DROP TABLE TRACK");
	if (!exec_sqlite_query(query, cdbase, NULL))
		ret = -1;

	query = g_strdup_printf("DROP TABLE YEAR");
	if (!exec_sqlite_query(query, cdbase, NULL))
		ret = -1;

	query = g_strdup_printf("DROP TABLE COMMENT");
	if (!exec_sqlite_query(query, cdbase, NULL))
		ret = -1;

	return ret;
}

static gint db_get_table_count(struct con_dbase *cdbase, const gchar *table)
{
	gchar *query;
	struct db_result result;
	gint ret = 0;

	query = g_strdup_printf("SELECT COUNT() FROM %s;", table);
	if (exec_sqlite_query(query, cdbase, &result)) {
		ret = atoi(result.resultp[1]);
		sqlite3_free_table(result.resultp);
	}

	return ret;
}

gint db_get_artist_count(struct con_dbase *cdbase)
{
	return db_get_table_count (cdbase, "ARTIST");
}

gint db_get_album_count(struct con_dbase *cdbase)
{
	return db_get_table_count (cdbase, "ALBUM");
}

gint db_get_track_count(struct con_dbase *cdbase)
{
	return db_get_table_count (cdbase, "TRACK");
}

void db_begin_transaction(struct con_dbase *cdbase)
{
	gchar *query = g_strdup("BEGIN TRANSACTION");
	exec_sqlite_query(query, cdbase, NULL);
}

void db_commit_transaction(struct con_dbase *cdbase)
{
	gchar *query = g_strdup("END TRANSACTION");
	exec_sqlite_query(query, cdbase, NULL);
}

gboolean exec_sqlite_query(gchar *query, struct con_dbase *cdbase,
			   struct db_result *result)
{
	gchar *err = NULL;
	gboolean ret = FALSE;

	if (!query)
		return FALSE;

	CDEBUG(DBG_DB, "%s", query);

	/* Caller doesn't expect any result */

	if (!result) {
		sqlite3_exec(cdbase->db, query, NULL, NULL, &err);
		if (err) {
			g_critical("SQL Err : %s",  err);
			g_critical("query   : %s", query);
			ret = FALSE;
		} else {
			ret = TRUE;
		}
		sqlite3_free(err);
	}

	/* Caller expects result */

	else {
		sqlite3_get_table(cdbase->db, query,
				  &result->resultp,
				  &result->no_rows,
				  &result->no_columns,
				  &err);
		if (err) {
			g_critical("SQL Err : %s",  err);
			g_critical("query   : %s", query);
			ret = FALSE;
		}
		else {
			ret = TRUE;
		}
		sqlite3_free(err);
	}

	/* Free the query here, don't free in the callsite ! */

	g_free(query);

	return ret;
}

static void
rescand_icompatible_db_response(GtkDialog *dialog,
				gint response,
				struct con_win *cwin)
{
	if(response == GTK_RESPONSE_YES)
		rescan_library_handler(cwin);

	gtk_widget_destroy(GTK_WIDGET(dialog));
}

#if GTK_CHECK_VERSION (3, 0, 0)
static void rescand_icompatible_db(struct con_win *cwin)
{
#else
static gboolean rescand_icompatible_db(gpointer data)
{
	struct con_win *cwin = data;
#endif
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new(GTK_WINDOW(cwin->mainwindow),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_WARNING,
					GTK_BUTTONS_YES_NO,
					_("Sorry: The music database is incompatible with previous versions to 0.8.0\n\n"
					"Want to upgrade the collection?."));

	g_signal_connect(G_OBJECT(dialog), "response",
			G_CALLBACK(rescand_icompatible_db_response), cwin);

	gtk_widget_show_all (dialog);
#if !GTK_CHECK_VERSION (3, 0, 0)
	return TRUE;
#endif
}

gint init_musicdbase(struct con_win *cwin)
{
	gint ret;
	gchar *db_file;
	const gchar *home;

	CDEBUG(DBG_INFO, "Initializing music dbase");

	cwin->cdbase = g_slice_new0(struct con_dbase);

	home = g_get_user_config_dir();
	db_file = g_build_path(G_DIR_SEPARATOR_S, home, "/pragha/pragha.db", NULL);

	if (cwin->cpref->installed_version != NULL &&
	    g_ascii_strcasecmp(cwin->cpref->installed_version, MIN_DATABASE_VERSION) < 0 ) {
		g_critical("Deleted Music database incompatible with previous to 0.8.0. Please rescan library.");
		ret = g_unlink(db_file);
		if (ret != 0)
			g_warning("%s", strerror(ret));
		#if GTK_CHECK_VERSION (3, 0, 0)
		rescand_icompatible_db(cwin);
		#else
		gtk_init_add(rescand_icompatible_db, cwin);
		#endif
	}

	/* Create the database file */

	ret = sqlite3_open(db_file, &cwin->cdbase->db);
	if (ret) {
		g_critical("Unable to open/create DB file : %s", db_file);
		g_free(db_file);
		return -1;
	}

	g_free(db_file);

	return init_dbase_schema(cwin->cdbase);
}

void db_free (struct con_dbase *cdbase)
{
	sqlite3_close(cdbase->db);
	g_slice_free(struct con_dbase, cdbase);
}
