# Detect number of processors for parallel testing
JOBS := $(shell nproc 2>/dev/null || echo 1)

all: build

build:
	git submodule update --init --recursive
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build -j\$(JOBS)
	ln -sf build/compile_commands.json .

run:
	./build/bin/FaceLocker --debug --reference_picture pictures/arnold_t1.jpeg

test:
	@echo "Running tests in parallel using \$(JOBS) jobs..."
	@cd build && ctest -j\$(JOBS) --output-on-failure

clean:
	rm -rf build compile_commands.json

.PHONY: build run test clean all

