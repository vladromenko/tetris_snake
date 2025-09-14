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

ifeq ($(COVERAGE),1)
CFLAGS   += -O0 --coverage
CXXFLAGS += -O0 --coverage
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

.PHONY: all install uninstall clean dvi dist test menu leaks gcov_report

all: $(BINS)
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
install: all
	install -d "$(BINDIR)"
	for b in $(BINS); do [ -f "$$b" ] && install -m0755 "$$b" "$(BINDIR)/$$b"; done
	@echo "Installed to $(BINDIR)"
uninstall:
	for b in $(BINS); do rm -f "$(BINDIR)/$$b"; done
	@echo "Uninstalled from $(BINDIR)"

DOXYFILE ?= Doxyfile
DOCS_DIR ?= build/docs
LATEX_DIR ?= $(DOCS_DIR)/latex
DVI_OUT ?= $(DOCS_DIR)/refman.dvi
PDF_OUT ?= $(DOCS_DIR)/refman.pdf
dvi: $(DOXYFILE)
	@mkdir -p "$(DOCS_DIR)"
	@DOXYGEN_OUTPUT_DIRECTORY="$(DOCS_DIR)" DOXYGEN_LATEX_OUTPUT="latex" doxygen "$(DOXYFILE)"
	@cd "$(LATEX_DIR)" && latex -interaction=nonstopmode refman.tex >/dev/null || true
	@cp -f "$(LATEX_DIR)/refman.dvi" "$(DVI_OUT)" 2>/dev/null || true
	@echo "DVI: $(DVI_OUT)"
	@cd "$(LATEX_DIR)" && pdflatex -interaction=nonstopmode refman.tex >/dev/null || true
	@cp -f "$(LATEX_DIR)/refman.pdf" "$(PDF_OUT)" 2>/dev/null || true
	@echo "PDF: $(PDF_OUT)"
	@{ command -v xdg-open >/dev/null 2>&1 && xdg-open "$(PDF_OUT)" || open "$(PDF_OUT)"; } >/dev/null 2>&1 || true

PROJECT_NAME ?= BrickGame
VERSION ?= 1.0
dist:
	@rm -rf build/dist && mkdir -p build/dist/$(PROJECT_NAME)-$(VERSION)
	@cp -R brick_game gui Makefile $(if $(wildcard $(DOXYFILE)),$(DOXYFILE),) build/dist/$(PROJECT_NAME)-$(VERSION)/
	@tar -czf build/$(PROJECT_NAME)-$(VERSION).tar.gz -C build/dist $(PROJECT_NAME)-$(VERSION)
	@rm -rf build/dist
	@echo "Archive: build/$(PROJECT_NAME)-$(VERSION).tar.gz"

BREW_GTEST := $(shell brew --prefix googletest 2>/dev/null)
GTEST_ROOT ?= $(if $(BREW_GTEST),$(BREW_GTEST),/opt/homebrew/opt/googletest)
GTEST_INCS := -I$(GTEST_ROOT)/include
GTEST_LIBS := -L$(GTEST_ROOT)/lib -lgtest -lgtest_main -lpthread

TEST_DIR   := test
TEST_BIN   := build/tests/snake_tests
TEST_SRCS  := $(TEST_DIR)/test_snake_fsm.cpp
UNIT_SRCS  := brick_game/snake/s_core.cpp \
              brick_game/snake/s_input.cpp \
              brick_game/snake/s_logic.cpp
TEST_OBJS  := $(TEST_SRCS:.cpp=.o) $(UNIT_SRCS:.cpp=.o)

$(TEST_BIN): $(TEST_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(GTEST_INCS) $^ $(GTEST_LIBS) -o $@

$(TEST_DIR)/%.o: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GTEST_INCS) -I. -Ibrick_game/snake -c $< -o $@

PKG_CONFIG ?= pkg-config
CHECK_CFLAGS := $(shell $(PKG_CONFIG) --cflags check 2>/dev/null)
CHECK_LIBS   := $(shell $(PKG_CONFIG) --libs   check 2>/dev/null)
ifeq ($(strip $(CHECK_LIBS)),)
  CHECK_LIBS := -lcheck -lm -lpthread
endif

CHECK_WARN_SUPPRESS ?= -Wno-gnu-zero-variadic-macro-arguments

TETRIS_TESTS := build/tests/tetris_tests
TETRIS_TEST_SOURCES := $(sort \
  $(wildcard test/tetris/*.c) \
  $(wildcard test/tetris_*.c) \
  $(wildcard test/*_tetris.c) \
  $(wildcard test/tetris_test.c))
TETRIS_TEST_OBJS := $(patsubst test/%.c,build/tests/%.o,$(TETRIS_TEST_SOURCES))

build/tests/%.o: test/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) $(CHECK_WARN_SUPPRESS) -Ibrick_game/tetris -c $< -o $@

$(TETRIS_TESTS): $(TETRIS_OBJS) $(TETRIS_TEST_OBJS)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CHECK_CFLAGS) $^ $(CHECK_LIBS) -o $@

test: $(TEST_BIN) $(TETRIS_TESTS)
	$(TEST_BIN) --gtest_color=yes
	CK_FORK=no $(TETRIS_TESTS)

LEAKS_BINS ?= $(BINS)
leaks-%: %
	@echo "Checking leaks for $<..."
	@{ command -v script >/dev/null 2>&1 && printf 'q' | script -q /dev/null -c "leaks -atExit -- ./$<" || leaks -atExit -- ./$<; } 2>&1 | grep 'LEAK:' || echo "(no explicit LEAK lines)"
leaks: $(addprefix leaks-,$(LEAKS_BINS))
	@echo "Done leaks checks."

menu:
	@echo "1) Snake (CLI)"; echo "2) Tetris (CLI)"; echo "3) Snake (Desktop)"; echo "4) Tetris (Desktop)"; echo "5) Tests"; echo "6) Exit"; \
	read -r a; case $$a in \
	1) $(MAKE) snake_console && ./snake_console;; \
	2) $(MAKE) tetris_console && ./tetris_console;; \
	3) $(MAKE) snake_desktop && ./snake_desktop;; \
	4) $(MAKE) tetris_desktop && ./tetris_desktop;; \
	5) $(MAKE) test;; 6) echo Bye;; *) echo Unknown;; esac

clean:
	@rm -f $(BINS) $(TEST_BIN) $(TETRIS_TESTS) \
		gui/cli/*.o gui/desktop/*.o \
		brick_game/tetris/*.o brick_game/snake/*.o \
		test/*.o build/tests/*.o gui/desktop/moc_*.cpp *.txt
	@rm -rf build/tests build/docs build/coverage
	@find . -name '*.gcda' -delete 2>/dev/null || true
	@find . -name '*.gcno' -delete 2>/dev/null || true
	@find . -name '*.gcov' -delete 2>/dev/null || true

LCOV ?= lcov
GENHTML ?= genhtml
COVERAGE_DIR ?= build/coverage
GCOV_TOOL ?= $(shell command -v llvm-cov >/dev/null 2>&1 && echo "llvm-cov gcov" || echo "gcov")
LCOV_FLAGS ?= --ignore-errors inconsistent,unsupported,format,unused --rc derive_function_end_line=0

gcov_report:
	@echo "Generating coverage report..."
	@rm -rf "$(COVERAGE_DIR)"
	@find . -name '*.gcda' -delete 2>/dev/null || true
	$(MAKE) clean >/dev/null
	$(MAKE) COVERAGE=1 test
	@mkdir -p "$(COVERAGE_DIR)"
	@$(LCOV) --version >/dev/null 2>&1 || { echo "lcov not found. Install lcov (e.g., brew install lcov)."; exit 1; }
	@$(GENHTML) --version >/dev/null 2>&1 || { echo "genhtml not found. Install lcov (e.g., brew install lcov)."; exit 1; }
	@$(LCOV) $(LCOV_FLAGS) --gcov-tool "$(GCOV_TOOL)" --capture --directory . --output-file "$(COVERAGE_DIR)/coverage.info" >/dev/null
	@$(LCOV) $(LCOV_FLAGS) --remove  "$(COVERAGE_DIR)/coverage.info" \
		'*/test/*' '/usr/*' '*/usr/*' '*/Library/*' '*/build/*' '*/gui/*' \
		-o "$(COVERAGE_DIR)/coverage.info" >/dev/null
	@$(GENHTML) "$(COVERAGE_DIR)/coverage.info" --output-directory "$(COVERAGE_DIR)/html" >/dev/null
	@echo "Coverage report: $(COVERAGE_DIR)/html/index.html"
	@{ command -v xdg-open >/dev/null 2>&1 && xdg-open "$(COVERAGE_DIR)/html/index.html" || open "$(COVERAGE_DIR)/html/index.html"; } >/dev/null 2>&1 || true
