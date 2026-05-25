### Configuration

# Debug build
isDebug ?= true

# Should Vulkan use portability extensions? (Required for Apple Silicon) 
isMacOS ?= true

# Packages paths
comma := ,
PKGS := vulkan glfw3
PKGS_INFO := $(shell pkg-config --cflags --libs $(PKGS))
PKGS_INC := $(filter -I%, $(PKGS_INFO))
PKGS_LIB := $(filter -L%, $(PKGS_INFO)) $(filter -l%, $(PKGS_INFO))
PKGS_RPATH := $(patsubst -L%, -Wl$(comma)-rpath$(comma)%, $(filter -L%, $(PKGS_LIB)))
PKGS_INC += -Ithird_party/cglm/include
PKGS_INC += -Ithird_party/glm/

### Compiler and linker commands
CC := cc
CXX := c++

# Flags shared by both C and C++
COMMON_FLAGS =
COMMON_FLAGS += -Wall -Wextra
COMMON_FLAGS += -Wdouble-promotion -Wfloat-conversion
COMMON_FLAGS += -MMD -MP
COMMON_FLAGS += -Isrc
COMMON_FLAGS += $(PKGS_INC)
ifeq ($(isMacOS),true)
COMMON_FLAGS += -DMAC_OS -DPORTABILITY
endif
ifeq ($(isDebug),true)
COMMON_FLAGS += -O0 -g -DVALIDATION
LDFLAGS += -g
else
COMMON_FLAGS += -O3 -DNDEBUG
endif

CFLAGS   = $(COMMON_FLAGS) -std=c23
CXXFLAGS = $(COMMON_FLAGS) -std=c++23

LDFLAGS =
LDFLAGS += -lm
LDFLAGS += $(PKGS_LIB) $(PKGS_RPATH)

SC = slangc
SCFLAGS = -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name

### Build directories and files
BUILD_DIR = build
TARGET = $(BUILD_DIR)/renderer
SRC_DIR = src
C_SRCS   = $(shell find $(SRC_DIR) -name "*.c"   -not -name "_*")
CXX_SRCS = $(shell find $(SRC_DIR) -name "*.cpp" -not -name "_*")
$(info C sources:   $(C_SRCS))
$(info C++ sources: $(CXX_SRCS))

OBJS_DIR = $(BUILD_DIR)/objs
C_OBJS   = $(patsubst $(SRC_DIR)/%, $(OBJS_DIR)/%, $(C_SRCS:.c=.o))
CXX_OBJS = $(patsubst $(SRC_DIR)/%, $(OBJS_DIR)/%, $(CXX_SRCS:.cpp=.o))
OBJS     = $(C_OBJS) $(CXX_OBJS)
DEPS     = $(OBJS:.o=.d)

SLANG_DIR     = $(SRC_DIR)/shaders
SLANG_SHADERS = $(wildcard $(SLANG_DIR)/*.slang)
$(info Shaders: $(SLANG_SHADERS))
SPV_DIR     = $(BUILD_DIR)/shaders
SPV_SHADERS = $(patsubst $(SLANG_DIR)/%, $(SPV_DIR)/%, $(SLANG_SHADERS:.slang=.spv))

### Targets
all: $(TARGET)

$(TARGET): $(OBJS) $(SPV_SHADERS) | $(BUILD_DIR)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJS_DIR)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJS_DIR)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(SPV_DIR)/triangle.spv: $(SLANG_DIR)/triangle.slang | $(SPV_DIR)
	$(SC) $< $(SCFLAGS) -entry vertMain -entry fragMain -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(SPV_DIR):
	mkdir -p $(SPV_DIR)

-include $(DEPS)

clean:
	rm -v $(TARGET) $(OBJS) $(DEPS) $(SPV_SHADERS)
