# SubCollider Makefile
# Simple CMake-based build system

# Build directory
BUILD_DIR := build

# CMake configuration flags
CMAKE_FLAGS := -DSUBCOLLIDER_BUILD_TESTS=ON \
               -DSUBCOLLIDER_BUILD_EXAMPLES=ON \
               -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=OFF

# Default target
.PHONY: all
all: test

# Help target
.PHONY: help
help:
	@echo "SubCollider Build System"
	@echo "========================"
	@echo ""
	@echo "Available targets:"
	@echo "  make test       - Build and run all tests"
	@echo "  make benchmark  - Build and run UGen benchmarks"
	@echo "  make jack       - Build JACK audio example"
	@echo "  make jack-run   - Build and run JACK example with auto-connection"
	@echo "  make basic      - Build basic example"
	@echo "  make clean      - Remove build directory"
	@echo "  make help       - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - CMake 3.14+"
	@echo "  - C++17 compatible compiler"
	@echo "  - JACK: libjack-dev (or libjack-jackd2-dev) for JACK example"

# Configure CMake (internal target)
$(BUILD_DIR)/Makefile:
	@echo "Configuring CMake..."
	@cmake -B $(BUILD_DIR) $(CMAKE_FLAGS)

# Build and run tests
.PHONY: test
test: $(BUILD_DIR)/Makefile
	@echo "Building tests..."
	@cmake --build $(BUILD_DIR) --target subcollider_tests
	@echo ""
	@echo "Running tests..."
	@echo ""
	@./$(BUILD_DIR)/subcollider_tests

# Build and run benchmarks
.PHONY: benchmark
benchmark: $(BUILD_DIR)/Makefile
	@echo "Building benchmarks..."
	@cmake --build $(BUILD_DIR) --target subcollider_benchmark
	@echo ""
	@./$(BUILD_DIR)/subcollider_benchmark

# Build JACK example
.PHONY: jack
jack: $(BUILD_DIR)/Makefile
	@echo "Building JACK example..."
	@cmake --build $(BUILD_DIR) --target example_jack
	@echo ""
	@echo "✓ Built: $(BUILD_DIR)/example_jack"
	@echo ""
	@echo "To run:"
	@echo "  ./$(BUILD_DIR)/example_jack"
	@echo ""
	@echo "Or with auto-connection:"
	@echo "  make jack-run"

# Build and run JACK example with auto-connection
.PHONY: jack-run
jack-run: jack
	@echo "Starting JACK example with auto-connection..."
	@echo "(Make sure JACK server is running)"
	@echo ""
	@# Start connection helper in background, then run the program in foreground
	@(sleep 0.8 && \
	  echo "[Connecting to system playback...]" && \
	  jack_connect subcollider:output_L system:playback_1 2>/dev/null && \
	  jack_connect subcollider:output_R system:playback_2 2>/dev/null && \
	  echo "[Connected to system:playback_1 and system:playback_2]" \
	) & \
	./$(BUILD_DIR)/example_jack

# Build basic example
.PHONY: basic
basic: $(BUILD_DIR)/Makefile
	@echo "Building basic example..."
	@cmake --build $(BUILD_DIR) --target example_basic
	@echo ""
	@echo "✓ Built: $(BUILD_DIR)/example_basic"
	@echo ""
	@echo "To run:"
	@echo "  ./$(BUILD_DIR)/example_basic"

# Run basic example
.PHONY: basic-run
basic-run: basic
	@echo "Running basic example..."
	@echo ""
	@./$(BUILD_DIR)/example_basic

# Clean build directory
.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"
