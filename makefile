### Configuration

# Debug build
isDebug ?= true

# Should Vulkan use portability extensions? (Required for Apple Silicon) 
isMacOS ?= true

###

CC = cc

CFLAGS = 
CFLAGS += -Wall -Wextra -std=c23
CFLAGS += -Wdouble-promotion -Wfloat-conversion
CFLAGS += $(shell pkg-config --cflags vulkan)
CFLAGS += $(shell pkg-config --cflags glfw3)

ifeq ($(isMacOS),true)
CFLAGS += -DMAC_OS
endif

ifeq ($(isDebug),true)
CFLAGS += -O0 -g
else
CFLAGS += -O2 -DNDEBUG
endif

LDFLAGS =
LDFLAGS += $(shell pkg-config --libs vulkan)
LDFLAGS += $(shell pkg-config --libs glfw3)
LDFLAGS += -lm
LDFLAGS += -Wl,-rpath,$(shell pkg-config --variable=libdir glfw3)
LDFLAGS += -Wl,-rpath,$(shell pkg-config --variable=libdir vulkan)

BUILD_DIR = build
TARGET = $(BUILD_DIR)/renderer

SRC_DIR = src
SRCS = $(wildcard src/*.c)
$(info srcs detected are $(SRCS))

OBJS_DIR = $(BUILD_DIR)/objs
OBJS = $(patsubst $(SRC_DIR)/%, $(OBJS_DIR)/%, $(SRCS:.c=.o))

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(OBJS_DIR): | $(BUILD_DIR)
	mkdir $(OBJS_DIR)

clean:
	rm -rfv build 
