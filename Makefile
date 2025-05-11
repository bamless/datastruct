all: main

main: main.c arena.c alloc.c context.c
	# gcc -fsanitize=address -Wall -Wextra -std=c99 -ggdb $^ -o main
	gcc -Wall -Wextra -std=c99 -ggdb $^ -o main

.PHONY: clean
clean:
	rm -rf main
