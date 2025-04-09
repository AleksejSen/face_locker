#!/bin/bash
echo Building...
conan install . --output-folder=build --build=missing -pr:b=debug -pr:h=debug
cmake -B "build" -S . -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
cp build/compile_commands.json .

