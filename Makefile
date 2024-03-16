# VARIABLES
## Directories
SRC_DIR=src
BUILD_DIR=build
INC_DIR=elm99/include

## Compilation flags
CFLAGS= -std=c99 -pedantic -Wall -Wextra -I./$(INC_DIR)/ -DUSE_ASSERT
LDFLAGS= -lm -lpthread -O3 -ffast-math

# FILE RULES
## Build elm
elm99/build/libelm.a:
	cd elm99 && make build

## Build main binary
$(BUILD_DIR)/main: $(SRC_DIR)/main.c elm99/build/libelm.a
	$(CC) $(SRC_DIR)/*.c elm99/build/libelm.a $(CFLAGS) $(LDFLAGS) -o $@

# PHONY RULES
## Define phony rules
.PHONY: clean test

clean:
	rm -rf $(BUILD_DIR)/*

test: $(BUILD_DIR)/main
	$(BUILD_DIR)/main
