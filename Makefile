CXX ?= g++
CXXFLAGS ?= -O3 -std=c++17 -Wall -Wextra -Wno-deprecated-declarations
PKG_CONFIG ?= pkg-config

PKGS = ayatana-appindicator3-0.1 gtk+-3.0 gio-2.0
CXXFLAGS += $(shell $(PKG_CONFIG) --cflags $(PKGS))
LIBS += $(shell $(PKG_CONFIG) --libs $(PKGS))

BIN_DIR = bin
TARGET = $(BIN_DIR)/power-tray
SRC = src/power-tray.cpp

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $< $(LIBS) -o $@

clean:
	rm -rf $(BIN_DIR) src/power-tray

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/power-tray
	install -m 755 src/power-profile-manager $(DESTDIR)/usr/local/bin/power-profile-manager

.PHONY: all clean install
