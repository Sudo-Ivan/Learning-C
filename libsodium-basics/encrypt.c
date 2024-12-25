#include "common.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        return 1;
    }

    if (sodium_init() < 0) {
        FATAL_ERROR("libsodium init failed");
    }

    unsigned char key[crypto_secretstream_xchacha20poly1305_KEYBYTES];
    crypto_secretbox_keygen(key);

    printf("Key: ");
    for(unsigned int i = 0; i < crypto_secretstream_xchacha20poly1305_KEYBYTES; i++) {
        printf("%02x", key[i]);
    }
    printf("\n");

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
    crypto_secretstream_xchacha20poly1305_init_push(&state, header, key);

    if (fwrite(header, 1, sizeof(header), output) != sizeof(header)) {
        fclose(input);
        fclose(output);
        FATAL_ERROR("Failed to write header");
    }

    unsigned char buffer[CHUNK_SIZE];
    unsigned char cipher[CHUNK_SIZE + crypto_secretstream_xchacha20poly1305_ABYTES];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, input)) > 0) {
        if (ferror(input)) {
            fclose(input);
            fclose(output);
            FATAL_ERROR("Error reading input file");
        }

        unsigned long long cipher_len;
        crypto_secretstream_xchacha20poly1305_push(
            &state, cipher, &cipher_len, buffer, bytes_read,
            NULL, 0, bytes_read < CHUNK_SIZE ? 
            crypto_secretstream_xchacha20poly1305_TAG_FINAL : 0);

        if (fwrite(cipher, 1, cipher_len, output) != cipher_len) {
            fclose(input);
            fclose(output);
            FATAL_ERROR("Failed to write encrypted chunk");
        }
    }

    sodium_memzero(key, sizeof(key));
    sodium_memzero(buffer, sizeof(buffer));
    sodium_memzero(cipher, sizeof(cipher));

    fclose(input);
    fclose(output);
    return 0;
}   