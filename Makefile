C_FLAGS = `sdl2-config --cflags` -g -W -Wall -Wextra -pedantic -O0 -I `sdl2-config --prefix`/include/

default: build
	./out/chip8

build:
	mkdir -p out
	/usr/bin/clang ${C_FLAGS} chip8.c -o out/chip8

clean:
	rm -rf out
