.PHONY: clean tests build run

clean:
        rm -rf build/*

tests:
        mkdir -p build && cd build && cmake .. -DBUILD_TESTS=ON && make -j$(nproc) && ctest --output-on-fai>

build:
        mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

run:
        ./build/DAWAudioEngine_artefacts/Release/DAWAudioEngine