# Source files
SRC :=                  \
	src/app             \
	src/backdrop        \
	src/camera          \
	src/draw            \
	src/icosphere       \
	src/instance        \
	src/loader          \
	src/log             \
	src/mat             \
	src/model           \
	src/shader          \
	src/shaded          \
	src/theme           \
	src/version         \
	src/vset            \
	src/window          \
	src/wireframe       \
	src/worker          \
	vendor/glew/glew    \
	# end of source files

################################################################################

# Generated files
# (listed separately so that 'make clean' deletes them)
VERSION_FILE := src/version.c
LOG_ALIGN_FILE := inc/log_align.h
GEN := $(VERSION_FILE) $(LOG_ALIGN_FILE)

################################################################################
# File generation for version and log alignment
GIT_REV := $(shell git log --pretty=format:'%h' -n 1)
GIT_DIFF := $(shell git diff --quiet --exit-code || echo +)
GIT_TAG := $(shell git describe --exact-match --tags 2> /dev/null || echo "")
GIT_BRANCH := $(shell git rev-parse --abbrev-ref HEAD)

VERSION := const char* GIT_REV="$(GIT_REV)$(GIT_DIFF)";\
const char* GIT_TAG="$(GIT_TAG)";\
const char* GIT_BRANCH="$(GIT_BRANCH)";

PREV_VERSION := $(shell cat $(VERSION_FILE) 2> /dev/null || echo "")

ifneq ($(VERSION), $(PREV_VERSION))
# Hilarious string escaping shenanigans
VERSION := $(subst =,=\",$(VERSION))
VERSION := $(subst ;,\";,$(VERSION))
$(shell echo "$(VERSION)" > $(VERSION_FILE))
endif

LONGEST=$(shell wc -l src/*.c|grep -v total|sed "s/ //g"|awk '{print length}'|sort|tail -1)
LOG_ALIGN=\#define LOG_ALIGN (1 + $(LONGEST))
PREV_LOG_ALIGN := $(shell cat $(LOG_ALIGN_FILE) 2> /dev/null || echo "")

ifneq ($(LOG_ALIGN), $(PREV_LOG_ALIGN))
$(shell echo "$(LOG_ALIGN)" > $(LOG_ALIGN_FILE))
endif

################################################################################

# Platform detection
ifndef TARGET
UNAME := $(shell uname)
	ifeq ($(UNAME), Darwin)
		TARGET := darwin
	endif
endif

CFLAGS := -Wall -Werror -g -O3 -pedantic -Iinc -Ivendor -Ivendor/glfw/include -Ivendor/glew
LDFLAGS = -Lvendor/glfw/build-$(TARGET)/src -lglfw3

# Build with Clang's undefined behavior sanitizer:
# make clean; env UBSAN=1 make
ifeq ($(UBSAN),1)
	CFLAGS  += -fsanitize=undefined
	LDFLAGS += -lstdc++ -lc++abi
endif

# Build with Clang's address sanitizer:
# make clean; env ASAN=1 make
ifeq ($(ASAN),1)
	CFLAGS  += -fsanitize=address
endif

ifeq ($(TARGET), darwin)
	SRC +=  platform/darwin platform/posix
	LDFLAGS := -framework Foundation             \
	           -framework Cocoa                  \
	           -framework IOKit                  \
	           -framework CoreVideo              \
	           -framework OpenGL                 \
	           -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk \
	           -dead_strip \
	           $(LDFLAGS)
	PLATFORM := -DPLATFORM_DARWIN
endif
ifeq ($(TARGET), win32-cross)
	CC := x86_64-w64-mingw32-gcc
	SRC += platform/win32
	CFLAGS += -mwindows -DGLEW_STATIC
	PLATFORM := -DPLATFORM_WIN32
	LDFLAGS += -lopengl32
	ERIZO_APP  := erizo.exe
	ERIZO_TEST := erizo-test.exe
else
	ERIZO_APP  := erizo
	ERIZO_TEST := erizo-test
endif

################################################################################

BUILD_DIR := build-$(TARGET)

all: $(GEN) $(ERIZO_APP) $(ERIZO_TEST)

OBJ := $(addprefix $(BUILD_DIR)/,$(SRC:=.o))
DEP := $(OBJ:.o=.d)

ifeq ($(TARGET), win32-cross)
$(BUILD_DIR)/erizo.coff: deploy/win32/erizo.rc
	x86_64-w64-mingw32-windres $? $@
OBJ += $(BUILD_DIR)/erizo.coff
endif

$(ERIZO_APP): $(BUILD_DIR)/src/main.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(ERIZO_TEST): $(BUILD_DIR)/src/test.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ -std=c99 $<
$(BUILD_DIR)/%.o: %.mm | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PLATFORM) -c -o $@ -std=c++11 $<

ifneq ($(MAKECMDGOALS),clean)
-include $(DEP)
$(BUILD_DIR)/%.d: %.c | $(BUILD_DIR) $(GEN)
	$(CC) $< $(PLATFORM) $(CFLAGS) -MM -MT $(@:.d=.o) > $@
$(BUILD_DIR)/%.d: %.mm | $(BUILD_DIR) $(GEN)
	$(CC) $< $(PLATFORM) -Iinc -MM -MT $(@:.d=.o) > $@
endif

$(BUILD_DIR):
	mkdir -p $(sort $(dir $(OBJ)))

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(GEN)
	rm -f $(ERIZO_APP)
	rm -f $(ERIZO_TEST)

################################################################################
# Building vendored GLFW
glfw:
	cd vendor/glfw && mkdir -p build-$(TARGET)
ifeq ($(TARGET), win32-cross)
	cd vendor/glfw/build-$(TARGET) && cmake \
	    -DCMAKE_TOOLCHAIN_FILE=../CMake/x86_64-w64-mingw32.cmake \
	    -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_DOCS=OFF \
	    -DGLFW_BUILD_TESTS=OFF .. && \
	    make
else
	cd vendor/glfw/build-$(TARGET) && cmake \
	    -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_DOCS=OFF \
	    -DGLFW_BUILD_TESTS=OFF .. && \
	    make
endif
