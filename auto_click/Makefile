# Makefile for auto_click
# 编译 click_core C 核心库

CC ?= gcc
CFLAGS := -shared -fPIC -Wall -O2 $(shell pkg-config --cflags libevdev 2>/dev/null || echo "-I/usr/include/libevdev-1.0/")
LDFLAGS := $(shell pkg-config --libs libevdev 2>/dev/null || echo "-levdev") -lpthread

TARGET := libclick_core.so
SRC := click_core.c
INC := click_core.h

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SRC) $(INC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -Dm755 $(TARGET) /usr/local/lib/$(TARGET)
	ldconfig

uninstall:
	rm -f /usr/local/lib/libclick_core.so
	ldconfig
