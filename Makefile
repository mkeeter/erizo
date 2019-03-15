SRC = $(wildcard *.c)
OBJ = $(addprefix build/,$(SRC:.c=.o))
DEP = $(OBJ:.o=.d)

BUILD_DIR = build
BUILD_DIR_FLAG = $(BUILD_DIR)/.f

CFLAGS = -std=c99 -Wall -Werror -g -O3 -pedantic
LDFLAGS = -lglfw -lglew -framework OpenGL $(CFLAGS)

hedgehog: $(OBJ)
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $^

build/%.o: %.c $(BUILD_DIR_FLAG)
	$(CC) $(CFLAGS) -c -o $@ $<

include $(DEP)
build/%.d: %.c $(BUILD_DIR_FLAG)
	$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) > $@

# Create a directory using a marker file
$(BUILD_DIR_FLAG):
	mkdir -p $(dir $@)
	touch $@

.PHONY: clean
clean:
	rm -rf $(OBJ) $(DEP) $(BUILD_DIR_FLAG)
	rmdir $(BUILD_DIR)
