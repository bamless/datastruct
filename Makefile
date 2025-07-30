CC=gcc
CFLAGS=-Wall -Wextra -ggdb -pedantic
LDFLAGS=

.PHONY: all
all: main threads wasm.wasm

main: main.c extlib.h
	$(CC) $(CFLAGS) -std=c99 $(LDFLAGS) main.c -o main

test/test: ./test/test.c ./test/ctest.h extlib.h
	$(CC) $(CFLAGS) -Wno-attributes -Wno-pragmas -std=c99 $(LDFLAGS) -I./test/ ./test/test.c -o test/test

.PHONY: test
test: test/test 
	./test/test

threads: threads.c extlib.h
	$(CC) $(CFLAGS) -std=c11 $(LDFLAGS) threads.c -o threads

wasm.wasm: wasm.c extlib.h
	clang $(CFAGS) -D EXTLIB_WASM=1 \
		-std=c99 -fno-builtin --target=wasm32 --no-standard-libraries \
		-Wl,--no-entry -Wl,--export=make_array -Wl,--export=ext_temp_reset \
		-Wl,--export=__heap_base -Wl,--allow-undefined \
		wasm.c -o wasm.wasm

.PHONY: clean
clean:
	rm -rf main
	rm -rf threads
	rm -rf wasm.wasm
	rm -rf test/test
