CFLAGS = -Wall -Wextra -Werror -O2 -fstack-protector-strong -D_FORTIFY_SOURCE=2
LDFLAGS = -lsodium
SANITIZE = -fsanitize=address -fsanitize=undefined

all: encrypt decrypt

encrypt: encrypt.c
	$(CC) $(CFLAGS) $(SANITIZE) $^ -o $@ $(LDFLAGS)

decrypt: decrypt.c
	$(CC) $(CFLAGS) $(SANITIZE) $^ -o $@ $(LDFLAGS)

clean:
	rm -f encrypt decrypt *.o 