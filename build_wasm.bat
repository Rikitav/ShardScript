emcmake cmake -S . -B out/build/wasm-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF
cmake --build out/build/wasm-release --target ShardScript -j 4
em++ -s SIDE_MODULE=1 -s WASM=1 -Wl,--whole-archive out/build/wasm-release/bin/libShardScript.a -o out/build/wasm-release/bin/ShardScript.wasm