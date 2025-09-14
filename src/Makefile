CC := gcc
CXX := g++
CSTD := -std=c11
CXXSTD := -std=c++20
WARN := -Wall -Wextra -Wpedantic
OPT := -O2
CFLAGS := $(CSTD) $(WARN) $(OPT) -Ibrick_game/snake -Ibrick_game/tetris -Igui/desktop
CXXFLAGS := $(CXXSTD) $(WARN) $(OPT) -Ibrick_game/snake -Ibrick_game/tetris -Igui/desktop
NCURSES := -lncurses
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
# Prefer Homebrew Qt if available; fallback to user-local Qt installer path
QTBREW := $(shell command -v brew >/dev/null 2>&1 && brew --prefix qt 2>/dev/null)
QTDIR ?= $(if $(QTBREW),$(QTBREW),$(HOME)/Qt/6.9.1/macos)
QT_INCS := -F$(QTDIR)/lib -I$(QTDIR)/lib/QtWidgets.framework/Headers -I$(QTDIR)/lib/QtGui.framework/Headers -I$(QTDIR)/lib/QtCore.framework/Headers -I$(QTDIR)/include
QT_LIBS := -F$(QTDIR)/lib -framework QtWidgets -framework QtGui -framework QtCore -Wl,-rpath,$(QTDIR)/lib
MOC := $(shell [ -x "$(QTDIR)/bin/moc" ] && echo "$(QTDIR)/bin/moc" || { [ -x "$(QTDIR)/libexec/moc" ] && echo "$(QTDIR)/libexec/moc" || echo moc; })
endif

# coverage flags removed

SNAKE_CPP := brick_game/snake/s_core.cpp brick_game/snake/s_input.cpp brick_game/snake/s_logic.cpp brick_game/snake/s_api.cpp
TETRIS_C  := brick_game/tetris/t_core.c brick_game/tetris/t_input.c brick_game/tetris/t_logic.c brick_game/tetris/t_api.c
CLI_C     := gui/cli/draw.c gui/cli/main.c
DESKTOP_CPP := gui/desktop/view.cpp gui/desktop/main.cpp
MOC_HDR   := gui/desktop/view.h
MOC_SRCS  := $(MOC_HDR:gui/desktop/%.h=gui/desktop/moc_%.cpp)
MOC_OBJS  := $(MOC_SRCS:.cpp=.o)

SNAKE_OBJS  := $(SNAKE_CPP:.cpp=.o)
TETRIS_OBJS := $(TETRIS_C:.c=.o)
CLI_OBJS    := $(CLI_C:.c=.o)
DESKTOP_OBJS:= $(DESKTOP_CPP:.cpp=.o) $(MOC_OBJS)
$(DESKTOP_OBJS): CXXFLAGS += $(QT_INCS)

BINS := snake_console tetris_console snake_desktop tetris_desktop

.PHONY: all deps deps-macos deps-linux install uninstall clean menu leaks \
        run-snake-cli run-tetris-cli run-snake-desktop run-tetris-desktop

all: deps $(BINS)
snake_console: $(CLI_OBJS) $(SNAKE_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(NCURSES) -o $@
tetris_console: $(CLI_OBJS) $(TETRIS_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(NCURSES) -o $@
snake_desktop: $(DESKTOP_OBJS) $(SNAKE_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(QT_LIBS) -o $@
tetris_desktop: $(DESKTOP_OBJS) $(TETRIS_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(QT_LIBS) -o $@
gui/desktop/moc_%.cpp: gui/desktop/%.h
	$(MOC) $(QT_INCS) $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Dependency installation helpers ---
# Runs before builds to ensure ncurses and Qt are available.
deps:
ifeq ($(UNAME_S),Darwin)
	@$(MAKE) --no-print-directory deps-macos
else
	@$(MAKE) --no-print-directory deps-linux || true
endif

deps-macos:
	@command -v brew >/dev/null 2>&1 || { echo "Homebrew not found. Install it from https://brew.sh"; exit 1; }
	@brew list --versions ncurses >/dev/null 2>&1 || brew install ncurses
	@brew list --versions qt >/dev/null 2>&1 || brew install qt
	@echo "Dependencies installed (macOS): ncurses, qt"

deps-linux:
	@echo "Attempting to install ncurses and Qt (Linux)"
	@if command -v apt-get >/dev/null 2>&1; then \
		sudo apt-get update && sudo apt-get install -y libncurses5-dev libncursesw5-dev qt6-base-dev qt6-base-dev-tools || sudo apt-get install -y qtbase5-dev qtbase5-dev-tools; \
	elif command -v dnf >/dev/null 2>&1; then \
		sudo dnf install -y ncurses-devel qt6-qtbase-devel qt6-qtbase-gui || sudo dnf install -y qt5-qtbase-devel; \
	elif command -v pacman >/dev/null 2>&1; then \
		sudo pacman -Sy --noconfirm ncurses qt6-base || sudo pacman -Sy --noconfirm qt5-base; \
	else \
		echo "Package manager not detected. Please install ncurses and Qt manually."; \
	fi

PREFIX ?= $(HOME)/.local
BINDIR := $(PREFIX)/bin
install: all
	install -d "$(BINDIR)"
	for b in $(BINS); do [ -f "$$b" ] && install -m0755 "$$b" "$(BINDIR)/$$b"; done
	@echo "Installed to $(BINDIR)"
uninstall:
	for b in $(BINS); do rm -f "$(BINDIR)/$$b"; done
	@echo "Uninstalled from $(BINDIR)"

## docs target removed

## dist target removed

## tests removed

LEAKS_BINS ?= $(BINS)
leaks-%: %
	@echo "Checking leaks for $<..."
	@{ command -v script >/dev/null 2>&1 && printf 'q' | script -q /dev/null -c "leaks -atExit -- ./$<" || leaks -atExit -- ./$<; } 2>&1 | grep 'LEAK:' || echo "(no explicit LEAK lines)"
leaks: $(addprefix leaks-,$(LEAKS_BINS))
	@echo "Done leaks checks."

menu:
	@echo "1) Snake (CLI)"; echo "2) Tetris (CLI)"; echo "3) Snake (Desktop)"; echo "4) Tetris (Desktop)"; echo "5) Exit"; \
	read -r a; case $$a in \
	1) $(MAKE) snake_console && ./snake_console;; \
	2) $(MAKE) tetris_console && ./tetris_console;; \
	3) $(MAKE) snake_desktop && ./snake_desktop;; \
	4) $(MAKE) tetris_desktop && ./tetris_desktop;; \
	5) echo Bye;; *) echo Unknown;; esac

# Run targets
run-snake-cli: snake_console
	@./snake_console

run-tetris-cli: tetris_console
	@./tetris_console

run-snake-desktop: snake_desktop
	@./snake_desktop

run-tetris-desktop: tetris_desktop
	@./tetris_desktop

clean:
	@rm -f $(BINS) \
		gui/cli/*.o gui/desktop/*.o \
		brick_game/tetris/*.o brick_game/snake/*.o \
		gui/desktop/moc_*.cpp *.txt
	@rm -rf build/docs build/coverage
	@find . -name '*.gcda' -delete 2>/dev/null || true
	@find . -name '*.gcno' -delete 2>/dev/null || true
	@find . -name '*.gcov' -delete 2>/dev/null || true

## coverage report removed
