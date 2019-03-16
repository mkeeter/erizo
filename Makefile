SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

BUILD_DIR := build

CFLAGS := -Wall -Werror -g -O3 -pedantic
LDFLAGS := -lglfw -lglew -framework OpenGL $(CFLAGS)

# Platform detection
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	OBJ := $(OBJ) platform_darwin.o
	LDFLAGS := $(LDFLAGS) -framework Foundation -framework Cocoa
	PLATFORM := -DPLATFORM_DARWIN
endif

OBJ := $(OBJ) version.o
OBJ := $(addprefix build/,$(OBJ))
DEP := $(OBJ:.o=.d)

all: hedgehog

hedgehog: $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $(OBJ)

build/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ -std=c99 $<
build/%.o: %.mm | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ $<

build/version.o: build/version.c  | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ -std=c99 $<

-include $(DEP)
build/%.d: %.c | $(BUILD_DIR)
	$(CC) $< $(PLATFORM) -MM -MT $(@:.d=.o) > $@
build/%.d: %.mm | $(BUILD_DIR)
	$(CC) $< $(PLATFORM) -MM -MT $(@:.d=.o) > $@

# Create a directory using a marker file
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: clean version
clean:
	rm -rf $(OBJ) $(DEP) build/version.c
	rmdir $(BUILD_DIR)

build/version.c: version
version:
	sh version.sh
