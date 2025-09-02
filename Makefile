# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -g

# Directories
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)/obj

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Main target
TARGET = calculator

# Create directories
$(shell mkdir -p $(BIN_DIR) $(OBJ_DIR))

# Main target build
$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf $(BIN_DIR)