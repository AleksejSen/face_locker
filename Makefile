# Detect number of processors for parallel testing
JOBS := $(shell nproc 2>/dev/null || echo 1)
CHECK_STATUS = && echo "\n === RECOGNITION SUCCESS ===" || echo "\n=== RECOGNITION FAILED ==="

all: build

build:
	git submodule update --init --recursive
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build -j\$(JOBS)
	ln -sf build/compile_commands.json .

run_false:
	@./build/bin/FaceLocker \
		--reference_picture pictures/rambo.jpg \
		--debug 
		$(CHECK_STATUS)

run:
	@./build/bin/FaceLocker \
		--reference_picture pictures/my_pic.jpeg \
		--debug \
		$(CHECK_STATUS)

run_multi:
	@./build/bin/FaceLocker \
		--reference_picture pictures/family.jpeg \
		--debug \
		$(CHECK_STATUS)

test:
	@echo "Running tests in parallel using \$(JOBS) jobs..."
	@cd build && ctest -j\$(JOBS) --output-on-failure

clean:
	rm -rf build compile_commands.json

.PHONY: build run_true run_false run_me run_family test clean all

