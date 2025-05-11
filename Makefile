all: main threads

main: main.c arena.c alloc.c context.c
	# gcc -fsanitize=address -Wall -Wextra -std=c99 -ggdb $^ -o main
	gcc -Wall -Wextra -std=c99 -ggdb $^ -o main

threads: threads.c arena.c alloc.c context.c
	gcc -Wall -Wextra -std=c11 -ggdb $^ -o threads

.PHONY: clean
clean:
	rm -rf main
