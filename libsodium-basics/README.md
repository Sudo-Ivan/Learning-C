# libsodium-basics

Docs: https://doc.libsodium.org/quickstart

## Makefile

-Wall -Wextra -Werror: Catch all warnings as errors
-O2: Optimization level 2
-fstack-protector-strong: Stack protection
-D_FORTIFY_SOURCE=2: Runtime buffer checks
-fsanitize: Runtime memory/behavior checks
-lsodium: Link against libsodium


## Blueprint

I am gonna create a blueprint to define how I want to structure my project. Lets keep things simple.

`common.h` -> common functions
`encrypt.c` -> encrypt function
`decrypt.c` -> decrypt function

## common header

Add headers for file operations, memory operations, and libsodium.

```c
#include <stdio.h>
#include <memory.h>
#include <sodium.h>
```

Constants for streaming encryption:

```c
#define CHUNK_SIZE 16384
#define CRYPTO_OVERHEAD crypto_secretstream_xchacha20poly1305_ABYTES
#define BUFFER_SIZE (CHUNK_SIZE + CRYPTO_OVERHEAD)
```

Why powers of 2?

- Memory alignment is optimal
- CPU cache lines work better
- OS page sizes are also powers of 2


CRYPTO_OVERHEAD is space needed for:

- MAC (Message Authentication Code) - ensures integrity
- Nonce - ensures unique encryption


## encryption

- Initialize libsodium
- Generate key
- Open input and output files
- Initialize secretstream state
- Write header to output
- Process file in chunks:
  - Read chunk
  - Encrypt with secretstream
  - Mark final chunk with TAG_FINAL
  - Write encrypted chunk
- Secure cleanup

see `encrypt.c` for more details.

## decryption

- Initialize libsodium
- Parse hex key from args
- Open input and output files
- Read header
- Initialize secretstream pull state
- Process file in chunks:
  - Read encrypted chunk
  - Decrypt with secretstream
  - Check for TAG_FINAL
  - Write decrypted chunk
- Secure cleanup

see `decrypt.c` for more details.

## Running

```bash
make
./encrypt input.txt output.txt
./decrypt output.txt decrypted.txt <key>
```

## Basically

- Streaming encryption for large files
- Authenticated encryption (XChaCha20-Poly1305)
- Proper chunk handling with final tag
- Secure memory wiping
- Header-based stream initialization