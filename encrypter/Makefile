CC = gcc
CFLAGS = -Wall -Wextra

all: encrypter

encrypter: encrypter.c
	$(CC) $(CFLAGS) -o encrypter encrypter.c -lmta_crypt -lmta_rand -lcrypto

clean:
	rm -f encrypter
