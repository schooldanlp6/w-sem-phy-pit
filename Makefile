# ESP32 DEVKIT1 + HLK-LD2450 radar + SSD1306 OLED
#
# Thin wrapper around idf.py. If idf.py is not on PATH yet (you did not run
# `get_idf`), the ESP-IDF environment is sourced automatically from IDF_EXPORT.
#
#   make                 build the firmware
#   make flash           flash it (PORT=/dev/ttyUSB0 by default)
#   make monitor         open the serial monitor (Ctrl+] to quit)
#   make flash-monitor   both
#   make menuconfig      change pins, OLED address/size, ...
#   make test            host tests of parser + OLED layout (no board needed)
#   make preview         show the OLED screens as ASCII art in the terminal
#   make help            list all targets

SHELL      := /bin/bash
IDF_EXPORT ?= $(HOME)/esp-idf/export.sh
PORT       ?= /dev/ttyUSB0
FLASH_BAUD ?= 460800

IDF_ENV := command -v idf.py >/dev/null 2>&1 || . "$(IDF_EXPORT)" >/dev/null
IDF     := $(IDF_ENV); idf.py

.PHONY: all build flash monitor flash-monitor menuconfig size erase \
        clean fullclean test preview test-clean ports help

all: build

build:
	$(IDF) build

flash:
	$(IDF) -p $(PORT) -b $(FLASH_BAUD) flash

monitor:
	$(IDF) -p $(PORT) monitor

flash-monitor:
	$(IDF) -p $(PORT) -b $(FLASH_BAUD) flash monitor

menuconfig:
	$(IDF) menuconfig

size:
	$(IDF) size

# Erases the whole flash of the connected board.
erase:
	$(IDF) -p $(PORT) erase-flash

# Removes build output only (keeps sdkconfig).
clean:
	$(IDF) clean

# Removes the whole build/ directory.
fullclean:
	$(IDF) fullclean

test:
	$(MAKE) -C test test

preview:
	$(MAKE) -C test preview

test-clean:
	$(MAKE) -C test clean

# Lists serial ports, to find the right PORT=
ports:
	@ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | grep . || echo "No serial port found - is the board plugged in (data cable)?"
	@id -nG | grep -qw dialout || echo "You are not in the 'dialout' group: sudo usermod -aG dialout $$USER (then log out and in)"

help:
	@sed -n '1,14p' Makefile
