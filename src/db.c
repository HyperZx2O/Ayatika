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
#include "db.h"

/* TODO: #include <sqlite3.h> once lib/sqlite3.h is added */

int initDatabase(void) {
    /* TODO: sqlite3_open + CREATE TABLE IF NOT EXISTS bookmarks */
    return 0;
}

int saveBookmark(Bookmark *bm) {
    (void)bm;
    /* TODO: implement */
    return 0;
}

int loadBookmarks(Bookmark *out, int maxCount) {
    (void)out;
    (void)maxCount;
    /* TODO: implement */
    return 0;
}

int deleteBookmark(int id) {
    (void)id;
    /* TODO: implement */
    return 0;
}

int bookmarkExists(int surahNum, int ayahNum) {
    (void)surahNum;
    (void)ayahNum;
    /* TODO: implement */
    return 0;
}


