BUILD_DIR := ./bin
SRC_DIR := src
EXAMPLES_DIR := examples
DEPS_DIR := ./third_party
PWD=$(shell pwd)

BG2E_EXAMPLE_APP_FILES := $(shell find $(EXAMPLES_DIR)/app -name '*.cpp')
BG2E_CPP_FILES := $(shell find $(SRC_DIR) -name '*.cpp')
DEPS := $(DEPS_DIR)/imgui $(DEPS_DIR)/simdjson
DEPS_CPP_FILES := $(shell find $(DEPS) -name '*.cpp' -or -name '*.c')

CPP_FILES := $(BG2E_CPP_FILES) $(DEPS_CPP_FILES)
OBJ_FILES := $(CPP_FILES:%=$(BUILD_DIR)/%.o)

BG2E_INCLUDE_DIR := ./include
DEPS_INCLUDE_DIR := $(shell find $(DEPS_DIR) -type d)
INCLUDE_DIR := $(BG2E_INCLUDE_DIR) $(DEPS_INCLUDE_DIR) $(VULKAN_SDK)/include
INC_FLAGS := $(addprefix -I,$(INCLUDE_DIR))

CPPFLAGS := -std=c++20
LDFLAGS := -L$(VULKAN_SDK)/lib -lvulkan -lSDL2 -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

example-app: bg2e copy-assets
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -o $(BUILD_DIR)/bg2e-example-app $(BG2E_EXAMPLE_APP_FILES) $(LDFLAGS) -L$(BUILD_DIR) -lbg2e-native

bg2e: $(OBJ_FILES) shaders
	$(CXX) -shared $(OBJ_FILES) -o $(BUILD_DIR)/libbg2e-native.so $(LDFLAGS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -fPIC -c $< -o $@

.PHONY: shaders clean

shaders:
	"$(PWD)/shaders/build.sh" shaders/src $(BUILD_DIR)/shaders
	"$(PWD)/shaders/build.sh" shaders/src/test $(BUILD_DIR)/shaders/test
	echo $(CPP_FILES)
	echo $(OBJ_FILES)
	echo $(INC_FLAGS)

copy-assets:
	cp -r ./assets $(BUILD_DIR)/assets

clean:
	rm -rf bin
