.PHONY: clean tests build

clean:
	rm -rf build/*

tests:
	cd /home/ugo/dev/daw/backend && mkdir -p build && cd build && cmake .. -DBUILD_TESTS=ON && make -j$(nproc) && ctest --output-on-failure --verbose

build:
	mkdir -p build && cd build && cmake .. && make -j$(nproc)