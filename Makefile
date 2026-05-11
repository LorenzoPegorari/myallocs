# MIT License
#
# Copyright (c) 2026 Lorenzo Pegorari (@LorenzoPegorari)
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# --------------------------------- VARIABLES ---------------------------------

# User-defined variables
BUILD_DIR := build

SRC := myallocs.c
OBJ := $(SRC:.c=.o)
INC := $(SRC:.c=.h)
BIN := libmyallocs.so

OBJ_DEBUG := $(SRC:.c=_debug.o)
BIN_DEBUG := libmyallocs_debug.so

SRC_TEST := test.c
OBJ_TEST := $(SRC_TEST:.c=.o)
BIN_TEST := test_myallocs

CC      := gcc
CFLAGS  := -fPIC -std=c99 -O2 -Wall -Wextra -pedantic
LDFLAGS := -shared

CFLAGS_DEBUG := -D MYALLOCS_DEBUG -fPIC -std=c99 -O2 -Wall -Wextra -pedantic

CFLAGS_TEST  := -D MYALLOCS_DEBUG -std=c99 -O2 -Wall -Wextra -pedantic
LDFLAGS_TEST := -L./$(BUILD_DIR) -lc -l$(BIN_DEBUG:lib%.so=%) -Wl,-rpath,./$(BUILD_DIR)

# NOTE:
# It is possible to add either to the CFLAGS or CFLAGS_DEBUG variable the
# following flags:
#   "-D MYALLOCS_BEST_FIT"     = use "BEST FIT" policy
#   "-D MYALLOCS_FULL_DEALLOC" = deallocate as many free blocks as possible


# ----------------------------------- GOALS -----------------------------------

.PHONY: release debug test clean


# Main goal
release: $(BUILD_DIR)/$(BIN)

# Linking
$(BUILD_DIR)/$(BIN): $(BUILD_DIR)/$(OBJ)
	$(CC) -o $@ $< $(LDFLAGS)

# Compiling
$(BUILD_DIR)/$(OBJ): $(SRC) $(INC)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<


# Debug goal
debug: $(BUILD_DIR)/$(BIN_DEBUG)

# Debug linking
$(BUILD_DIR)/$(BIN_DEBUG): $(BUILD_DIR)/$(OBJ_DEBUG)
	$(CC) -o $@ $< $(LDFLAGS)

# Debug compiling
$(BUILD_DIR)/$(OBJ_DEBUG): $(SRC) $(INC)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_DEBUG) -o $@ -c $<


# Test goal
test: $(BUILD_DIR)/$(BIN_TEST)

# Test linking
$(BUILD_DIR)/$(BIN_TEST): $(BUILD_DIR)/$(OBJ_TEST) $(BUILD_DIR)/$(BIN_DEBUG)
	$(CC) -o $@ $< $(LDFLAGS_TEST)

# Test compiling
$(BUILD_DIR)/$(OBJ_TEST): $(SRC_TEST)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS_TEST) -o $@ -c $<


# Clean
clean:
	$(RM) -r $(BUILD_DIR)
