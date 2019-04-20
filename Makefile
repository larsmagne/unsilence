CPPFLAGS= -I/usr/local/include -O3 -Wall
LDFLAGS= -L/usr/local/lib -lsndfile
CC = gcc 

all: unsilence

unsilence.o: unsilence.c

unsilence: unsilence.o
	$(CC) $(CPPFLAGS) -o unsilence $(LDFLAGS) unsilence.o

clean:
	rm -f unsilence *.o

TAGS: *.h *.c
	etags *.[ch]
