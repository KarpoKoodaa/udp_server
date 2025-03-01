# Variables
PROJECT := udp-server

SRC_DIR := ./src
INC_DIR := ./include
OBJ_DIR := ${BUILD_DIR}/obj
BUILD_DIR := ./build

CC := gcc
CC_FLAGS := -I${INC_DIR} -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2  -Wunused-parameter -g

EXEC := $(BUILD_DIR)/udp-server 
EXEC2 := $(BUILD_DIR)/gbn-client
EXEC3 := $(BUILD_DIR)/sr_client
SRC := $(wildcard $(SRC_DIR)/*.c)
EXEC_SRC := ./src/udp_server.c ./src/crc.c ./src/sleep.c ./src/rdn_num.c ./src/rdt.c ./src/gbn.c ./src/sr.c
EXEC2_SRC := ./src/gbn_client.c ./src/crc.c
EXEC3_SRC := ./src/sr_client.c ./src/crc.c
OBJ := $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Rules
.PHONY: all clean

all: $(EXEC) $(EXEC2) $(EXEC3)

$(EXEC): $(BUILD_DIR)
	$(CC) $(CC_FLAGS) -o $@ $(EXEC_SRC) 
# $(CC) $(CC_FLAGS) -o $@ ${SRC}

$(EXEC2): $(BUILD_DIR)
	$(CC) $(CC_FLAGS) -o $@ $(EXEC2_SRC) 

$(EXEC3): $(BUILD_DIR)
	$(CC) $(CC_FLAGS) -o $@ $(EXEC3_SRC)

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

#gbn: $(BUILD_DIR)  
#	$(CC) $(CC_FLAGS) -o $@  $(SRC_DIR)/gbn_server.c $(SRC_DIR)/crc.c


clean:
	rm -rv $(BUILD_DIR) $(OBJ_DIR)