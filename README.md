# Ayatika

A desktop Quran study application written entirely in C, powered by RayLib.

Built as a Structured Programming course project at IUT (Islamic University of Technology) by a team of 3.

## Features

- Full Quran navigation across all 114 Surahs and 6,236 Ayahs
- Uthmani and Imlaei Arabic script rendering
- Vim-style keyboard controls
- Real-time prayer times (Waqt) with prohibited time windows
- Multiple translations (English, Bengali)
- Bookmarks with tags, notes, and highlights
- Recitation audio playback
- Major Hadiths panel (Sahih Bukhari, Sahih Muslim)
- Fuzzy search across the entire Quran
- Azan screensaver with audio on idle
- Multiple themes
- Ambient nature sounds
- Sleeping cat idle animation

## Tech Stack

| Library | Purpose |
|---|---|
| RayLib 5.x | Window, rendering, input, audio |
| libcurl | HTTP requests to AlQuran.cloud API |
| cJSON | JSON parsing |
| SQLite3 | Bookmark and note storage |
| FriBidi | Arabic RTL text reordering |
| fts_fuzzy_match | Fuzzy search engine |

## Building

```bash
make
make run
```

Requires RayLib, libcurl, sqlite3, and fribidi development libraries installed on your system.

## Project Structure

See `ARCHITECTURE.md` for the full file layout, team role breakdown, and Git workflow.

## Team

| Role | Owns |
|---|---|
| Backend Engineer | `quran.c`, `prayer.c`, `db.c`, `surah_meta.c` |
| Frontend Engineer | `main.c`, `ui.c`, `theme.c`, `input.c` |
| Systems & Features Engineer | `audio.c`, `screensaver.c`, `search.c` |

## License

Quran text and translations sourced from [AlQuran.cloud](https://alquran.cloud), used under their public API terms.
