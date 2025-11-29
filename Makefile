# SubCollider Makefile
# Simple CMake-based build system

# Build directory
BUILD_DIR := build

# CMake configuration flags
CMAKE_FLAGS := -DSUBCOLLIDER_BUILD_TESTS=ON \
               -DSUBCOLLIDER_BUILD_EXAMPLES=ON \
               -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=OFF \
               -DSUBCOLLIDER_BUILD_MOOG_EXAMPLE=OFF \
               -DSUBCOLLIDER_BUILD_SUPERSAW_EXAMPLE=OFF

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
	@echo "  make test              - Build and run all tests"
	@echo "  make benchmark         - Build and run UGen benchmarks"
	@echo "  make jack              - Build JACK audio example"
	@echo "  make jack-run          - Build and run JACK example with auto-connection"
	@echo "  make example-playback  - Build and run JACK playback example with auto-connection"
	@echo "  make example-moog      - Build and run Moog filter example with auto-connection"
	@echo "  make example-supersaw  - Build and run SuperSaw example with auto-connection"
	@echo "  make basic             - Build basic example"
	@echo "  make clean             - Remove build directory"
	@echo "  make help              - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - CMake 3.14+"
	@echo "  - C++17 compatible compiler"
	@echo "  - JACK: libjack-dev (or libjack-jackd2-dev) for JACK examples"
	@echo "  - libsndfile: libsndfile1-dev for playback example"
	@echo "  - X11: libx11-dev for Moog and SuperSaw examples"

# Configure CMake (internal target)
$(BUILD_DIR)/Makefile:
	@echo "Configuring CMake..."
	@cmake -B $(BUILD_DIR) $(CMAKE_FLAGS)

# Build and run tests
.PHONY: test
test: $(BUILD_DIR)/Makefile
	@echo "Building tests..."
	@cmake --build $(BUILD_DIR) --target subcollider_tests -j
	@echo ""
	@echo "Running tests..."
	@echo ""
	@./$(BUILD_DIR)/subcollider_tests

# Build and run benchmarks
.PHONY: benchmark
benchmark: $(BUILD_DIR)/Makefile
	@echo "Building benchmarks..."
	@cmake --build $(BUILD_DIR) --target subcollider_benchmark -j
	@echo ""
	@./$(BUILD_DIR)/subcollider_benchmark

# Build JACK example
.PHONY: jack
jack:
	@echo "Building JACK example..."
	@cmake -B $(BUILD_DIR) -DSUBCOLLIDER_BUILD_TESTS=ON \
	                       -DSUBCOLLIDER_BUILD_EXAMPLES=ON \
	                       -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=ON \
	                       -DSUBCOLLIDER_BUILD_MOOG_EXAMPLE=OFF
	@cmake --build $(BUILD_DIR) --target example_jack -j
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
jack-run:
	@echo "Building JACK example..."
	@cmake -B $(BUILD_DIR) -DSUBCOLLIDER_BUILD_TESTS=ON \
	                       -DSUBCOLLIDER_BUILD_EXAMPLES=ON \
	                       -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=ON \
	                       -DSUBCOLLIDER_BUILD_MOOG_EXAMPLE=OFF
	@cmake --build $(BUILD_DIR) --target example_jack -j
	@echo ""
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
	@cmake --build $(BUILD_DIR) --target example_basic -j
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

# Build and run JACK playback example with auto-connection
.PHONY: example-playback
example-playback:
	@echo "Building JACK playback example..."
	@cmake -B $(BUILD_DIR) -DSUBCOLLIDER_BUILD_TESTS=ON \
	                       -DSUBCOLLIDER_BUILD_EXAMPLES=ON \
	                       -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=OFF \
	                       -DSUBCOLLIDER_BUILD_JACK_PLAYBACK_EXAMPLE=ON \
	                       -DSUBCOLLIDER_BUILD_MOOG_EXAMPLE=OFF \
	                       -DSUBCOLLIDER_BUILD_SUPERSAW_EXAMPLE=OFF
	@cmake --build $(BUILD_DIR) --target example_jack_playback -j
	@echo ""
	@echo "Starting JACK playback example with auto-connection..."
	@echo "(Make sure JACK server is running)"
	@echo ""
	@# Start connection helper in background, then run the program in foreground
	@(sleep 0.8 && \
	  echo "[Connecting to system playback...]" && \
	  jack_connect subcollider_playback:output_L system:playback_1 2>/dev/null && \
	  jack_connect subcollider_playback:output_R system:playback_2 2>/dev/null && \
	  echo "[Connected to system:playback_1 and system:playback_2]" \
	) & \
	./$(BUILD_DIR)/example_jack_playback

# Build and run Moog filter example with auto-connection
.PHONY: example-moog
example-moog:
	@echo "Building Moog filter example..."
	@cmake -B $(BUILD_DIR) -DSUBCOLLIDER_BUILD_TESTS=ON \
	                       -DSUBCOLLIDER_BUILD_EXAMPLES=ON \
	                       -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=OFF \
	                       -DSUBCOLLIDER_BUILD_MOOG_EXAMPLE=ON \
	                       -DSUBCOLLIDER_BUILD_SUPERSAW_EXAMPLE=OFF
	@cmake --build $(BUILD_DIR) --target example_moog -j
	@echo ""
	@echo "Starting Moog filter example with auto-connection..."
	@echo "(Make sure JACK server is running)"
	@echo ""
	@# Start connection helper in background, then run the program in foreground
	@(sleep 0.8 && \
	  echo "[Connecting to system playback...]" && \
	  jack_connect subcollider_moog:output_L system:playback_1 2>/dev/null && \
	  jack_connect subcollider_moog:output_R system:playback_2 2>/dev/null && \
	  echo "[Connected to system:playback_1 and system:playback_2]" \
	) & \
	./$(BUILD_DIR)/example_moog

# Backward compatibility alias
.PHONY: moog
moog: example-moog

# Build and run SuperSaw example with auto-connection
.PHONY: example-supersaw
example-supersaw:
	@echo "Building SuperSaw example..."
	@cmake -B $(BUILD_DIR) -DSUBCOLLIDER_BUILD_TESTS=ON \
	                       -DSUBCOLLIDER_BUILD_EXAMPLES=ON \
	                       -DSUBCOLLIDER_BUILD_JACK_EXAMPLE=OFF \
	                       -DSUBCOLLIDER_BUILD_MOOG_EXAMPLE=OFF \
	                       -DSUBCOLLIDER_BUILD_SUPERSAW_EXAMPLE=ON
	@cmake --build $(BUILD_DIR) --target example_supersaw -j
	@echo ""
	@echo "Starting SuperSaw example with auto-connection..."
	@echo "(Make sure JACK server is running)"
	@echo ""
	@# Start connection helper in background, then run the program in foreground
	@(sleep 0.8 && \
	  echo "[Connecting to system playback...]" && \
	  jack_connect subcollider_supersaw:output_L system:playback_1 2>/dev/null && \
	  jack_connect subcollider_supersaw:output_R system:playback_2 2>/dev/null && \
	  echo "[Connected to system:playback_1 and system:playback_2]" \
	) & \
	./$(BUILD_DIR)/example_supersaw

# Backward compatibility alias
.PHONY: supersaw
supersaw: example-supersaw

# Clean build directory
.PHONY: clean
clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"
