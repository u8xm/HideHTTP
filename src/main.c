#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include "../include/network.h"
#include "../include/ssl_module.h"
#include "../include/http.h"
#include "../include/utils.h"

int main() {
    int http_port = 8080;
    int https_port = 8443;
    const char *root = "./public";

    SSL_CTX *ctx = init_ssl_context("server.crt", "server.key");

    system("clear");

    int http_fd = start_server(http_port);
    int https_fd = (ctx) ? start_server(https_port) : -1;

    print_banner(http_port, (ctx ? https_port : 0), root);

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(http_fd, &fds);
        int max_fd = http_fd;
        
        if (https_fd != -1) {
            FD_SET(https_fd, &fds);
            if (https_fd > max_fd) max_fd = https_fd;
        }

        if (select(max_fd + 1, &fds, NULL, NULL, NULL) < 0) continue;

        if (FD_ISSET(http_fd, &fds)) {
            ClientConn c = accept_connection(http_fd, NULL);
            if (c.fd >= 0) {
                handle_request(c, root);
                close(c.fd);
            }
        }

        if (https_fd != -1 && FD_ISSET(https_fd, &fds)) {
            ClientConn c = accept_connection(https_fd, ctx);
            if (c.fd >= 0) {
                handle_request(c, root);
                SSL_shutdown(c.ssl_handle);
                SSL_free(c.ssl_handle);
                close(c.fd);
            }
        }
    }

    if (ctx) cleanup_ssl(ctx);
    return 0;
}