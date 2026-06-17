# Ayatika — Shared Architecture & Git Workflow

## Read this file together before anyone starts coding.

---

## Project Structure

```
ayatika/
├── .gitignore
├── README.md
├── Makefile
├── ARCHITECTURE.md
├── src/
│   ├── main.c              ← Frontend Engineer (entry point + game loop)
│   ├── quran.h             ← Shared header — defines all structs (Backend owns, all approve)
│   ├── quran.c             ← Backend Engineer
│   ├── prayer.h/.c         ← Backend Engineer
│   ├── db.h/.c             ← Backend Engineer
│   ├── surah_meta.c        ← Backend Engineer
│   ├── ui.h/.c             ← Frontend Engineer
│   ├── theme.h/.c          ← Frontend Engineer
│   ├── input.h/.c          ← Frontend Engineer
│   ├── audio.h/.c          ← Systems & Features Engineer
│   ├── screensaver.h/.c    ← Systems & Features Engineer
│   └── search.h/.c         ← Systems & Features Engineer
├── lib/
│   ├── raylib.h             (system-installed or vendored)
│   ├── cJSON.h / cJSON.c
│   ├── sqlite3.h / sqlite3.c
│   └── fts_fuzzy_match.h
├── assets/
│   ├── azan.mp3
│   ├── nature.ogg
│   ├── click.wav
│   ├── surah_switch.wav
│   ├── Amiri.ttf
│   └── cat.png
└── data/                    ← gitignored, generated at runtime
    ├── quran.json
    ├── translation_en.json
    ├── translation_bn.json
    ├── almaktaba.db
    └── config.ini
```

---

## Team Roles

| Role | Files Owned | Responsibility |
|---|---|---|
| **Backend Engineer** | `quran.h` (shared), `quran.c`, `prayer.h/.c`, `db.h/.c`, `surah_meta.c` | Quran data fetching/parsing/caching, prayer time calculation, bookmark database, Surah metadata |
| **Frontend Engineer** | `main.c`, `ui.h/.c`, `theme.h/.c`, `input.h/.c` | Window, game loop, all screens, Arabic rendering, keybindings, themes |
| **Systems & Features Engineer** | `audio.h/.c`, `screensaver.h/.c`, `search.h/.c` | Audio system, Azan screensaver, sleeping cat, fuzzy search |

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

## Integration Dependency Order

```
quran.h (shared structs — agreed by all 3 before any other code)
    ↓
quran.c, prayer.c, db.c (Backend implements)
    ↓
main.c scaffold, theme.c (Frontend implements, using placeholder data initially)
    ↓
audio.c, screensaver.c (Systems implements)
    ↓
ui.c, input.c (Frontend completes with real data from Backend)
    ↓
search.c (Systems completes with real data from Backend)
    ↓
Full integration and polish
```

**The critical path:** Backend must finish `quran.h` and the core of `quran.c` before Frontend and Systems can use real data. Until then, Frontend and Systems build against hardcoded placeholder data.

---

## Shared Struct Ownership Rule

`quran.h` is owned by the Backend Engineer but affects everyone, since `AppState` accumulates fields from all three members over the project. The process for any change:

1. Propose the change in the group chat (e.g. "adding `audioUrl` to `Ayah`")
2. Confirm it won't break anyone else's code
3. Commit with the `[shared]` prefix
4. Everyone pulls immediately after

---

## How the Final Build Wires Together

```
main()                          ← Frontend
  ├── loadConfig()              ← Backend
  ├── loadQuranData()           ← Backend
  ├── initDatabase()            ← Backend
  ├── initThemes()              ← Frontend
  ├── initFonts()                ← Frontend
  ├── initAudio()               ← Systems
  ├── initScreensaver()         ← Systems
  └── game loop:
        ├── updatePrayerTimes() ← Backend
        ├── updateAudio()       ← Systems
        ├── handleInput()       ← Frontend (calls Backend's db functions, Systems' audio)
        ├── drawCurrentScreen() ← Frontend (calls Systems' drawScreensaver, drawCat, drawSearch)
        └── on exit:
              ├── saveConfig()  ← Backend
              └── closeAudio()  ← Systems
```

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

- [ ] Clone the repo
- [ ] Run `make` to confirm the build system works (it will fail to link until source files have real content — that's expected at this stage)
- [ ] Check out your own branch (`backend/m1`, `frontend/m2`, or `systems/m3`)
- [ ] Read your individual task file (`member1.md`, `member2.md`, or `member3.md`)
- [ ] Confirm `src/quran.h` matches what's described in your task file before writing any code
- [ ] Start with Week 1 tasks
