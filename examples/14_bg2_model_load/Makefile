APP_NAME=example-14-bg2-model-load
BUILD_DIR := ./bin
SRC_DIR := src
BG2E_INCLUDE_DIR := ../../include
BG2E_LIB_PATH := ../../bin
PWD=$(shell pwd)

APP_CPP_FILES := $(shell find $(SRC_DIR) -name '*.cpp')
DEPS := $(BG2E_LIB_PATH)
GTK_INCLUDES := $(shell pkg-config --cflags --libs gtk+-3.0)

CPP_FILES := $(APP_CPP_FILES)
OBJ_FILES := $(CPP_FILES:%=$(BUILD_DIR)/%.o)
VULKAN_INCLUDE := $(VULKAN_SDK)/include

INCLUDE_DIR := $(BG2E_INCLUDE_DIR) $(VULKAN_INCLUDE)
INC_FLAGS := $(addprefix -I,$(INCLUDE_DIR))

LDFLAGS := -L$(BG2E_INCLUDE_DIR) -L$(VULKAN_SDK)/lib -lbg2e-native -lvulkan -lSDL2 -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi -lgtk-3

# Build configuration: default is release, use DEBUG=1 for debug build
ifdef DEBUG
    CPPFLAGS := -std=c++20 -Wall -Wextra -g -O0 -DDEBUG
	CFLAGS := -Wall -Wextra -g -O0 -DDEBUG
else
#    CPPFLAGS := -std=c++20 -Wall -Wextra -O3 -DNDEBUG
endif

buile-example: $(OBJ_FILES) shaders copy-assets copy-bg2e-resources
	$(CXX) $(OBJ_FILES) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -o $(BUILD_DIR)/$(APP_NAME) $(LDFLAGS) -L$(BG2E_LIB_PATH) -L$(BUILD_DIR) -Wl,-rpath,'$$ORIGIN'

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) $(GTK_INCLUDES) -fPIC -c $< -o $@

.PHONY: shaders clean

shaders:
	"$(PWD)/../../shaders/build.sh" shaders/src $(BUILD_DIR)/shaders

copy-assets:
	cp -r ../../assets $(BUILD_DIR)/assets

copy-bg2e-resources:
	cp -r ../../bin/shaders $(BUILD_DIR)
	cp ../../bin/libbg2e-native.so $(BUILD_DIR)

clean:
	rm -rf bin
