# Define directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build

# Define the compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -I$(INCLUDE_DIR) -I /usr/include/freetype2/ -O3
LDFLAGS := -lfreetype -lz

# Find all source files and corresponding object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Define the target binary
TARGET := $(BUILD_DIR)/rtop

# Default rule
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

# Rule to compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(wildcard $(SRC_DIR)/*.h)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
