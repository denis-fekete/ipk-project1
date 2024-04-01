SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/objects
TARGET = ipk24chat-client
LIB_DIR = $(SRC_DIR)/libs

CC = gcc
CVERSTION = -std=c17
LDFLAGS := -lm

# Default flags for debug build
DEBUG_CFLAGS = -pthread -pedantic-errors -Wall -Wextra -Werror -g -DDEBUG
RELEASE_CFLAGS = -pthread -pedantic-errors -Wall -Wextra -Werror

ifeq ($(DEBUG),true)
	CFLAGS = $(CVERSTION) $(DEBUG_CFLAGS) -I$(LIB_DIR)
else
	CFLAGS = $(CVERSTION) $(RELEASE_CFLAGS) -I$(LIB_DIR)
endif

SRCS := $(wildcard $(SRC_DIR)/*.c)
LIB_SRCS := $(wildcard $(LIB_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
LIB_OBJS := $(patsubst $(LIB_DIR)/%.c, $(OBJ_DIR)/libs/%.o, $(LIB_SRCS))

all: $(TARGET)

$(TARGET): $(OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/libs/%.o: $(LIB_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean doc

doc:
	doxygen Doxyfile
	pip install esp-doxybook
	esp-doxybook -i docs/xml -o ./README.md
	clear

doc_clean:
	rm -r ./docs/docbook ./docs/html ./docs/latex ./docs/man ./docs/xml

clean:
	rm -rf $(OBJ_DIR)/*.o $(OBJ_DIR)/libs/*.o $(TARGET)
