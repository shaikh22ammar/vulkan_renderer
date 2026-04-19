CC = cc

CFLAGS = -Wall -Wextra -std=c23
CFLAGS += -Wdouble-promotion -Wfloat-conversion
CFLAGS += $(shell pkg-config --cflags vulkan)
CFLAGS += $(shell pkg-config --cflags glfw3)
CFLAGS += -DMAC_OS

LDFLAGS = 
LDFLAGS += $(shell pkg-config --libs vulkan)
LDFLAGS += $(shell pkg-config --libs glfw3)
LDFLAGS += -lm
LDFLAGS += -Wl,-rpath,$(shell pkg-config --variable=libdir glfw3)
LDFLAGS += -Wl,-rpath,$(shell pkg-config --variable=libdir vulkan)

SRCS = src/main.c

TARGET = bin/renderer

debug: CFLAGS += -g -O0
debug: $(TARGET)

build: CFLAGS += -O2
build: $(TARGET)

bin/renderer: $(SRCS) | bin
	$(CC) $(CFLAGS) src/*.c -o bin/renderer $(LDFLAGS)

compile_commands.json: $(SRCS)
	bear -- make $(TARGET)

run: 
	./bin/renderer

bin:
	mkdir -p bin/

clean:
	rm -rfv *.o bin/*

