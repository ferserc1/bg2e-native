
BUILD_DIR := ./bin
SRC_DIR := src
DEPS_DIR := ./third_party
PWD=$(shell pwd)

BG2E_CPP_FILES := $(shell find $(SRC_DIR) -name '*.cpp')
DEPS_CPP_FILES := $(shell find $(DEPS_DIR) -name '*.cpp' -or -name '*.c')

CPP_FILES := $(BG2E_CPP_FILES) $(DEPS_CPP_FILES)
OBJ_FILES := $(CPP_FILES:%=$(BUILD_DIR)/%.o)

BG2E_INCLUDE_DIR := ./include
DEPS_INCLUDE_DIR := $(shell find $(DEPS_DIR) -type d)
INCLUDE_DIR := $(BG2E_INCLUDE_DIR) $(DEPS_INCLUDE_DIR)
INC_FLAGS := $(addprefix -I,$(INCLUDE_DIR))

CPPFLAGS := -std=c++20

bg2e: $(OBJ_FILES)
	$(CXX) $(OBJ_FILES) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

.PHONY: shaders clean

shaders:
	"$(PWD)/shaders/build.sh" shaders/src $(BUILD_DIR)/shaders
	"$(PWD)/shaders/build.sh" shaders/src/test $(BUILD_DIR)/shaders/test
	echo $(CPP_FILES)
	echo $(OBJ_FILES)
	echo $(INC_FLAGS)

clean:
	rm -rf bin
