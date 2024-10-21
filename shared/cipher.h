#include <assert.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define AES_KEY_LEN 32
#define AES_IV_LEN 12
#define AES_TAG_LEN 16

#define RSA_KEYLEN 256
#define RSA_MAXLEN 256
#define RSA_PADDING_SIZE 42  // RSA_PKCS1_OAEP_PADDING

#define RSA_PUBLIC 0
#define RSA_PRIVATE 1
/*
    Encrypts plaintext to ciphertext, using the given key and iv.
    output length: [0, plaintext_len + cipher_block_size]
*/

int64_t aes_256_gcm_encrypt(uint8_t *plaintext,
                            uint32_t plaintext_len,
                            uint8_t *key,
                            uint8_t *iv,
                            uint8_t *tag,
                            uint8_t *ciphertext);

int64_t aes_256_gcm_decrypt(uint8_t *ciphertext,
                            uint32_t ciphertext_len,
                            uint8_t *key,
                            uint8_t *iv,
                            uint8_t *tag,
                            uint8_t *plaintext);

uint8_t *aes_256_rnd_key();

uint8_t *aes_256_rnd_iv();

uint8_t *aes_256_rnd_tag();

int64_t rsa_2048_encrypt(uint8_t *plaintext,
                         uint64_t plaintext_len,
                         EVP_PKEY *public_key,
                         uint8_t **ciphertext);

int64_t rsa_2048_decrypt(uint8_t *ciphertext,
                         uint64_t ciphertext_len,
                         EVP_PKEY *private_key,
                         uint8_t **plaintext);

EVP_PKEY *rsa_2480_rnd_keypair();
/*
    success -> 1
    error -> 0
*/
int rsa_write_key(char *path, EVP_PKEY *pkey, int type);

/*
    success -> 1
    error -> 0
*/
int rsa_fwrite_key(FILE *fp, EVP_PKEY *pkey, int type);
/*
    success -> PKEY
    error -> NULL
*/
EVP_PKEY *rsa_read_key(char *path, int type);

/*
    success -> PKEY
    error -> NULL
*/
EVP_PKEY *rsa_fread_key(FILE *fp, int type);

// Generate cryptographically strong private bytes
void SECURE_RND_BYTES(uint8_t *buffer, uint32_t len);
