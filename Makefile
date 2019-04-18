# Source files
SRC :=       \
	app      \
	backdrop \
	camera   \
	instance \
	loader   \
	log      \
	main     \
	mat      \
	model    \
	shader   \
	theme    \
	vset     \
	window   \
	worker   \
	# end of source files

# Force the version-generation script to run before
# anything else, generating version.c and log_align.h
_GEN := $(shell sh gen.sh $(SRC))

# Generated files
GEN :=       \
	sphere   \
	version  \
	# end of generated files

BUILD_DIR := build

CFLAGS := -Wall -Werror -g -O3 -pedantic -Iinc
LDFLAGS := -lglfw -lglew -framework OpenGL $(CFLAGS)

# Build with Clang's undefined behavior sanitizer:
# make clean; env UBSAN=1 make
ifeq ($(UBSAN),1)
	CFLAGS := $(CFLAGS) -fsanitize=undefined
	LDFLAGS := $(LDFLAGS) -fsanitize=undefined -lstdc++ -lc++abi
endif

# Build with Clang's address sanitizer:
# make clean; env ASAN=1 make
ifeq ($(ASAN),1)
	CFLAGS := $(CFLAGS) -fsanitize=address
	LDFLAGS := $(LDFLAGS) -fsanitize=address
endif

# Platform detection
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	SRC := $(SRC) platform_unix platform_darwin
	LDFLAGS := $(LDFLAGS) -framework Foundation -framework Cocoa
	PLATFORM := -DPLATFORM_DARWIN
endif

OBJ := $(addprefix build/,$(SRC:=.o) $(GEN:=.o))
DEP := $(OBJ:.o=.d)

all: erizo

erizo: $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $^

build/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ -std=c99 $<
build/%.o: src/%.mm | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP)
build/%.d: src/%.c | $(BUILD_DIR)
	$(CC) $< $(PLATFORM) -Iinc -MM -MT $(@:.d=.o) > $@
build/%.d: src/%.mm | $(BUILD_DIR)
	$(CC) $< $(PLATFORM) -Iinc -MM -MT $(@:.d=.o) > $@
endif

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

src/sphere.c: sphere.stl
	xxd -i $< > $@

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(addprefix src/,$(GEN:=.c))
	rm -f erizo
