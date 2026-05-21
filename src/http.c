#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/http.h"
#include "../include/utils.h"

#define BUF_SIZE 8192

void send_response(ClientConn client, const char *status, const char *type, const char *body, long len) {
    char header[512];
    int hlen = sprintf(header, "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", status, type, len);
    
    if (client.is_ssl) {
        SSL_write(client.ssl_handle, header, hlen);
        if (body && len > 0) SSL_write(client.ssl_handle, body, len);
    } else {
        write(client.fd, header, hlen);
        if (body && len > 0) write(client.fd, body, len);
    }
}

void handle_request(ClientConn client, const char *root) {
    char buffer[BUF_SIZE];
    int nread = client.is_ssl ? SSL_read(client.ssl_handle, buffer, BUF_SIZE - 1) : read(client.fd, buffer, BUF_SIZE - 1);
    
    if (nread <= 0) return;
    buffer[nread] = '\0';

    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) < 2) return;

    if (strstr(path, "..")) {
        send_response(client, "403 Forbidden", "text/plain", "Forbidden", 9);
        return;
    }

    char full_path[512];
    sprintf(full_path, "%s%s", root, strcmp(path, "/") == 0 ? "/index.html" : path);

    int fd = open(full_path, O_RDONLY);
    if (fd < 0) {
        send_response(client, "404 Not Found", "text/plain", "Not Found", 9);
    } else {
        struct stat st;
        fstat(fd, &st);
        send_response(client, "200 OK", get_mime_type(full_path), NULL, st.st_size);
        
        while ((nread = read(fd, buffer, BUF_SIZE)) > 0) {
            if (client.is_ssl) SSL_write(client.ssl_handle, buffer, nread);
            else write(client.fd, buffer, nread);
        }
        close(fd);
    }
}