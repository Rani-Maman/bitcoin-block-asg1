CC = gcc
CFLAGS = -Wall -Wextra

all: decrypter

decrypter: decrypter.c
	$(CC) $(CFLAGS) -o decrypter decrypter.c -lmta_crypt -lmta_rand -lcrypto

clean:
	rm -f decrypter
