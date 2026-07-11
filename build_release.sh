cmake -S . -B out/build/x64-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/x64-release -j$(nproc)
