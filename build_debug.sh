cmake -S . -B out/build/x64-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-debug -j$(nproc)
