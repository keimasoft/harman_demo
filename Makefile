# ─────────────────────────────────────────────────────────────────────────────
# Makefile – gst-video-pipeline
#
# Targets:
#   make              build both binaries (default)
#   make server       build only the server
#   make client       build only the client
#   make clean        remove build artefacts
#   make install      install binaries to PREFIX/bin  (default: /usr/local)
#   make uninstall    remove installed binaries
#
# Override the install prefix:   make install PREFIX=/opt/gst-demo
# ─────────────────────────────────────────────────────────────────────────────

CC      := gcc
PREFIX  := /usr/local
BINDIR  := $(PREFIX)/bin

SRCDIR  := src
BUILDDIR:= build

# ── pkg-config packages ───────────────────────────────────────────────────────
GST_PKGS        := gstreamer-1.0 gstreamer-video-1.0
RTSP_PKGS       := gstreamer-rtsp-server-1.0
AVDEC_PKGS      := gstreamer-1.0 gstreamer-video-1.0

# ── flags ────────────────────────────────────────────────────────────────────
COMMON_CFLAGS   := -Wall -Wextra -O2 -g \
                   $(shell pkg-config --cflags $(GST_PKGS))

SERVER_CFLAGS   := $(COMMON_CFLAGS) \
                   $(shell pkg-config --cflags $(RTSP_PKGS))

CLIENT_CFLAGS   := $(COMMON_CFLAGS)

SERVER_LDFLAGS  := $(shell pkg-config --libs $(GST_PKGS) $(RTSP_PKGS))
CLIENT_LDFLAGS  := $(shell pkg-config --libs $(AVDEC_PKGS))

# ── targets ──────────────────────────────────────────────────────────────────
.PHONY: all server client clean install uninstall check-deps

all: check-deps $(BUILDDIR)/server $(BUILDDIR)/client

check-deps:
	@pkg-config --exists $(GST_PKGS) $(RTSP_PKGS) || \
	  { echo "[ERROR] Missing pkg-config packages. See README for install instructions."; exit 1; }

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/server: $(SRCDIR)/server.c | $(BUILDDIR)
	$(CC) $(SERVER_CFLAGS) $< -o $@ $(SERVER_LDFLAGS)
	@echo "[OK]  built $@"

$(BUILDDIR)/client: $(SRCDIR)/client.c | $(BUILDDIR)
	$(CC) $(CLIENT_CFLAGS) $< -o $@ $(CLIENT_LDFLAGS)
	@echo "[OK]  built $@"

server: check-deps $(BUILDDIR)/server
client: check-deps $(BUILDDIR)/client

clean:
	rm -rf $(BUILDDIR)
	@echo "[OK]  cleaned"

install: all
	install -d $(BINDIR)
	install -m 755 $(BUILDDIR)/server $(BINDIR)/gst-video-server
	install -m 755 $(BUILDDIR)/client $(BINDIR)/gst-video-client
	@echo "[OK]  installed to $(BINDIR)"

uninstall:
	rm -f $(BINDIR)/gst-video-server $(BINDIR)/gst-video-client
	@echo "[OK]  uninstalled"
