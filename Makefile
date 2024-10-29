# Variables
PROJECT := udp-server

SRC_DIR := ./src
INC_DIR := ./include
OBJ_DIR := ${BUILD_DIR}/obj
BUILD_DIR := ./build

CC := gcc
CC_FLAGS := -I${INC_DIR} -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2 -Wconversion -Wunused-parameter -g

EXE := $(BUILD_DIR)/udp-server
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.0)

# Rules
.PHONY: all clean

all: $(EXE)

$(EXE): $(BUILD_DIR)
		$(CC) $(CC_FLAGS) -o $@ ${SRC}

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	$@(RM) -rv $(BUILD_DIR) $(OBJ_DIR)