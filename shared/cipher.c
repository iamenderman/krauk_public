#include "cipher.h"

uint8_t *rnd(uint32_t size);

/*
    Interface definition
*/
int64_t aes_256_gcm_encrypt(uint8_t *plaintext, uint32_t plaintext_len, uint8_t *key, uint8_t *iv, uint8_t *tag, uint8_t *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    uint32_t ciphertext_len;
    int32_t len;

    // creates cipher context
    // EVP_CIPHER_CTX_init(ctx);

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        fprintf(stderr, "[-] Failed to create new context\n");
        return -1;
    }

    // init aes256-gcm encryption
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
        fprintf(stderr, "[-] Failed to set cipher algorithm \n");
        return -1;
    }

    // sets the key and iv
    if (1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) {
        fprintf(stderr, "[-] Failed to set key & iv \n");
        return -1;
    }

    // encrypts the given plaintext
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1) {
        fprintf(stderr, "[-] Failed to encrypt \n");
        return -1;
    }
    // counts the number of bytes encoded
    ciphertext_len = len;

    // finished the encryption
    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1) {
        fprintf(stderr, "[-] Failed finish encryption \n");
        return -1;
    }
    ciphertext_len += len;

    /* Get the tag */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_LEN, tag) != 1) {
        fprintf(stderr, "[-] Failed to get tag \n");
        return -1;
    }
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int64_t aes_256_gcm_decrypt(uint8_t *ciphertext, uint32_t ciphertext_len, uint8_t *key, uint8_t *iv, uint8_t *tag, uint8_t *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int32_t plaintext_len;
    int32_t len;
    int result = -1;

    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        fprintf(stderr, "[-] Failed to create new context\n");
        return -1;
    }

    // init aes256-gcm decryption
    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL)) {
        fprintf(stderr, "[-] Failed to set cipher algorithm \n");
        return -1;
    }

    // sets the key and iv
    if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) {
        fprintf(stderr, "[-] Failed to set key & iv \n");
        return -1;
    }

    // decrypts the given plaintext
    if (!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        fprintf(stderr, "[-] Failed to decrypt \n");
        return -1;
    }
    plaintext_len = len;

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_LEN, tag)) {
        fprintf(stderr, "[-] Failed to set tag \n");
        return -1;
    }

    /*
     * Finalise the decryption. A positive return value indicates success,
     * anything else is a failure - the plaintext is not trustworthy.
     */
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) > 0) {
        plaintext_len += len;
        result = plaintext_len;
    } else {
        fprintf(stderr, "[-] INVALID TAG\n");
    }

    // cleanup
    EVP_CIPHER_CTX_free(ctx);

    return result;
}

int64_t rsa_2048_encrypt(uint8_t *plaintext, uint64_t plaintext_len, EVP_PKEY *public_key, uint8_t **ciphertext) {
    EVP_PKEY_CTX *ctx;
    uint8_t *out = NULL;
    uint64_t outlen = 0;

    // creates context based on key
    ctx = EVP_PKEY_CTX_new(public_key, NULL);
    if (!ctx) {
        perror("[-] Failed to create context based on key");
        return -1;
    }

    if (EVP_PKEY_encrypt_init(ctx) <= 0) {
        perror("[-] Failed to init encryption");
        return -1;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        perror("[-] Failed to set rsa padding");
        return -1;
    }

    /* Determine buffer length */
    if (EVP_PKEY_encrypt(ctx, NULL, &outlen, plaintext, plaintext_len) <= 0) {
        perror("[-] Failed to determine buffer length");
        return -1;
    }

    out = malloc(outlen);
    if (!out) {
        perror("[-] Failed to malloc");
        return -1;
    }

    if (EVP_PKEY_encrypt(ctx, out, &outlen, plaintext, plaintext_len) <= 0) {
        puts("[-] Failed to encrypt given plaintext");
        return -1;
    }

    *ciphertext = out;

    // cleanup
    EVP_PKEY_CTX_free(ctx);

    return outlen;
}

int64_t rsa_2048_decrypt(uint8_t *ciphertext, uint64_t ciphertext_len, EVP_PKEY *private_key, uint8_t **plaintext) {
    EVP_PKEY_CTX *ctx;
    uint8_t *out;
    uint64_t outlen;

    ctx = EVP_PKEY_CTX_new(private_key, NULL);
    if (!ctx) {
        perror("[-] Failed to create context based on key");
        return -1;
    }

    if (EVP_PKEY_decrypt_init(ctx) <= 0) {
        perror("[-] Failed to init decryption");
        return -1;
    }

    if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
        perror("[-] Failed to set rsa padding");
        return -1;
    }

    /* Determine buffer length */
    if (EVP_PKEY_decrypt(ctx, NULL, &outlen, ciphertext, ciphertext_len) <= 0) {
        perror("[-] Failed to determine buffer length");
        return -1;
    }

    out = malloc(outlen);
    if (!out) {
        perror("[-] Failed to malloc");
        return -1;
    }

    if (EVP_PKEY_decrypt(ctx, out, &outlen, ciphertext, ciphertext_len) <= 0) {
        puts("[-] Failed to decrypt given plaintext");
        return -1;
    }

    *plaintext = out;

    // cleanup
    EVP_PKEY_CTX_free(ctx);

    return outlen;
}

EVP_PKEY *rsa_2480_rnd_keypair() {
    EVP_PKEY_CTX *ctx;
    EVP_PKEY *pkey = NULL;

    // createsc context based on keytype_id
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);

    if (!ctx) {
        perror("[-] Failed to create key generation evnioment: ");
        return NULL;
    }
    // inints key algortihm
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        perror("[-] Failed to setup PKEY with given enviromemt: ");
        return NULL;
    }

    // sets number of bits
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 256 * 8) <= 0) {
        perror("[-] Failed to set number of given bits: ");
        return NULL;
    }

    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        perror("[-] Failed to key genreation ");
        return NULL;
    }

    EVP_PKEY_CTX_free(ctx);

    return pkey;
}

/*
    i2d_PrivateKey_fp() return 1 if successfully encoded or zero if an error occurs.
*/
// i2d_PUBKEY_fp() and i2d_PUBKEY_bio() return 1 if successfully encoded or 0 if an error occurs.

int rsa_write_key(char *path, EVP_PKEY *pkey, int type) {
    FILE *fp;
    int res;

    if (access(path, F_OK) == -1) {
        fp = fopen(path, "w+");
    } else {
        fp = fopen(path, "r+");
    }
    res = rsa_fwrite_key(fp, pkey, type);

    // cleanup
    fclose(fp);

    return res;
}

int rsa_fwrite_key(FILE *fp, EVP_PKEY *pkey, int type) {
    if (type == RSA_PUBLIC) {
        return i2d_PUBKEY_fp(fp, pkey);
    } else if (type == RSA_PRIVATE) {
        return i2d_PrivateKey_fp(fp, pkey);
    }

    return -2;
}

EVP_PKEY *rsa_read_key(char *path, int type) {
    FILE *fp;
    EVP_PKEY *key;

    if (access(path, F_OK) == -1) {
        fp = fopen(path, "w+");
    } else {
        fp = fopen(path, "r+");
    }

    key = rsa_fread_key(fp, type);

    // cleanup
    fclose(fp);

    return key;
}

EVP_PKEY *rsa_fread_key(FILE *fp, int type) {
    if (type == RSA_PUBLIC) {
        return d2i_PUBKEY_fp(fp, NULL);
    } else if (type == RSA_PRIVATE) {
        return d2i_PrivateKey_fp(fp, NULL);
    }

    return NULL;
}

void SECURE_RND_BYTES(uint8_t *buffer, uint32_t len) {
    if (RAND_priv_bytes_ex(NULL, buffer, len, 0) != 1) {
        perror("[-] Failed to generate random bytes");
        exit(EXIT_FAILURE);
    }
}

/*
    Assist
*/
uint8_t *rnd(uint32_t size) {
    uint8_t *rnd_arr = calloc(size, sizeof(uint8_t));

    if (RAND_priv_bytes_ex(NULL, rnd_arr, size, 0) != 1) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return rnd_arr;
}