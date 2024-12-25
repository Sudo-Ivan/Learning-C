#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <memory.h>
#include <sodium.h>

// 16KB chunks - good balance for most files
#define CHUNK_SIZE 16384

// Crypto overhead: auth tag + nonce
#define CRYPTO_OVERHEAD (crypto_secretbox_MACBYTES + crypto_secretbox_NONCEBYTES)

// Buffer needs to hold chunk + crypto data
#define BUFFER_SIZE (CHUNK_SIZE + CRYPTO_OVERHEAD)

// Error codes
#define ERROR_FILE_OPEN -1
#define ERROR_SODIUM_INIT -2
#define ERROR_MEMORY -3
#define ERROR_CRYPTO -4

// Helper to print errors and exit
#define FATAL_ERROR(msg) do { \
    fprintf(stderr, "Error: %s\n", msg); \
    exit(1); \
} while(0)

#endif 