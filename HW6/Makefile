CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2

all: hogwarts wizard professor

hogwarts: hogwarts.c common.c
	$(CC) $(CFLAGS) -o $@ $^

wizard: wizard.c common.c
	$(CC) $(CFLAGS) -o $@ $^

professor: professor.c common.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f hogwarts wizard professor *.o