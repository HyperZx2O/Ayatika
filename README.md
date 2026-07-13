# Ayatika — القرآن الكريم

> **A desktop Quran study application, written entirely in C, powered by RayLib.**
>
> *Built for CSE 4202 — Structured Programming II Laboratory, Islamic University of Technology.*

Ayatika is a keyboard-driven Quran reader with Arabic script rendering, multiple themes, bookmarking, and a curated Hadith panel. The frontend branch delivers the complete user interface and interaction system with zero external dependencies — just `gcc` and `make` on any Windows machine with MinGW-w64.

---

## Screens

| Screen | Description |
|---|---|
| **Dashboard** | Ayah of the day, prayer time overview, continue-reading widget, quick-access tiles |
| **Reading Hub** | Choose between Surah overview or Hadith panel |
| **Surah Overview** | Context, revelation type, ayah count, and key themes for the selected surah |
| **Ayah Reader** | Arabic text with optional English/Bengali translation, focus mode, inline bookmarking |
| **Hadith Panel** | Curated Hadiths from Bukhari and Muslim |
| **Search** | Search across Quranic text (UI placeholder — engine wired but awaiting backend integration) |
| **Bookmarks** | Saved ayah references with relative timestamps |
| **Settings** | Theme, font scale, screensaver timer, language, calculation method, latitude, navigation mode |
| **Screensaver** | Idle overlay with any-key-to-exit navigation back to origin screen |

## Keyboard Navigation

Ayatika supports two navigation modes — **Vim-style** (default) or **arrow keys** — switchable from Settings.

| Key | Action |
|---|---|
| `j` / `Down` | Move cursor down |
| `k` / `Up` | Move cursor up |
| `h` / `Left` | Move cursor left |
| `l` / `Right` | Move cursor right |
| `Enter` | Open selected item |
| `Esc` | Go back to previous screen |
| `g` / `Home` | Go to top / jump to dashboard |
| `G` / `End` | Go to bottom |
| `t` | Cycle theme |
| `s` | Open settings |
| `/` | Open search |
| `m` | Open bookmarks |
| `b` | Bookmark current ayah |
| `f` | Toggle focus mode |
| `F1` | Toggle help overlay |

## Themes

Ayatika ships with four hand-tuned color themes that also control the window title bar (dark/light):

| Theme | Background | Accent | Character |
|---|---|---|---|
| **Celestial Night** | Deep navy `#0C0E1C` | Gold `#D2AF5A` | Dark, scholarly |
| **Moonlit Garden** | Warm cream `#F8F2EB` | Rose `#B46464` | Light, warm |
| **Peacock Court** | Dark teal `#0A191C` | Coral `#D27864` | Rich, vibrant |
| **Amber Sanctum** | Warm brown `#372D23` | Emerald `#50AF78` | Earthy, calm |

Press **`t`** to cycle through themes — the title bar follows suit automatically.

## Tech Stack

| Component | Technology |
|---|---|
| Language | **C11** (GCC) |
| Graphics & Input | **RayLib 5.x** (via GLFW) |
| Arabic shaping | **FriBidi** (RTL text reordering) |
| Storage | **SQLite3** (vendored, for future bookmark/history persistence) |
| Fonts | **Amiri** (Arabic), **JetBrains Mono** (UI) |

## Quick Start

### Requirements

- A Windows machine (7 / 10 / 11)
- **MinGW-w64** with `gcc`, `windres`, and `make` on your system PATH

### Build & Run

```bash
git clone -b frontend https://github.com/HyperZx2O/Ayatika.git
cd Ayatika
make
make run
```

No additional dependencies to install — all libraries are statically linked and vendored in `lib/`. The only system DLL required (`glfw3.dll`) is included in the repository.

### Tests

```bash
make test
```

Runs 23 tests across mock data, theme system, screen enumeration, UI rendering, and input handling.

## Project Structure

```
Ayatika/
├── assets/              # Font files (Amiri, JetBrains Mono)
├── data/                # Runtime data (gitignored, created on first run)
├── lib/
│   ├── include/
│   │   ├── fribidi/     # Vendored FriBidi headers
│   │   └── raylib.h     # Vendored RayLib header
│   ├── sqlite3.c        # Vendored SQLite amalgamation
│   ├── sqlite3.h
│   ├── libraylib.a      # Static-linked RayLib
│   ├── libglfw3.dll.a   # GLFW import library
│   └── libfribidi.a     # Static-linked FriBidi
├── src/
│   ├── main.c           # Entry point, window init, main loop
│   ├── ui.c / ui.h      # All screen rendering (dashboard, reader, settings, etc.)
│   ├── input.c / input.h# Keyboard input handling and key bindings
│   ├── theme.c / theme.h# Color theme definitions and cycling
│   ├── quran.c / quran.h# Quran data structures, daily ayah logic
│   └── mock_data.c/h    # Built-in mock dataset (surahs, ayahs, hadiths)
├── tests/               # Phase test files
├── glfw3.dll            # GLFW runtime DLL (required at runtime)
├── ayatika.exe.manifest # Windows DPI / compatibility manifest
├── Makefile             # Zero-dependency build (just gcc + make)
└── ARCHITECTURE.md      # Full project architecture and team breakdown
```

## Architecture Notes

- **All rendering is immediate-mode** via RayLib — no retained UI state beyond the `AppState` struct.
- **Navigation uses a lightweight screen stack** — `pushScreen` / `goBack` with `previousScreen` tracking, plus a `settingsOrigin` field for screensaver back-navigation.
- **Arabic text** is shaped with FriBidi then loaded into a RayLib font atlas at startup, covering all codepoints needed across the loaded surahs.
- **Theming** — 4 themes stored as static `Color` structs. `cycleTheme()` advances the index; `applyTitleBarTheme()` calls `DwmSetWindowAttribute` (via runtime dynamic loading of `dwmapi.dll`) to match the native title bar.
- **The dashboard** shows a daily ayah selected by day-of-year modulo total ayahs, prayer time estimates, and a "continue reading" card driven by mock bookmarks.

---

*Quran text sourced from [AlQuran.cloud](https://alquran.cloud) — used under their public API terms. Arabic typesetting uses the [Amiri](https://www.amirifont.org/) font by Khaled Hosny.*
