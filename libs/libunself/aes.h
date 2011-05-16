#ifndef QEMU_AES_H
#define QEMU_AES_H

#include "tools.h"

#define AES_MAXNR         14
#define AES_BLOCK_SIZE    16

struct aes_key_st
{
    uint32_t rd_key[4 * (AES_MAXNR + 1)];
    int      rounds;
};
typedef struct aes_key_st   AES_KEY;

int AES_set_encrypt_key(const uint8_t *userKey, const int bits,
                        AES_KEY *key);
int AES_set_decrypt_key(const uint8_t *userKey, const int bits,
                        AES_KEY *key);

void AES_encrypt(const uint8_t *in, uint8_t *out,
                 const AES_KEY *key);
void AES_decrypt(const uint8_t *in, uint8_t *out,
                 const AES_KEY *key);

void AES_cbc256_decrypt(uint8_t *key, uint8_t *iv,
                        uint8_t *in, uint64_t len, uint8_t *out);
void AES_ctr128_encrypt(uint8_t *key, uint8_t *iv,
                        uint8_t *in, uint64_t len, uint8_t *out);

#endif
