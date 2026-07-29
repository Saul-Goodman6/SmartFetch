CC = gcc
CFLAGS = -Wall -O2 -I./H_code
GIT_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")
CFLAGS += -DSF_VERSION=\"$(GIT_HASH)\"

SRC = C_code/main.c C_code/ui.c C_code/collect.c
OBJ = $(SRC:.c=.o)
TARGET = sfetch

PREFIX ?= /usr/local
DATA_DIR = /usr/share/smartfetch

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	mkdir -p $(DESTDIR)$(DATA_DIR)/Ascii_art
	cp -r Ascii_art/* $(DESTDIR)$(DATA_DIR)/Ascii_art/
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(TARGET) $(DESTDIR)$(PREFIX)/bin/

clean:
	rm -f C_code/*.o $(TARGET)

.PHONY: all install clean