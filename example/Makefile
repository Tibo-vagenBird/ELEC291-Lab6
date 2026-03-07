SHELL=cmd

# Toolchain path
TOOLCHAIN = D:/avr8-gnu-toolchain/bin

# Tools
CC      = $(TOOLCHAIN)/avr-gcc
OBJCOPY = $(TOOLCHAIN)/avr-objcopy
SIZE    = $(TOOLCHAIN)/avr-size

# Target name (change this to your project name)
TARGET = main

# Microcontroller
CPU = -mmcu=atmega328p

# Directories
SRC_DIR   = src
INC_DIR   = inc
BUILD_DIR = build

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Compiler flags
COPT = -g -Os $(CPU) -I$(INC_DIR)

# Default target
all: $(BUILD_DIR)/$(TARGET).hex

# Link object files into .elf, then convert to .hex
$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(CPU) -Wl,-Map,$(BUILD_DIR)/$(TARGET).map $^ -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $@
	@echo done!

# Compile each .c from /src into .o in /build
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(COPT) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

# Remove all build artifacts
clean:
	@del /Q $(BUILD_DIR)\*.hex $(BUILD_DIR)\*.elf $(BUILD_DIR)\*.o $(BUILD_DIR)\*.map 2> nul
	@echo cleaned.

# Serial port for flashing (usage: make flashload COM15)
PORT ?= COM3
_PORT := $(filter COM%, $(MAKECMDGOALS))
ifneq ($(_PORT),)
  PORT := $(_PORT)
endif

# Absorb the COM* argument so make doesn't treat it as a missing target
COM%:
	@rem

# Flash the hex file to the microcontroller
flashload:
	spi_atmega -CRYSTAL -p -v -PORT=$(PORT) $(BUILD_DIR)/$(TARGET).hex

# Phony targets
.PHONY: all clean flashload
