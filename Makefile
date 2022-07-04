CC      = clang
C_FLAGS = `sdl2-config --cflags` -g -W -Wall -Wextra -pedantic -O0 -I `sdl2-config --prefix`/include/
MAIN    = chip8.c

default: build
	./out/chip8

build:
	mkdir -p out
	${CC} ${C_FLAGS} ${MAIN} -o out/chip8

clean:
	rm -rf out lib
