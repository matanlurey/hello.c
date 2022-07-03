default: build
	./out/hello_world

build:
	mkdir -p out
	/usr/bin/clang -Wall -Wextra -g hello_world.c -o out/hello_world

clean:
	rm -rf out
