SRCS=shell.c tokenizer.c
EXECUTABLES=shell

CC=gcc
CFLAGS=-g -Wall -std=gnu99
LDFLAGS=

OBJS=$(SRCS:.c=.o)

all: $(EXECUTABLES)

$(EXECUTABLES): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

test: test_token.c tokenizer.c
	gcc -g -c tokenizer.c -std=c11 -Wall -o tokenizer.o
	gcc -g -c test_token.c -std=c11 -Wall -o test_token.o
	gcc -g test_token.o tokenizer.o -std=c11 -Wall -o test
	

clean:
	rm -rf $(EXECUTABLES) $(OBJS) test test_token.o
