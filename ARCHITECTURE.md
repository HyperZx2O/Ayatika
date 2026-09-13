# Ayatika — Shared Architecture & Git Workflow
> Synced to the current `src/` implementation (mock-data integration stage).
## Read this file together before anyone starts coding.

---

## Project Structure (actual)

```
ayatika/
├── .gitignore
├── README.md
├── Makefile              # all/run/clean/test + 6 test_* harnesses
├── ARCHITECTURE.md
├── ayatika.exe.manifest
├── glfw3.dll
├── src/
│   ├── main.c              ← Frontend (entry point + game loop)
│   ├── quran.h             ← Shared header — all structs + APIs (Backend owns, all approve)
│   ├── quran.c             ← Backend
│   ├── prayer.h/.c         ← Backend
│   ├── db.h/.c             ← Backend
│   ├── surah_meta.c        ← Backend (114 entries, no header)
│   ├── config.h/.c         ← Backend
│   ├── mock_data.h/.c      ← Frontend scaffold (shares quran.h types)
│   ├── ui.h/.c             ← Frontend (all screens)
│   ├── theme.h/.c          ← Frontend (4 themes)
│   ├── input.h/.c          ← Frontend (dual-mode bindings + settings)
│   ├── audio.h/.c          ← Systems & Features
│   ├── screensaver.h/.c    ← Systems & Features (cat + screensaver + Hadith panel)
│   └── search.h/.c         ← Systems & Features
├── lib/
│   ├── cJSON.h / cJSON.c
│   ├── sqlite3.h / sqlite3.c
│   ├── fts_fuzzy_match.h   # recursionLimit 256
│   ├── raylib.h            # stub for backend-only compiles
│   ├── include/raylib.h    # real header
│   ├── include/fribidi/    # vendored headers
│   ├── curl/curl.h
│   ├── libraylib.a
│   ├── libglfw3.a / libglfw3.dll.a
│   ├── libfribidi.a
│   └── libcurl.a
├── assets/
│   ├── Amiri.ttf
│   ├── JetBrainsMono-Regular.ttf
 │   ├── azan.mp3
 │   └── cat.png             # 6-frame sheet
├── data/                   ← gitignored, runtime-generated (+ .gitkeep)
 │   ├── quran.json
 │   ├── translation_en.json
 │   ├── almaktaba.db
│   └── config.ini
└── tests/                  # 6 wired to `make test` + 7 legacy
    ├── test_audio.c / test_search.c / test_search_ui.c
    ├── test_screensaver.c / test_cat.c / test_systems.c (--auto)
    └── test_phase2-7.c, test_raylib.c (legacy)
```

`src/quran.h` defines: `Surah`, `Ayah`, `Bookmark`, `Hadith`, `PrayerTimes`, `SearchResult`, 10-screen `AppScreen` (Dashboard, Surah List, Ayah Reader, Search, Bookmarks, Screensaver, Surah Overview, Settings, Reading Hub, Hadith), and full `AppState` (navigation + data + prayer + UI + audio + idle + search + config + hub/hadith cursors + `vimMotions/fontScale/idleSeconds/autoResume`). `MAX_SEARCH_RESULTS 15`, `TOTAL_SURAHS 114`.

---

## Team Roles

| Role | Files Owned | Responsibility |
|---|---|---|
| **Backend Engineer** | `quran.h` (shared), `quran.c`, `prayer.h/.c`, `db.h/.c`, `surah_meta.c`, `config.h/.c` | Quran fetch/parse/cache, prayer calc, bookmarks DB, 114 metadata, config |
| **Frontend Engineer** | `main.c`, `mock_data.h/.c`, `ui.h/.c`, `theme.h/.c`, `input.h/.c` | Window, loop, 10 screens, Arabic RTL, scaling, dual-mode keys, 4 themes, settings |
| **Systems & Features Engineer** | `audio.h/.c`, `screensaver.h/.c`, `search.h/.c` | Audio, Azan screensaver, cat (6f/0.15s), fuzzy search (+500 surah boost), Hadith-of-day |

---

## Branch Strategy

```
main                  ← stable, working code only — no direct pushes
├── backend/m1
├── frontend/m2
└── systems/m3
```

**Rules:**
- Nobody pushes to `main` directly — ever
- Each member opens a Pull Request to merge into `main`
- At least one other member must review and approve the PR
- Merge into `main` every Sunday

---

## Commit Message Convention

```
[backend]  short description of what you did
[frontend] short description of what you did
[systems]  short description of what you did
[shared]   changes to quran.h or Makefile (discuss with team before committing)
```

Examples:
```
[backend] libcurl fetch and JSON cache working
[frontend] arabic RTL rendering with Amiri font
[systems] sleeping cat sprite animation complete
[shared] add searchResultCount field to AppState struct
```

---

## Weekly Merge Process

Every Sunday:
1. Each member opens a Pull Request from their branch to `main`
2. Other two members review and leave at least one comment
3. Merge in this order: `backend/m1` first, then `frontend/m2`, then `systems/m3`
4. After each merge, the other two pull `main` into their own branch:
   ```bash
   git checkout backend/m1
   git merge main
   # resolve any conflicts
   git push origin backend/m1
   ```

---

## Integration Dependency Order (current stage)

```
quran.h (shared structs — done, 10 screens + extended AppState)
    ↓
quran.c, prayer.c, db.c, surah_meta.c, config.c (Backend done, verified)
    ↓
main.c scaffold, theme.c, mock_data.c (Frontend done, mock-data stage)
    ↓
audio.c, screensaver.c (Systems done, harnesses green)
    ↓
ui.c, input.c (Frontend done: 10 screens, dual bindings, settings)
    ↓
search.c (Systems done: engine + screen + seams)
    ↓
Full integration (remaining):
  main.c → loadQuranData/updatePrayerTimes/initAudio/updateAudio,
  drawCurrentScreen → drawSearch/drawScreensaver, full-link flags
```

**The critical path (done):** Backend `quran.h` + `quran.c` core landed first; Frontend/Systems built against `mock_data.c` sharing the real `quran.h` types.

---

## Shared Struct Ownership Rule

`quran.h` is owned by the Backend Engineer but affects everyone, since `AppState` accumulates fields from all three members. The process for any change:

1. Propose the change in the group chat (e.g. "adding `audioUrl` to `Ayah`" — done)
2. Confirm it won't break anyone else's code
3. Commit with the `[shared]` prefix
4. Everyone pulls immediately after

Note: backend `.c` files compile with `#define RAYLIB_H` before `quran.h` (they use no RayLib types); `lib/raylib.h` stub exists for that path.

---

## How the Final Build Wires Together

Current `main()` (mock stage) → target wiring:

```
main()                          ← Frontend
  ├── loadConfig()              ← Backend (pending wiring; mock defaults now)
  ├── loadQuranData()           ← Backend (pending wiring; loadMockData now)
  ├── initDatabase()            ← Backend (pending wiring)
  ├── initThemes()              ← Frontend (done)
  ├── initFonts(state)          ← Frontend (done, codepoint atlas)
  ├── initFocusTexture()        ← Frontend (done)
  ├── initAudio()               ← Systems (pending wiring)
  ├── initScreensaver()         ← Systems (pending wiring)
  └── game loop:
        ├── updatePrayerTimes() ← Backend (pending wiring)
        ├── updateAudio(state)  ← Systems (pending wiring)
        ├── handleInput()       ← Frontend (done: shared/vim/arrow/settings/screensaver)
        ├── drawCurrentScreen() ← Frontend (done; search/screensaver full wiring pending)
        └── on exit:
              ├── saveConfig()  ← Backend (pending wiring)
              ├── closeAudio()  ← Systems (pending wiring)
              ├── closeScreensaver()/closeFonts()/closeFocusTexture()
```

Key APIs crossed: `getAyah/getDailyAyahIndex`, `saveBookmark/loadBookmarks/deleteBookmark/bookmarkExists`, `getSurahMeta`, `getNextPrayerName/Time/formatCountdown`, `playAzan/stopAzan`, `playReminder/stopReminder`, `drawScreensaver/drawCat`, `runSearch`, `drawHadithPage`.

---

## Makefile (actual)

```makefile
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I./src -I./lib
LIBS = -lraylib -lm
SRC = $(wildcard src/*.c) $(wildcard lib/cJSON.c) $(wildcard lib/sqlite3.c)
all / run / clean
test: test_audio test_search test_search_ui test_screensaver test_cat test_systems
```

Known gap: full-app link still needs `-lcurl -lsqlite3 -lfribidi` + `-I./lib/include`; current flags cover the 6 systems harnesses.

---

## Conflict Resolution

If two members accidentally edit the same file:
1. Git conflicts are normal — do not panic
2. Open the conflicted file, look for `<<<<<<`, `=======`, `>>>>>>>`
3. Discuss in the group chat which version to keep
4. Remove the conflict markers, keep the correct code
5. `git add` the file and commit with `[shared] resolve conflict in X`

To avoid conflicts entirely: only edit files in your own section. If you need to change a file owned by someone else, ask them first.

---

## Getting Started Checklist

- [x] Clone the repo
- [x] `make test` — 6 systems harnesses build + run (use `--auto` for systems)
- [x] `src/quran.h` matches `context.md` shared structs
- [ ] Check out your branch and complete remaining integration wiring (`main.c` live data, search/screensaver dispatch, full-link flags)
