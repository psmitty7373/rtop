# Define directories
SRC_DIR := src
INCLUDE_DIR := include
BUILD_DIR := build
DEP_DIR := $(BUILD_DIR)/deps

# Define the compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -I$(INCLUDE_DIR) -I /usr/include/freetype2/ -g
LDFLAGS := -lfreetype -lz -lpng

# Find all source files and corresponding object files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS := $(patsubst $(SRC_DIR)/%.c, $(DEP_DIR)/%.d, $(SRCS))

# Define the target binary
TARGET := $(BUILD_DIR)/rtop

# Default rule
all: $(TARGET)

# Rule to build the target
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)

# Rule to compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(DEP_DIR)/%.d
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Generate dependency files
$(DEP_DIR)/%.d: $(SRC_DIR)/%.c
	@mkdir -p $(DEP_DIR)
	@$(CC) -MM $(CFLAGS) $< -MF $@ -MT $(BUILD_DIR)/$*.o

# Include dependency files if they exist
-include $(DEPS)

# Clean rule
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
