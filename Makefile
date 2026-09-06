ifeq ($(origin CC),default)
CC := m68k-amigaos-gcc
endif

CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -noixemul
CPPFLAGS ?= -Isrc
LDFLAGS ?= -noixemul

TARGET := AmiDisk
SOURCES := src/main.c src/core/ad_version.c src/io/trackdisk/trackdisk.c src/io/adf/adf.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

check:
	python3 tools/check_repo.py
	python3 tools/check_m1.py
	python3 tools/check_m2.py

clean:
	rm -f $(OBJECTS) $(TARGET)
