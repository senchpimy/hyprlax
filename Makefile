CC = gcc
# Use generic architecture for CI compatibility
ifdef CI
CFLAGS = -Wall -Wextra -O2 -Isrc
LDFLAGS =
else
CFLAGS = -Wall -Wextra -O3 -march=native -flto -Isrc
LDFLAGS = -flto
endif

# Cross-compilation support
ifdef ARCH
ifeq ($(ARCH),aarch64)
CC = aarch64-linux-gnu-gcc
CFLAGS = -Wall -Wextra -O3 -flto -Isrc
endif
endif

# Package dependencies
PKG_DEPS = wayland-client wayland-protocols wayland-egl egl glesv2

# Get package flags
PKG_CFLAGS = $(shell pkg-config --cflags $(PKG_DEPS))
PKG_LIBS = $(shell pkg-config --libs $(PKG_DEPS))

# Wayland protocols
WAYLAND_PROTOCOLS_DIR = $(shell pkg-config --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER = $(shell pkg-config --variable=wayland_scanner wayland-scanner)

# Protocol files
XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml
LAYER_SHELL_PROTOCOL = protocols/wlr-layer-shell-unstable-v1.xml

# Generated protocol files  
PROTOCOL_SRCS = protocols/xdg-shell-protocol.c protocols/wlr-layer-shell-protocol.c
PROTOCOL_HDRS = protocols/xdg-shell-client-protocol.h protocols/wlr-layer-shell-client-protocol.h

# Source files
SRCS = src/hyprlax.c src/ipc.c $(PROTOCOL_SRCS)
OBJS = $(SRCS:.c=.o)
TARGET = hyprlax

# hyprlax-ctl client
CTL_SRCS = src/hyprlax-ctl.c
CTL_OBJS = $(CTL_SRCS:.c=.o)
CTL_TARGET = hyprlax-ctl

all: $(TARGET) $(CTL_TARGET)

# Generate protocol files
protocols/xdg-shell-protocol.c: $(XDG_SHELL_PROTOCOL)
	@mkdir -p protocols
	$(WAYLAND_SCANNER) private-code < $< > $@

protocols/xdg-shell-client-protocol.h: $(XDG_SHELL_PROTOCOL)
	@mkdir -p protocols
	$(WAYLAND_SCANNER) client-header < $< > $@

protocols/wlr-layer-shell-protocol.c: $(LAYER_SHELL_PROTOCOL)
	@mkdir -p protocols
	$(WAYLAND_SCANNER) private-code < $< > $@

protocols/wlr-layer-shell-client-protocol.h: $(LAYER_SHELL_PROTOCOL)
	@mkdir -p protocols
	$(WAYLAND_SCANNER) client-header < $< > $@

# Compile
%.o: %.c $(PROTOCOL_HDRS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(PKG_CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(PKG_LIBS) -lm -o $@

$(CTL_TARGET): $(CTL_OBJS)
	$(CC) $(LDFLAGS) $(CTL_OBJS) -o $@

clean:
	rm -f $(TARGET) $(CTL_TARGET) $(OBJS) $(CTL_OBJS) $(PROTOCOL_SRCS) $(PROTOCOL_HDRS)
	rm -rf protocols/*.o src/*.o

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DESTDIR ?=

install: $(TARGET) $(CTL_TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm755 $(CTL_TARGET) $(DESTDIR)$(BINDIR)/$(CTL_TARGET)

install-user: $(TARGET) $(CTL_TARGET)
	install -Dm755 $(TARGET) ~/.local/bin/$(TARGET)
	install -Dm755 $(CTL_TARGET) ~/.local/bin/$(CTL_TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(BINDIR)/$(CTL_TARGET)

uninstall-user:
	rm -f ~/.local/bin/$(TARGET)
	rm -f ~/.local/bin/$(CTL_TARGET)

# Test suite
TEST_SRCS = tests/test_hyprlax.c
TEST_TARGET = tests/test_hyprlax
IPC_TEST_SRCS = tests/test_ipc.c src/ipc.c
IPC_TEST_TARGET = tests/test_ipc

$(TEST_TARGET): $(TEST_SRCS) $(TARGET)
	$(CC) $(CFLAGS) $(TEST_SRCS) -lm -o $(TEST_TARGET)

$(IPC_TEST_TARGET): $(IPC_TEST_SRCS)
	$(CC) $(CFLAGS) $(IPC_TEST_SRCS) -o $(IPC_TEST_TARGET)

test: $(TEST_TARGET) $(IPC_TEST_TARGET)
	@echo "Running test suite..."
	@./$(TEST_TARGET)
	@echo "Running IPC test suite..."
	@./$(IPC_TEST_TARGET)

# Linting targets
lint:
	@if [ -x scripts/lint.sh ]; then \
		./scripts/lint.sh; \
	else \
		echo "Lint script not found. Please ensure scripts/lint.sh exists and is executable."; \
		exit 1; \
	fi

lint-fix:
	@if [ -x scripts/lint.sh ]; then \
		./scripts/lint.sh --fix; \
	else \
		echo "Lint script not found. Please ensure scripts/lint.sh exists and is executable."; \
		exit 1; \
	fi

clean-tests:
	rm -f $(TEST_TARGET) $(IPC_TEST_TARGET)

.PHONY: all clean install install-user uninstall uninstall-user test clean-tests lint lint-fix