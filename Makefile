CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -g -lm

flows: flows.c
	$(CC) $(CFLAGS) $^ -o "$@"

clean:
	rm flows