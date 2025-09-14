**A Little After‑Hours Build**
- I built this Snake and Tetris after work simply because it's fun. Bare‑bones, no engines — just ncurses/Qt, build and play.

**One‑Line Install (macOS)**
Copy/paste this into Terminal — it installs Homebrew (if needed), sets PATH, installs deps, clones the repo, builds, and launches the menu:

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"; \
if [ -x /opt/homebrew/bin/brew ]; then eval "$(/opt/homebrew/bin/brew shellenv)"; else eval "$(/usr/local/bin/brew shellenv)"; fi; \
brew install ncurses qt git; \
git clone https://github.com/vladromenko/tetris_snake.git; cd tetris_snake; make
```

**Quick Start (macOS)**
- Get the code:
  - Clone: `git clone https://github.com/vladromenko/tetris_snake.git && cd tetris_snake`
- Install dependencies:
  - Xcode CLI tools (first time): `xcode-select --install`
  - Homebrew (one command):
    - `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`
  - Then install libraries: `brew install ncurses qt`
- Run:
  - `make` — opens the menu, pick 1–4 and play.
  - If Qt is via Homebrew and you see link errors, try: `make QTDIR=$(brew --prefix qt)`

**Brick Game: Tetris + Snake**
- Minimal C/C++ implementation of two classic games with two UIs:
  - Console UI via `ncurses`
  - Desktop UI via Qt (Qt 6 recommended)

**Games**
- Snake: Classic grid snake with speed ramp and score persistence.
- Tetris: Classic falling blocks with preview, score, and levels.

**Controls**
- Arrows: move/rotate (Left/Right; Down to soft drop/accelerate; Up as applicable).
- Space: action/rotate/accelerate (hold in CLI to accelerate in Snake).
- Enter: start.
- P: pause/resume.
- Q or Esc: quit.

**Requirements (macOS)**
- C/C++ toolchain: Apple Clang or GCC.
- Console UI: `ncurses` (via Homebrew).
- Desktop UI: Qt 6 (via Homebrew or Qt Online Installer).

**Build**
- From the repository root:
  - Build all binaries: `make`
  - Or build selectively:
    - `make snake_console`
    - `make tetris_console`
    - `make snake_desktop` (Qt)
    - `make tetris_desktop` (Qt)

**Run**
- Easiest: `make` — opens the interactive menu and runs the selected game.
- Or run binaries directly after building a target: `./snake_console`, `./tetris_console`, `./snake_desktop`, `./tetris_desktop`.

**Project Layout**
- `brick_game/snake`: game logic and API glue for Snake (`snake.h`, `s_*.cpp`).
- `brick_game/tetris`: game logic and API glue for Tetris (`t_*.c`).
- `gui/cli`: console UI (ncurses) shared by both games.
- `gui/desktop`: Qt 6 desktop UI shared by both games.
- `Makefile`: top-level build and run targets.

**Configuration (Qt on macOS)**
- The Makefile links Qt using Apple frameworks and expects Qt at `$(HOME)/Qt/6.x/macos` by default.
- If Qt is installed via Homebrew, pass the prefix at build time:
  - `make QTDIR=$(brew --prefix qt)`
- The Makefile locates `moc` under `$(QTDIR)/bin` or `$(QTDIR)/libexec` automatically.
 

**Data & Persistence**
- Snake high score persists in `snake_highscore.txt` (created in the working directory). Delete the file to reset.

**Clean**
- Remove build outputs and intermediates: `make clean`

**Troubleshooting**
- Qt not found (macOS): pass `QTDIR` explicitly: `make QTDIR=/Users/you/Qt/6.x.x/macos`
- `moc` not found: ensure Qt is installed; `$(QTDIR)/bin/moc` or `$(QTDIR)/libexec/moc` should exist.
- Linker errors with `ncurses`: install dev headers; on some distros use `-lncursesw` (adjust Makefile if needed).

**License**
- Educational/demo project; no specific license declared.
A Little After‑Hours Build
- This is a bare‑bones, evenings-and-weekends build of Tetris and Snake I hacked together for fun after work. No engines, no heavy frameworks beyond ncurses/Qt — just compile and play.

Quick Start (Download → Build → Play)
- Get the code:
  - Option A: Clone — `git clone <this‑repo‑url>` and `cd` into it.
  - Option B: Download ZIP — use the green “Code” button (or a release ZIP), unzip, and `cd` into the folder.
- Install deps (see details below):
  - macOS: `brew install ncurses qt`
  - Linux (CLI only by default): install your distro’s ncurses dev package(s).
- Build and run:
  - `make menu` → pick 1–4 and play.
  - Or build specific targets and run binaries directly (see sections below).
