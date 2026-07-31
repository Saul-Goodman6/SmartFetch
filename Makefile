CC = gcc
CFLAGS = -Wall -O2 -I./H_code
LDLIBS = -lcurl
GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
CFLAGS += -DSF_VERSION=\"$(GIT_HASH)\"

SRC = C_code/main.c C_code/ui.c C_code/collect.c
OBJ = $(SRC:.c=.o)
TARGET = sfetch

PREFIX ?= /usr/local
DATA_DIR = /usr/share/smartfetch

all: check-deps $(TARGET)

check-deps:
	@echo '#include <curl/curl.h>' | $(CC) -E - >/dev/null 2>&1 || { \
		echo "Error: libcurl development headers not found (curl/curl.h missing)."; \
		echo ""; \
		echo "Install them first:"; \
		echo "  Debian/Ubuntu : sudo apt install libcurl4-openssl-dev"; \
		echo "  Fedora        : sudo dnf install libcurl-devel"; \
		echo "  Arch          : sudo pacman -S curl"; \
		exit 1; \
	}

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	mkdir -p $(DESTDIR)$(DATA_DIR)/Ascii_art
	cp -r Ascii_art/* $(DESTDIR)$(DATA_DIR)/Ascii_art/
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(TARGET) $(DESTDIR)$(PREFIX)/bin/

clean:
	rm -f C_code/*.o $(TARGET)

.PHONY: all check-deps install clean
