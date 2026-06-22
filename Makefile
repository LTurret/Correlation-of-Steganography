BUILD_DIR = build
TARGET = ImageSeriationSteg

.PHONY: all clean run help

all: $(BUILD_DIR)/CMakeCache.txt
	@echo "Build..."
	cmake --build $(BUILD_DIR) --config Release

$(BUILD_DIR)/CMakeCache.txt:
	@echo "Initialize CMake configuration..."
	mkdir -p $(BUILD_DIR)
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

clean:
	@echo "Clean..."
	rm -rf $(BUILD_DIR)

run: all
	@echo "Launch..."
	./$(BUILD_DIR)/$(TARGET)

help:
	@echo "Parameters:"
	@echo "  make"
	@echo "  make clean"
	@echo "  make run"
