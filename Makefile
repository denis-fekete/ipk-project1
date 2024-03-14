SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objects
TARGET = program
LIB_DIR = $(SRC_DIR)/libs

CC = g++
CVERSTION = -std=c++20
LDFLAGS := -lm -lpcap -lnet

CFLAGS = $(CVERSTION) -pthread -pedantic-errors -Wall -Wextra -Werror -g -DDEBUG -I$(LIB_DIR)

SRCS := $(wildcard $(SRC_DIR)/*.c)
LIB_SRCS := $(wildcard $(LIB_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
LIB_OBJS := $(patsubst $(LIB_DIR)/%.c, $(OBJ_DIR)/libs/%.o, $(LIB_SRCS))

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/libs/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -rf $(OBJ_DIR)/*.o $(OBJ_DIR)/libs/*.o $(BUILD_DIR)/$(TARGET)