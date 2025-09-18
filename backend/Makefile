.PHONY: clean build

clean:
	rm -rf build/*

build: clean
	cd build
	cmake ..
	make -j$(nproc)