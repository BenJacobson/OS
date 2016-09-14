
all: | bin
	gcc --std=gnu99 -o bin/a.out src/os345*.c

debug: | bin
	gcc -g --std=gnu99 -o bin/a.out src/os345*.c

clean:
	rm bin/*

bin:
	mkdir bin