#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/network.h"

int start_server(int port) {
    int fd;
    struct sockaddr_in addr;
    int opt = 1;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    if (listen(fd, 64) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

ClientConn accept_connection(int server_fd, SSL_CTX *ctx) {
    ClientConn conn = {0};
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    
    conn.fd = accept(server_fd, (struct sockaddr *)&addr, &len);
    if (conn.fd < 0) return conn;

    if (ctx != NULL) {
        conn.ssl_handle = SSL_new(ctx);
        SSL_set_fd(conn.ssl_handle, conn.fd);
        if (SSL_accept(conn.ssl_handle) <= 0) {
            SSL_free(conn.ssl_handle);
            close(conn.fd);
            conn.fd = -1;
            return conn;
        }
        conn.is_ssl = 1;
    } else {
        conn.is_ssl = 0;
        conn.ssl_handle = NULL;
    }
    return conn;
}