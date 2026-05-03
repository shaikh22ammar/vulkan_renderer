### Configuration

# Debug build
isDebug ?= true

# Should Vulkan use portability extensions? (Required for Apple Silicon) 
isMacOS ?= true

### Compiler and linker commands

CC = cc

CFLAGS = 
CFLAGS += -Wall -Wextra -std=c23
CFLAGS += -Wdouble-promotion -Wfloat-conversion
CFLAGS += -MMD -MP
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

SC = slangc
SCFLAGS = -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name 


### Build directories and files

BUILD_DIR = build
TARGET = $(BUILD_DIR)/renderer

SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
$(info srcs detected are $(SRCS))

OBJS_DIR = $(BUILD_DIR)/objs
OBJS = $(patsubst $(SRC_DIR)/%, $(OBJS_DIR)/%, $(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

SLANG_DIR = $(SRC_DIR)/shaders
SLANG_SHADERS = $(wildcard $(SPV_DIR)/*.slang)

SPV_DIR = $(BUILD_DIR)/shaders
SPV_SHADERS = $(patsubst $(SLANG_DIR)/%, $(SPV_DIR)/%, $(SLANG_SHADERS:.slang=.spv))


### Targets

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) 

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c $(SPV_SHADERS) | $(OBJS_DIR)
	$(CC) $(CFLAGS) -o $@ -c $<

$(SPV_DIR)/triangle.spv: $(SLANG_DIR)/triangle.slang | $(SPV_DIR)
	$(SC) $< $(SCFLAGS) -entry vertMain -entry fragMain -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJS_DIR): | $(BUILD_DIR)
	mkdir -p $(OBJS_DIR)

$(SPV_DIR):
	mkdir -p $(SPV_DIR)

-include $(DEPS)

clean:
	rm -v $(TARGET) $(OBJS) $(DEPS) $(SPV_SHADERS)
