CC := gcc
CFLAGS := -W -Wall -Wextra -O2

colr: colr.c
	$(CC) $(CFLAGS) -o $@ $^
