#include "krauk_server_res.h"

int handle_new_user(SERVER_CTX *s_ctx, USER_BASE *ub) {
    USER_INFO *user;  // new user
    uint8_t u_path[USER_ID_LEN + 14]; // 14 = hardcoded lenght to user file location
    FILE *config_fp;

    // adds new user to user base
    user = USER_BASE_new_user(ub);

    // creates user config file
    sprintf(u_path, "%s/%s", P_USERS, user->id);
    config_fp = fopen(u_path, "w+");

    if (config_fp == NULL) {
        perror("[-] Failed to create user file");
        return -1;
    }

    // writes public key to config
    if (rsa_fwrite_key(config_fp, s_ctx->public_key, RSA_PUBLIC) == 0) {
        perror("[-] Failed to write server public key to user config");
        return -1;
    }

    // writes id and user specific key to config file
    fwrite(user->id, sizeof(uint8_t), USER_ID_LEN, config_fp);
    if (rsa_fwrite_key(config_fp, user->MAC_public_key, RSA_PUBLIC) == 0) {
        perror("[-] Failed to write shared public key to user config");
        return -1;
    }
    if (rsa_fwrite_key(config_fp, user->MAC_private_key, RSA_PRIVATE) == 0) {
        perror("[-] Failed to write shared private key to user config");
        return -1;
    }

    printf("Generated user [%s], copy file to client!\n", u_path);

    // cleanup
    fclose(config_fp);

    return 0;
}
