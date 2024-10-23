#include "user.h"

void USER_BASE_add_user(USER_BASE *ub, USER_INFO *user);
bool user_id_taken(USER_BASE *ub, uint8_t *user_id);

USER_BASE *USER_BASE_deserialize(char *ub_file_path) {
    USER_BASE *ub;
    int32_t i = 0;
    uint8_t id_buffer[USER_ID_LEN];

    ub = calloc(1, sizeof(USER_BASE));
    ub->user_count = 0;

    // creates user file if needed
    if (access(ub_file_path, F_OK) == -1) {
        ub->user_fp = fopen(ub_file_path, "w+");
    } else {
        ub->user_fp = fopen(ub_file_path, "r+");
    }

    while (fread(id_buffer, 1, USER_ID_LEN, ub->user_fp) == USER_ID_LEN) {
        // this is slow :(
        ub->user_count++;
        ub->users = realloc(ub->users, sizeof(USER_INFO *) * (ub->user_count));
        ub->users[i] = calloc(1, sizeof(USER_INFO));

        memcpy(ub->users[i]->id, id_buffer, USER_ID_LEN);
        ub->users[i]->id[USER_ID_LEN] = '\0';

        ub->users[i]->MAC_public_key = rsa_fread_key(ub->user_fp, RSA_PUBLIC);
        if (ub->users[i]->MAC_public_key == NULL) {
            perror("[-] Failed to read user public key from user base file");
            exit(EXIT_FAILURE);
        }

        ub->users[i]->MAC_private_key = rsa_fread_key(ub->user_fp, RSA_PRIVATE);
        if (ub->users[i]->MAC_private_key == NULL) {
            perror("[-] Failed to read user private key from user base");
            exit(EXIT_FAILURE);
        }

        i++;
    }

    return ub;
}

void USER_BASE_free(USER_BASE *ub) {
    for (size_t i = 0; i < ub->user_count; i++) {
        EVP_PKEY_free(ub->users[i]->MAC_public_key);
        EVP_PKEY_free(ub->users[i]->MAC_private_key);
        free(ub->users[i]);
    }

    fclose(ub->user_fp);
    free(ub->users);
    free(ub);
}

USER_INFO *USER_BASE_new_user(USER_BASE *ub) {
    USER_INFO *user = calloc(1, sizeof(USER_INFO));
    uint8_t short_id[USER_ID_LEN / 2];

    // ugly mf code - generates user id
    do {
        SECURE_RND_BYTES(short_id, USER_ID_LEN / 2);
        for (size_t offset = 0; offset < USER_ID_LEN / 2; offset++) {
            sprintf(user->id + (2 * offset), "%02x", short_id[offset] & 0xff);
        }
        user->id[USER_ID_LEN] = '\0';
    } while (user_id_taken(ub, user->id));

    // generates pkey pair
    EVP_PKEY *pair = rsa_2480_rnd_keypair();

    // hacky solution to split the pair to the private and public part
    // TODO: find better solution :)
    if (rsa_write_key("publickey.temp", pair, RSA_PUBLIC) == 0) {
        perror("[-] Failed to write shared public key to tempfile");
        exit(EXIT_FAILURE);
    }
    if (rsa_write_key("privatekey.temp", pair, RSA_PRIVATE) == 0) {
        perror("[-] Failed to write shared private key to tempfile");
        exit(EXIT_FAILURE);
    }

    user->MAC_public_key = rsa_read_key("publickey.temp", RSA_PUBLIC);
    if (user->MAC_public_key == NULL) {
        perror("[-] Failed to read shared public from tempfile");
        exit(EXIT_FAILURE);
    }

    user->MAC_private_key = rsa_read_key("privatekey.temp", RSA_PRIVATE);
    if (user->MAC_private_key == NULL) {
        perror("[-] Failed to read shared private from tempfile");
        exit(EXIT_FAILURE);
    }

    if (remove("publickey.temp") == -1) {
        perror("[-] Failed to remove temp keyfile");
    }
    if (remove("privatekey.temp") == -1) {
        perror("[-] Failed to remove temp keyfile");
    }
    // cleanup
    EVP_PKEY_free(pair);
    //
    USER_BASE_add_user(ub, user);

    return user;
}

void USER_BASE_add_user(USER_BASE *ub, USER_INFO *user) {
    ub->users = realloc(ub->users, sizeof(USER_INFO *) * (ub->user_count + 1));
    ub->users[ub->user_count++] = user;

    // write user id
    fwrite(user->id, USER_ID_LEN, 1, ub->user_fp);

    if (rsa_fwrite_key(ub->user_fp, user->MAC_public_key, RSA_PUBLIC) == 0) {
        perror("[-] Failed to write shared public key to userbase file");
        exit(EXIT_FAILURE);
    }
    if (rsa_fwrite_key(ub->user_fp, user->MAC_private_key, RSA_PRIVATE) == 0) {
        perror("[-] Failed to write shared private key to userbase file");
        exit(EXIT_FAILURE);
    }
}

int USER_BASE_get_keys(USER_BASE *ub, CLIENT_CTX *ctx) {
    printf("USER BASE SIZE: %d\n", ub->user_count);

    for (size_t i = 0; i < ub->user_count; i++) {
        printf("user: %s\n", ub->users[i]);

        if (memcmp(ctx->id, ub->users[i], USER_ID_LEN) == 0) {
            // TODO: dont duplicate keys every time a client connets
            ctx->MAC_public_key = EVP_PKEY_dup(ub->users[i]->MAC_public_key);
            ctx->MAC_private_key = EVP_PKEY_dup(ub->users[i]->MAC_private_key);

            puts("[+] Found key");

            return 0;
        }
    }

    return -1;
}

bool user_id_taken(USER_BASE *ub, uint8_t *user_id) {
    for (size_t i = 0; i < ub->user_count; i++) {
        if (memcmp(user_id, ub->users[i]->id, USER_ID_LEN) == 0) {
            return true;
        }
    }
    return false;
}