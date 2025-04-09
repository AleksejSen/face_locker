# Dependencies

Look at `flake.nix` for the dependencies.


# Building

```sh
conan install . --build=missing -pr:b=default -pr:h=default -of build
cmake -B "build" -S . -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/ofc
```
