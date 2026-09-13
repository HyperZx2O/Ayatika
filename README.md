# Ayatika — القرآن الكريم

> **A desktop Quran study application, written entirely in C, powered by RayLib.**
>
> *Built for CSE 4202 — Structured Programming II Laboratory, Islamic University of Technology.*

Ayatika is a keyboard-driven Quran reader with live API data (6236 ayahs, EN + BN translations), shaped Arabic rendering with tashkeel-aware layout, 4 themes, dual Vim/arrow-key modes, tag bookmarks in SQLite, a unified Finder (surah jump + fuzzy ayah search), prayer times with a reminder/Azan alarm, and a 14,940-hadith Bukhari/Muslim panel. Builds with `gcc` + `make` on Windows (MinGW-w64).

---

## Screens (9, all in `drawCurrentScreen()`)

| Screen | Description |
|---|---|
| **Dashboard** | Greeting, prayer countdown + progress, random Ayah of the Day, Hadith card, continue-reading |
| **Reading Hub** | Two tiles: Surah vs Hadith |
| **Surah Overview** | Modal: Arabic name, Meccan/Medinan + ayah-count badges, context blurb |
| **Ayah Reader** | Wrapped right-aligned Arabic + English, ★ bookmark, focus cinematic mode |
| **Hadith Panel** | Scrollable Bukhari/Muslim cards with source tabs + filter, full-text modal with scroll |
| **Bookmarks** | Saved `surah:ayah` + tag + relative timestamps |
| **Settings** | 9 rows: Vim motions, font scale, screensaver s, auto-resume, theme, calc method, latitude edit, test reminder, test prayer alarm |
| **Screensaver** | Azan prayer alarm (screensaver + Azan together), clock, live next-prayer line, cat |
| **Help overlay** | `F1` opens on top of any screen, `Esc` closes (Vim vs arrow tables) |

Overlays (float above any screen): **Finder** (`Surahs | Ayahs` tabs), bookmark tag editor, hadith modal, bookmark toast.

## Keyboard Navigation

Dual modes — **Vim-style** (`vimMotions==1`) or **arrow keys** (default) — switchable in Settings. Held nav keys repeat (one step, then 25/s after 0.45s).

| Key | Action |
|---|---|
| `j/k` or `Up/Down` | Move cursor (dashboard/hub/list/reader/hadith/finder context-aware) |
| `h/l` or `Left/Right` | Left/right (dashboard/hub/list) |
| `Enter` | Open selected item |
| `Esc` | Go back (help/palette/editor: close; screensaver: exits + silences Azan) |
| `G` (Shift) / `End` or `PgUp/PgDn` | Go to top / bottom |
| `Ctrl+d / Ctrl+u` | Half page down / up |
| `t` | Cycle theme (of 4) |
| `s` | Open settings |
| `/` | Finder, Ayahs tab |
| `o` | Finder, Surahs tab |
| `Tab` | Switch Finder tab / cycle hadith filter |
| `m` | Open bookmarks |
| `b` | Bookmark current ayah with tag editor (reader only) |
| `d` | Delete bookmark (bookmarks page only) |
| `f` | Toggle focus cinematic (reader only) |
| `g` | Go to dashboard (both modes) |
| `F1` | Open help overlay |

Surah list extras: type digits to jump to a number live (`Go: 36`), type a free letter to summon the palette prefilled, click Finder rows to open. Settings rows use `j/k`+`Enter`/`Space`, latitude row types inline (`Enter` confirm, `Esc` cancel).

## Prayer Alarm

Reminder audio plays 5 minutes before the waqt (`assets/reminder.mp3`); at waqt start the app enters the screensaver and plays the Azan (`assets/azan.mp3`, once per waqt). Exiting the screensaver silences it.
## Themes

Four presets + native title-bar sync (`dwmapi.dll`):

| Theme | Background | Accent | Character |
|---|---|---|---|
| **Celestial Night** | Deep navy `#0C0E1C` | Gold `#D2AF5A` | Dark, scholarly |
| **Moonlit Garden** | Warm cream `#F8F2EB` | Rose `#A55555` | Light, warm |
| **Peacock Court** | Dark teal `#0A191C` | Coral `#D27864` | Rich, vibrant |
| **Amber Sanctum** | Warm brown `#372D23` | Emerald `#50AF78` | Earthy, calm |

Press **`t`** to cycle — the title bar follows suit automatically. Status line tints accent on errors (`Couldn't save/delete — try again`, offline notice).

## Tech Stack

| Component | Technology |
|---|---|
| Language | **C11** (GCC, no C++) |
| Graphics & Input | **RayLib 5.x** (`lib/libraylib.a`, `lib/include/raylib.h`) + GLFW (`glfw3.dll`, DLL link) |
| Arabic shaping | **FriBidi** (`lib/libfribidi.a`) via `fribidi_log2vis` + mark-aware draw/measure (tashkeel overstrikes, zero advance) |
| Fetch | **WinINet** (system, no libcurl) → AlQuran.cloud (`quran-uthmani`, `en.sahih`) + Hadith API CDN (`eng-bukhari`, `eng-muslim`) |
| JSON | **cJSON** (`lib/cJSON.h/.c`) |
| Storage | **SQLite3** (`lib/sqlite3.h/.c`, `data/almaktaba.db`) |
| Search | **fts_fuzzy_match** (`lib/fts_fuzzy_match.h`, limit 256) |
| Prayer | PrayTime v2.5 port in `prayer.c` (Hanafi Asr, Karachi/MWL/ISNA) |
| Fonts | **Amiri** (Arabic, 96pt atlas w/ Presentation Forms), **JetBrains Mono** (UI) |
| Audio | RayLib audio only (no miniaudio): `azan.mp3` (3.4 min), `reminder.mp3` (41 s) |

## Quick Start

### Requirements

- Windows 7 / 10 / 11
- **MinGW-w64** with `gcc`, `make`, and `windres` on PATH (windres embeds the DPI-aware manifest)

### Build & Run

```bash
git clone https://github.com/HyperZx2O/Ayatika.git
cd Ayatika
make
make run
```

Vendored static libs in `lib/`; only system DLL needed (`glfw3.dll`) is in the repo root. First run downloads Quran, translation, and hadith data into `data/` (offline afterwards).

### Tests

```bash
make test
```

Builds + runs 7 harnesses: `test_backend`, `test_audio`, `test_search`, `test_search_ui`, `test_screensaver`, `test_cat`, `test_systems --auto` (each PASS/FAIL, non-zero on failure).

## Project Structure

```
Ayatika/
├── assets/              # Amiri + JetBrains Mono + Hind Siliguri, azan/reminder/nature/click/switch, cat.png (6f), hadiths.json fallback
├── data/                # Runtime JSON/DB/ini (gitignored + .gitkeep)
├── lib/
│   ├── include/raylib.h + include/fribidi/
│   ├── cJSON.h/.c, sqlite3.h/.c, fts_fuzzy_match.h
│   └── libraylib.a, libglfw3.dll.a, libfribidi.a
├── src/
│   ├── main.c           # Entry, title-bar sync, scale loop, idle→screensaver, live API data
│   ├── quran.h/.c       # Shared structs + fetch/parse/lookup (Quran API + Hadith API, 4096B fields, UTF-8-safe copy)
│   ├── prayer.c         # PrayTimes + prohibited + countdown + alert helpers
│   ├── db.c             # SQLite tag bookmarks (deterministic order) + closeDatabase
│   ├── surah_meta.c     # 114 hardcoded surahs
│   ├── config.c         # 11-key ini + Dhaka defaults
│   ├── ui.c / ui.h      # 9 screens + Finder/palette/editor overlays, Scale system, FriBidi RTL, wrapped Arabic
│   ├── input.c / input.h# Shared/vim/arrow/settings/palette/editor bindings, key repeat, jump buffer
│   ├── theme.c / theme.h# 4 presets, cycleTheme
│   ├── audio.c / audio.h# Azan/reminder/recitation(file)/nature/SFX + waqt alert check, FileExists-guarded
│   ├── screensaver.c/h  # Cat + screensaver + firePrayerAlarm
│   └── search.c / search.h # Ayah fuzzy engine + palette surah filter + seams
├── tests/               # 7 wired harnesses + test_data fixture
├── glfw3.dll
├── ayatika.exe.manifest # PerMonitorV2 DPI + asInvoker (embedded via windres)
├── manifest.rc
├── Makefile
└── ARCHITECTURE.md
```

## Architecture Notes

- **Immediate-mode RayLib** — no retained UI beyond `AppState` (9 screens, `Scale S`, Finder state, search results, prayer, config).
- **Navigation** — `pushScreen`/`goBack` + `previousScreen`; hub/hadith cursors; `dashboardCursor` 0–4; `g` goes home; help/palette/editor are input-modal overlays.
- **Arabic** — `collectArabicCodepoints()` + `LoadFontEx(Amiri,96)` incl. Presentation Forms; `reorderArabic()` (FriBidi) → visual-order word wrap (`drawArabicWrapped`, cached, right-aligned lines); tashkeel marks draw overstruck via `drawShaped`/`measureShaped` (always paired).
- **Buffers** — ayah/translation fields are 4096B (real maxima: 2283B Arabic); ingest uses UTF-8-boundary-safe copy; FriBidi working sets are 4096 codepoints.
- **Scaling** — `computeScale(min(sw/1280,sh/720)∈[0.4,3.0])` derives all layout/fonts; `fontScale` multiplies; window minimum 960×600.
- **Theming** — 4 static `Theme`s; `cycleTheme()` wraps; `applyTitleBarTheme()` via runtime `dwmapi.dll`; footer status tints accent on errors.
- **Dashboard** — greeting + `nextPrayerInfo()` countdown/progress + date-seeded random Ayah of the Day + Hadith-of-day + continue-reading with relative times.
- **Finder** — one query box, two engines: surah fuzzy/prefix (`paletteFilter`) and ayah fuzzy (`runSearch`: lowercased, translation + surah-name `+500`, `qsort` desc, top 15, 2-char min); `Tab` switches; `searchAppendChar/Backspace/MoveSelection` seams.
- **Screensaver/cat/alarm** — 6f @ 0.15s 2.5x; `checkPrayerAlerts()` fires reminder at T-5min and `firePrayerAlarm()` (screensaver + Azan) at T-0, once per (day, prayer); rings + clock + live next-prayer line.
- **Bookmarks** — tag-only SQLite rows (`ORDER BY timestamp DESC, id DESC`); `b` opens the tag editor; `d` deletes with feedback.
- **Config** — `data/config.ini` 11 keys (`latitude/longitude/calcMethod/language/lastSurah/lastAyah/vimMotions/fontScale/idleSeconds/autoResume/theme`); `fgets`-parsed; Dhaka defaults.

## Team

| Member | GitHub |
|---|---|
| MD. Sadman Saif | [@HyperZx2O](https://github.com/HyperZx2O) |
| Afra Tasfia | [@afraNoOneAT](https://github.com/afraNoOneAT) |
| Montaha Zaman | [@yvonnieeez](https://github.com/yvonnieeez) |

---

*Quran text sourced from [AlQuran.cloud](https://alquran.cloud) — used under their public API terms. Arabic typesetting uses the [Amiri](https://www.amirifont.org/) font by Khaled Hosny.*
