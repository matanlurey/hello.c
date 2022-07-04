default: build
	./out/chip8

build:
	mkdir -p out
	/usr/bin/clang -Wall -Wextra -g chip8.c -o out/chip8

clean:
	rm -rf out
