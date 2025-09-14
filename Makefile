CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -flto -Isrc
LDFLAGS = -flto

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
SRCS = src/hyprlax.c $(PROTOCOL_SRCS)
OBJS = $(SRCS:.c=.o)
TARGET = hyprlax

all: $(TARGET)

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

clean:
	rm -f $(TARGET) $(OBJS) $(PROTOCOL_SRCS) $(PROTOCOL_HDRS)
	rm -rf protocols/*.o src/*.o

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
DESTDIR ?=

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

install-user: $(TARGET)
	install -Dm755 $(TARGET) ~/.local/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall-user:
	rm -f ~/.local/bin/$(TARGET)

# Test suite
TEST_SRCS = tests/test_hyprlax.c
TEST_TARGET = tests/test_hyprlax

$(TEST_TARGET): $(TEST_SRCS) $(TARGET)
	$(CC) $(CFLAGS) $(TEST_SRCS) -lm -o $(TEST_TARGET)

test: $(TEST_TARGET)
	@echo "Running test suite..."
	@./$(TEST_TARGET)

clean-tests:
	rm -f $(TEST_TARGET)

.PHONY: all clean install install-user uninstall uninstall-user test clean-tests