# Detect number of processors for parallel testing
JOBS := $(shell nproc 2>/dev/null || echo 1)

all: build

build:
	git submodule update --init --recursive
	cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
	cmake --build build -j\$(JOBS)
	ln -sf build/compile_commands.json .

run_true:
	./build/bin/FaceLocker --debug --reference_picture pictures/arnold_t1.jpeg


run_false:
	./build/bin/FaceLocker --debug --reference_picture pictures/rambo.jpg


run_me:
	./build/bin/FaceLocker --debug --reference_picture pictures/my_pic.jpeg

run_family:
	./build/bin/FaceLocker --debug --reference_picture pictures/family.jpeg

test:
	@echo "Running tests in parallel using \$(JOBS) jobs..."
	@cd build && ctest -j\$(JOBS) --output-on-failure

clean:
	rm -rf build compile_commands.json

.PHONY: build run_true run_false run_me run_family test clean all

