.PHONY: clean tests build run watch

clean:
	rm -rf build/*

tests:
	mkdir -p build && cd build && cmake .. -DBUILD_TESTS=ON && make -j$(nproc) && ctest --output-on-failure

build:
	mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc)

run:
	./build/DAWAudioEngine_artefacts/Release/DAWAudioEngine

watch:
	watchexec -r -e cpp,hpp "cd build && make -j$$(nproc) && ./DAWAudioEngine_artefacts/Release/DAWAudioEngine"