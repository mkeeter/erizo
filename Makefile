SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

BUILD_DIR := build
BUILD_DIR_FLAG := $(BUILD_DIR)/.f

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

hedgehog: $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $^

build/%.o: %.c $(BUILD_DIR_FLAG)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ -std=c99 $<
build/%.o: %.mm $(BUILD_DIR_FLAG)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ $<

-include $(DEP)
build/%.d: %.c $(BUILD_DIR_FLAG)
	$(CC) $< $(PLATFORM) -MM -MT $(@:.d=.o) > $@
build/%.d: %.mm $(BUILD_DIR_FLAG)
	$(CC) $< $(PLATFORM) -MM -MT $(@:.d=.o) > $@

build/version.c:
	sh version.sh

# Create a directory using a marker file
$(BUILD_DIR_FLAG):
	mkdir -p $(dir $@)
	touch $@

.PHONY: clean
clean:
	rm -rf $(OBJ) $(DEP) $(BUILD_DIR_FLAG) build/version.c
	rmdir $(BUILD_DIR)
