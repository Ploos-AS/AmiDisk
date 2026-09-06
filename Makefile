CC ?= m68k-amigaos-gcc
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -noixemul
CPPFLAGS ?= -Isrc
LDFLAGS ?= -noixemul

TARGET := AmiDisk
SOURCES := src/main.c src/core/ad_version.c src/io/trackdisk/trackdisk.c
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

clean:
	rm -f $(OBJECTS) $(TARGET)
