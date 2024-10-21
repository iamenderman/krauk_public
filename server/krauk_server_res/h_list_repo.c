#include "krauk_server_res.h"

int handle_list_repos(KRAUK_FD ksc, CLIENT_CTX *ctx) {
    PATH_BUILDER *pb = PATH_BUILDER_new(ctx->id, 0, 0);
    file_info info = open_file(INFO_PATH(pb), O_RDWR | O_APPEND, MEM_DIRECT);
    PATH_BUILDER_free(pb);
    // sends file
    return krauk_send_file(ksc, ctx, info);
}