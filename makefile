CC = cc

CFLAGS = -Wall -Wextra -std=c23
CFLAGS += -Wdouble-promotion -Wfloat-conversion
CFLAGS += $(shell pkg-config --cflags vulkan)
CFLAGS += $(shell pkg-config --cflags glfw3)

LDFLAGS = 
LDFLAGS += $(shell pkg-config --libs vulkan)
LDFLAGS += $(shell pkg-config --libs glfw3)
LDFLAGS += -lm
LDFLAGS += -Wl,-rpath,$(shell pkg-config --variable=libdir glfw3)
LDFLAGS += -Wl,-rpath,$(shell pkg-config --variable=libdir vulkan)

run: 
	./bin/renderer

bin/renderer: src/*.c | bin
	$(CC) $(CFLAGS) src/*.c -o bin/renderer $(LDFLAGS)

bin:
	mkdir -p bin/

clean:
	rm -rfv *.o bin/*
