# DIVseed — alpha 0.1

A small Linux text editor derived from DIV Games Studio, preserving its bitmap fonts, floating menus and desktop character. (AI PROJECT)

## Build

Requires a C compiler, Make, pkg-config and **SDL2 development libraries (2.0.18+)**. On Debian/Ubuntu: `sudo apt install build-essential pkg-config libsdl2-dev`.

```sh
make
./divseed
```

If SDL2 is missing, the build explains what to install. `make deps` offers to install the required packages on Debian/Ubuntu, Fedora or Arch, asking first. `make run` starts the editor; `make clean` removes build files. Keep `assets/` beside the executable.

## Use

Ctrl+N: new · Ctrl+O: open · Ctrl+S: save · Ctrl+Shift+S: save as · Ctrl+W: close. Ctrl+Z/Y: undo/redo. Ctrl+C/X/V: clipboard. F1: help.

Drag titles to move windows and menus; resize from the bottom-right corner. Windows dim when covered, show a white border while moving, and open with an expanding outline. The original DIV pointer and raised buttons are retained. Maximizing the Linux window enlarges the desktop continuously again; fractional scales can make bitmap strokes uneven.

UTF-8 editing, multiple documents, file browsing and safe temporary-file saves are supported. This is an alpha: keep backups. Mixed line endings are rejected; tabs display as one-cell markers. Unicode uses one cell per scalar, with incomplete glyph coverage and no shaping or grapheme layout.

GPLv3. Based on [DIV Games Studio](https://github.com/DIVGAMES/DIV-Games-Studio). Attribution and font notices: `NOTICE`, `LICENSE`, `assets/UNICODE-FONT-LICENSE`.
