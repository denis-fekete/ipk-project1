# CXX      := -c++
CXX      := gcc
# CPPVERSION := -std=c++20
CPPVERSION := -std=c17
CXXFLAGS :=  $(CPPVERSION) -pedantic-errors -Wall -Wextra -Werror -g -DDEBUG
LDFLAGS  := -lm -lpcap -lnet
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
TARGET   := program
SRC      :=\
   $(wildcard src/*.c) \
#    $(wildcard src/*.cpp) \

OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)


all: build $(BUILD)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -MMD -o $@

$(BUILD)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(BUILD)/$(TARGET) $^ $(LDFLAGS)


.PHONY: all build clean debug release info

build:
	@mkdir -p $(BUILD)
	@mkdir -p $(OBJ_DIR)

run:
	@./$(BUILD)/$(TARGET)

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(BUILD)/*