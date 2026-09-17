#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "../include/http.h"
#include "../include/embedded_assets.h"
#include "../include/utils.h"

#define BUF_SIZE 8192

static int write_client(ClientConn client, const void *data, size_t length) {
    const char *cursor = data;

    while (length > 0) {
        int written = client.is_ssl
            ? SSL_write(client.ssl_handle, cursor, length)
            : write(client.fd, cursor, length);
        if (written <= 0) return -1;
        cursor += written;
        length -= (size_t) written;
    }
    return 0;
}

void send_response(ClientConn client, const char *status, const char *type, const char *body, long len) {
    char header[512];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.1 %s\r\nContent-Type: %s\r\nContent-Length: %ld\r\n"
        "Cache-Control: no-cache\r\nConnection: close\r\n\r\n",
        status, type, len);

    if (hlen > 0) write_client(client, header, (size_t) hlen);
    if (body && len > 0) write_client(client, body, (size_t) len);
}

void handle_request(ClientConn client, const char *root) {
    char buffer[BUF_SIZE];
    int nread = client.is_ssl ? SSL_read(client.ssl_handle, buffer, BUF_SIZE - 1) : read(client.fd, buffer, BUF_SIZE - 1);
    
    if (nread <= 0) return;
    buffer[nread] = '\0';

    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) < 2) {
        send_response(client, "400 Bad Request", "text/plain; charset=utf-8", "Bad Request", 11);
        return;
    }

    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        send_response(client, "405 Method Not Allowed", "text/plain; charset=utf-8", "Method Not Allowed", 18);
        return;
    }

    char *query = strchr(path, '?');
    if (query) *query = '\0';

    if (strstr(path, "..")) {
        send_response(client, "403 Forbidden", "text/plain; charset=utf-8", "Forbidden", 9);
        return;
    }

    char full_path[512];
    int path_length = snprintf(full_path, sizeof(full_path), "%s%s", root,
        strcmp(path, "/") == 0 ? "/index.html" : path);
    if (path_length < 0 || (size_t) path_length >= sizeof(full_path)) {
        send_response(client, "414 URI Too Long", "text/plain; charset=utf-8", "URI Too Long", 12);
        return;
    }

    int fd = open(full_path, O_RDONLY);
    if (fd < 0) {
        if (strcmp(path, "/") == 0) {
            send_response(client, "200 OK", "text/html; charset=utf-8",
                method[0] == 'H' ? NULL : (const char *) embedded_index_html,
                (long) embedded_index_html_len);
        } else if (strcmp(path, "/style.css") == 0) {
            send_response(client, "200 OK", "text/css; charset=utf-8",
                method[0] == 'H' ? NULL : (const char *) embedded_style_css,
                (long) embedded_style_css_len);
        } else {
            send_response(client, "404 Not Found", "text/plain; charset=utf-8", "Not Found", 9);
        }
    } else {
        struct stat st;
        if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
            close(fd);
            send_response(client, "404 Not Found", "text/plain; charset=utf-8", "Not Found", 9);
            return;
        }
        send_response(client, "200 OK", get_mime_type(full_path), NULL, st.st_size);

        if (method[0] != 'H') {
            while ((nread = read(fd, buffer, BUF_SIZE)) > 0) {
                if (write_client(client, buffer, (size_t) nread) < 0) break;
            }
        }
        close(fd);
    }
}