CC      = clang
C_FLAGS = -I /opt/homebrew/include $(shell sdl2-config --libs) -g -W -Wall -Wextra -pedantic -O0
MAIN    = chip8.c

default: build
	./out/chip8

build:
	mkdir -p out
	${CC} ${C_FLAGS} ${MAIN} -o out/chip8

clean:
	rm -rf out lib
