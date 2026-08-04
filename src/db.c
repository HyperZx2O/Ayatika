/* ============================================================
 * db.c — SQLite bookmark, note, and config storage
 * Owned by: Backend Engineer
 *
 * Responsibilities:
 *   - Initialize SQLite database and bookmarks table
 *   - CRUD operations for bookmarks (save/load/delete/exists)
 *   - Read/write config.ini for user preferences
 *
 * See member1.md for the full implementation plan.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
/* quran.h includes <raylib.h>, which is not available when this module
   is compiled in isolation. No struct here uses raylib types, so we
   short-circuit raylib.h via its include guard (same pattern as quran.c). */
#define RAYLIB_H
#include "db.h"

static sqlite3 *db = NULL;

int initDatabase(void) {
    if (db) return 1;
    if (sqlite3_open("data/almaktaba.db", &db) != SQLITE_OK) return 0;

    const char *sql =
        "CREATE TABLE IF NOT EXISTS bookmarks ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  surah_id  INTEGER NOT NULL,"
        "  ayah_id   INTEGER NOT NULL,"
        "  tag       TEXT    DEFAULT '',"
        "  note      TEXT    DEFAULT '',"
        "  timestamp INTEGER NOT NULL"
        ");";
    if (sqlite3_exec(db, sql, NULL, NULL, NULL) != SQLITE_OK) return 0;
    return 1;
}

int saveBookmark(Bookmark *bm) {
    if (!db || !bm) return 0;

    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "INSERT INTO bookmarks (surah_id, ayah_id, tag, note, timestamp)"
        " VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_int (stmt, 1, bm->surahNumber);
    sqlite3_bind_int (stmt, 2, bm->ayahNumber);
    sqlite3_bind_text(stmt, 3, bm->tag,  -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, bm->note, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 5, (int)time(NULL));

    int ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int loadBookmarks(Bookmark *out, int maxCount) {
    if (!db || !out || maxCount <= 0) return 0;

    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT id, surah_id, ayah_id, tag, note, timestamp"
        " FROM bookmarks ORDER BY timestamp DESC;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

    int n = 0;
    while (n < maxCount && sqlite3_step(stmt) == SQLITE_ROW) {
        Bookmark *b = &out[n];
        b->id          = sqlite3_column_int(stmt, 0);
        b->surahNumber = sqlite3_column_int(stmt, 1);
        b->ayahNumber  = sqlite3_column_int(stmt, 2);
        const char *tag  = (const char *)sqlite3_column_text(stmt, 3);
        const char *note = (const char *)sqlite3_column_text(stmt, 4);
        strncpy(b->tag,  tag  ? tag  : "", sizeof(b->tag)  - 1);
        strncpy(b->note, note ? note : "", sizeof(b->note) - 1);
        b->tag[sizeof(b->tag) - 1]   = '\0';
        b->note[sizeof(b->note) - 1] = '\0';
        b->timestamp = (long)sqlite3_column_int(stmt, 5);
        n++;
    }
    sqlite3_finalize(stmt);
    return n;
}

int deleteBookmark(int id) {
    if (!db) return 0;

    sqlite3_stmt *stmt = NULL;
    const char *sql = "DELETE FROM bookmarks WHERE id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_int(stmt, 1, id);
    int ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

int bookmarkExists(int surahNum, int ayahNum) {
    if (!db) return 0;

    sqlite3_stmt *stmt = NULL;
    const char *sql =
        "SELECT COUNT(*) FROM bookmarks WHERE surah_id = ? AND ayah_id = ?;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_int(stmt, 1, surahNum);
    sqlite3_bind_int(stmt, 2, ayahNum);

    int exists = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW)
        exists = sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    return exists;
}
