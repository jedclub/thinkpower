CXX ?= g++

# Base Architecture & Native SIMD Flags for AMD Ryzen (Zen 2 Renoir)
NATIVE_SIMD_FLAGS = -O3 \
                    -march=native \
                    -mtune=native \
                    -mavx2 \
                    -mfma \
                    -mbmi2 \
                    -fno-plt \
                    -ftree-vectorize \
                    -mprefer-vector-width=256 \
                    -fomit-frame-pointer \
                    -fno-semantic-interposition \
                    -fstrict-aliasing \
                    -pipe

CXXFLAGS ?= $(NATIVE_SIMD_FLAGS) -std=c++17 -Wall -Wextra -Wno-deprecated-declarations
PKG_CONFIG ?= pkg-config

PKGS = ayatana-appindicator3-0.1 gtk+-3.0 gio-2.0
GTK_CFLAGS = $(shell $(PKG_CONFIG) --cflags $(PKGS))
GTK_LIBS = $(shell $(PKG_CONFIG) --libs $(PKGS))

LDFLAGS += -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now

BIN_DIR = bin
PROFILE_DIR = $(CURDIR)/build/pgo_data
TARGET = $(BIN_DIR)/power-tray
SRC = src/power-tray.cpp

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Standard build with LTO and AVX2
$(TARGET): $(SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -flto=auto $(GTK_CFLAGS) $< $(GTK_LIBS) $(LDFLAGS) -flto=auto -s -o $@

# ------------------------------------------------------------------------------
# PGO (Profile-Guided Optimization) Pipeline
# ------------------------------------------------------------------------------
pgo: $(SRC) | $(BIN_DIR)
	@echo "========================================================="
	@echo "  🔥 [1/3] Building Instrumentation Binary (-fprofile-generate)"
	@echo "========================================================="
	rm -rf $(PROFILE_DIR)
	mkdir -p $(PROFILE_DIR)
	$(CXX) $(CXXFLAGS) -fprofile-generate=$(PROFILE_DIR) $(GTK_CFLAGS) $< $(GTK_LIBS) $(LDFLAGS) -o $(TARGET)
	@echo "========================================================="
	@echo "  🎯 [2/3] Profiling & Training Workload (500,000 runs)"
	@echo "========================================================="
	$(TARGET) --train
	@echo "========================================================="
	@echo "  ⚡ [3/3] Building Final Optimized Binary (-fprofile-use)"
	@echo "========================================================="
	$(CXX) $(CXXFLAGS) -flto=auto -fprofile-use=$(PROFILE_DIR) -fprofile-correction $(GTK_CFLAGS) $< $(GTK_LIBS) $(LDFLAGS) -flto=auto -s -o $(TARGET)
	rm -rf $(PROFILE_DIR)
	@echo "========================================================="
	@echo "  ✨ Extreme PGO + AVX2 + LTO Build Complete: $(TARGET)"
	@echo "========================================================="

clean:
	rm -rf $(BIN_DIR) build src/power-tray

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/power-tray
	install -m 755 src/power-profile-manager $(DESTDIR)/usr/local/bin/power-profile-manager

.PHONY: all clean install pgo
