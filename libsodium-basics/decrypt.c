#include "common.h"
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input> <output> <key>\n", argv[0]);
        return 1;
    }

    if (sodium_init() < 0) {
        FATAL_ERROR("libsodium init failed");
    }

    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    if (sodium_hex2bin(key, sizeof(key), argv[3], strlen(argv[3]), NULL, NULL, NULL) != 0) {
        FATAL_ERROR("Invalid key format");
    }

    FILE *input = fopen(argv[1], "rb");
    if (!input) {
        FATAL_ERROR("Failed to open input file");
    }

    FILE *output = fopen(argv[2], "wb");
    if (!output) {
        fclose(input);
        FATAL_ERROR("Failed to open output file");
    }

    unsigned char header[crypto_secretstream_xchacha20poly1305_HEADERBYTES];
    crypto_secretstream_xchacha20poly1305_state state;

    if (fread(header, 1, sizeof(header), input) != sizeof(header)) {
        fclose(input);
        fclose(output);
        FATAL_ERROR("Failed to read header");
    }

    if (crypto_secretstream_xchacha20poly1305_init_pull(&state, header, key) != 0) {
        fclose(input);
        fclose(output);
        FATAL_ERROR("Incomplete header");
    }

    unsigned char buffer[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    unsigned char decrypted[CHUNK_SIZE];
    size_t bytes_read;
    unsigned char tag;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input)) > 0) {
        if (ferror(input)) {
            fclose(input);
            fclose(output);
            FATAL_ERROR("Error reading input file");
        }

        unsigned long long decrypted_len;
        if (crypto_secretstream_xchacha20poly1305_pull(
                &state, decrypted, &decrypted_len, &tag,
                buffer, bytes_read, NULL, 0) != 0) {
            fclose(input);
            fclose(output);
            FATAL_ERROR("Decryption failed");
        }

        if (fwrite(decrypted, 1, decrypted_len, output) != decrypted_len) {
            fclose(input);
            fclose(output);
            FATAL_ERROR("Failed to write decrypted data");
        }

        if (tag == crypto_secretstream_xchacha20poly1305_TAG_FINAL) {
            break;
        }
    }

    sodium_memzero(key, sizeof(key));
    sodium_memzero(buffer, sizeof(buffer));
    sodium_memzero(decrypted, sizeof(decrypted));

    fclose(input);
    fclose(output);

    return 0;
} 