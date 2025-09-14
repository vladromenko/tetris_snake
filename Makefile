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
QTDIR ?= $(HOME)/Qt/6.9.1/macos
QT_INCS := -F$(QTDIR)/lib -I$(QTDIR)/lib/QtWidgets.framework/Headers -I$(QTDIR)/lib/QtGui.framework/Headers -I$(QTDIR)/lib/QtCore.framework/Headers -I$(QTDIR)/include
QT_LIBS := -F$(QTDIR)/lib -framework QtWidgets -framework QtGui -framework QtCore -Wl,-rpath,$(QTDIR)/lib
MOC := $(shell [ -x "$(QTDIR)/bin/moc" ] && echo "$(QTDIR)/bin/moc" || { [ -x "$(QTDIR)/libexec/moc" ] && echo "$(QTDIR)/libexec/moc" || echo moc; })
endif


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

.PHONY: all clean menu snake_console tetris_console snake_desktop tetris_desktop

all: menu
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

PREFIX ?= $(HOME)/.local
BINDIR := $(PREFIX)/bin

menu:
	@echo "1) Snake (CLI)"; echo "2) Tetris (CLI)"; echo "3) Snake (Desktop)"; echo "4) Tetris (Desktop)"; echo "5) Exit"; \
	read -r a; case $$a in \
	1) $(MAKE) snake_console && ./snake_console;; \
	2) $(MAKE) tetris_console && ./tetris_console;; \
	3) $(MAKE) snake_desktop && ./snake_desktop;; \
	4) $(MAKE) tetris_desktop && ./tetris_desktop;; \
	5) echo Bye;; *) echo Unknown;; esac

clean:
	@rm -f $(BINS) \
		gui/cli/*.o gui/desktop/*.o \
		brick_game/tetris/*.o brick_game/snake/*.o \
		gui/desktop/moc_*.cpp *.txt
