CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: encrypt.out

encrypt.out: main.o shared.o server.o client.o
	$(CC) $(CFLAGS) -o encrypt.out main.o shared.o server.o client.o -lmta_crypt -lmta_rand -lcrypto

main.o: main.c shared.h server.h client.h
	$(CC) $(CFLAGS) -c main.c

shared.o: shared.c shared.h
	$(CC) $(CFLAGS) -c shared.c

server.o: server.c server.h shared.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c client.h shared.h
	$(CC) $(CFLAGS) -c client.c

clean:
	rm -f *.o encrypt.out
