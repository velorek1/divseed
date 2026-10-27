# DIVseed — alpha 0.4

A small Linux text editor derived from DIV Games Studio, preserving its bitmap fonts, floating menus and desktop character. (AI EXPERIMENT)

## Build

Requires a C compiler, Make, pkg-config and **SDL2 development libraries (2.0.18+)**. On Debian/Ubuntu: `sudo apt install build-essential pkg-config libsdl2-dev`.

```sh
make
./divseed
./divseed file.txt
```

If SDL2 is missing, the build explains what to install. `make deps` offers to install the required packages on Debian/Ubuntu, Fedora or Arch, asking first. `make run` starts the editor; `make clean` removes build files. Keep `assets/` beside the executable.

## Use

Ctrl+N: new · Ctrl+O: open · Ctrl+S: save · Ctrl+Shift+S: save as · Ctrl+W: close. Ctrl+Z/Y: undo/redo. Ctrl+C/X/V: clipboard. F1: help.

Page Up/Down moves by the visible editor height; Shift extends selection. The caret blinks and resets when typing or navigating. Text fields support clicking, Left/Right, Home/End, Backspace/Delete and Ctrl+A to clear.

Edit → Find (Ctrl+F) and Replace (Ctrl+H) search literal, case-sensitive UTF-8 text in the current document, wrapping at the end. Replace changes the matching selection (or finds the next match); Replace all is one undoable operation. Empty replacement deletes matches. Tab switches fields; Enter finds the next match; Esc closes. Search fields accept up to 511 bytes; regex, case folding and multi-file search are not provided.

Starts at **960×600**. Display offers common resolutions, custom sizes from 800×600 to 3840×2160, and fullscreen. **Alt+Enter** toggles desktop fullscreen. Up to six custom sizes can be added for the current session; settings are not saved between runs.

Open files with `./divseed file.txt` (multiple files and quoted paths work; use `--` before names starting with `-`). TAB inserts four spaces as one undoable edit. Existing tab bytes are preserved and shown as blank cells.

The blue **C reference** button opens scrollable keyword reminders for C11. Scope follows the [WG14 C11 draft](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf); this is a brief guide, not the full specification.

C syntax colors are automatic for `.c` and `.h` files. Use View → Toggle C colors for untitled files or to turn them off. Keywords, comments, strings/characters, numbers and preprocessor directives are colored; this is lexical highlighting, not a compiler. Selection remains clearly visible.

Drag titles to move windows and menus; resize from the bottom-right corner. Windows dim when covered, show a white border while moving, and open with an expanding outline. The original DIV pointer and raised buttons are retained. Maximizing the Linux window enlarges the desktop continuously again; fractional scales can make bitmap strokes uneven.

UTF-8 editing, multiple documents, file browsing and safe temporary-file saves are supported. This is an alpha: keep backups. Mixed line endings are rejected. Unicode uses one cell per scalar, with incomplete glyph coverage and no shaping or grapheme layout.

GPLv3. Based on [DIV Games Studio](https://github.com/DIVGAMES/DIV-Games-Studio). Attribution and font notices: `NOTICE`, `LICENSE`, `assets/UNICODE-FONT-LICENSE`.
![Alt text](divseed.jpg?raw=true "Demo")
