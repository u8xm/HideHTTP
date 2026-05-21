#ifndef NETWORK_H
#define NETWORK_H

#include <openssl/ssl.h>

typedef struct {
    int fd;
    int is_ssl;
    SSL *ssl_handle;
} ClientConn;

int start_server(int port);

ClientConn accept_connection(int server_fd, SSL_CTX *ctx);

#endif