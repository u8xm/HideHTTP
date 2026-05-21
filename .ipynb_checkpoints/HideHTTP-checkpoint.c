#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <getopt.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define DEFAULT_PORT 9999
#define BUFFER_SIZE 8192
#define MAX_IP_ENTRIES 1000

const char *get_content_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    return "application/octet-stream";
}

#define DEFAULT_TLS_CERT "server.crt"
#define DEFAULT_TLS_KEY "server.key"

void print_help(const char *prog) {
    printf("Usage: %s [--port N] [--root PATH] [--timeout S] [--verbose] [--tls] [--gen-cert] [--tls-cert FILE] [--tls-key FILE] [--rate N] [--help]\n", prog);
    printf("  --port N      TCP port (default %d)\n", DEFAULT_PORT);
    printf("  --root PATH   Serve files from this directory (default .)\n");
    printf("  --timeout S   read timeout in seconds (default 5)\n");
    printf("  --verbose     print request logs\n");
    printf("  --tls         enable TLS (requires cert and key)\n");
    printf("  --gen-cert    generate self-signed cert/key as server.crt/server.key\n");
    printf("  --tls-cert    TLS certificate path (default server.crt)\n");
    printf("  --tls-key     TLS private key path (default server.key)\n");
    printf("  --rate N      max requests per minute per IP (default 0 off)\n");
    printf("  --help        show this help\n");
}

typedef struct {
    char ip[INET_ADDRSTRLEN];
    int count;
    time_t window_start;
} RateEntry;

int check_rate_limit(RateEntry *rates, int rate_count, const char *ip, int max_requests) {
    time_t now = time(NULL);
    for (int i = 0; i < rate_count; i++) {
        if (strcmp(rates[i].ip, ip) == 0) {
            if (now - rates[i].window_start >= 60) {
                rates[i].count = 1;
                rates[i].window_start = now;
                return 0;
            }
            if (rates[i].count >= max_requests) return -1;
            rates[i].count++;
            return 0;
        }
    }
    if (rate_count < MAX_IP_ENTRIES) {
        strncpy(rates[rate_count].ip, ip, INET_ADDRSTRLEN - 1);
        rates[rate_count].ip[INET_ADDRSTRLEN - 1] = '\0';
        rates[rate_count].count = 1;
        rates[rate_count].window_start = now;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    char root[512] = ".";
    int timeout_seconds = 5;
    int verbose = 0;
    int rate_limit = 0;
    int use_tls = 0;
    int gen_cert = 0;
    char tls_cert[512] = "";
    char tls_key[512] = "";

    static struct option long_options[] = {
        {"port", required_argument, NULL, 'p'},
        {"root", required_argument, NULL, 'r'},
        {"timeout", required_argument, NULL, 't'},
        {"verbose", no_argument, NULL, 'v'},
        {"tls", no_argument, NULL, 's'},
        {"gen-cert", no_argument, NULL, 'g'},
        {"tls-cert", required_argument, NULL, 'c'},
        {"tls-key", required_argument, NULL, 'k'},
        {"rate", required_argument, NULL, 'l'},
        {"help", no_argument, NULL, 'h'},
        {0, 0, 0, 0}
    };

    int c;
    while ((c = getopt_long(argc, argv, "p:r:t:vs:gc:k:l:h", long_options, NULL)) != -1) {
        switch (c) {
            case 'p': port = atoi(optarg); break;
            case 'r': strncpy(root, optarg, sizeof(root)-1); root[sizeof(root)-1] = '\0'; break;
            case 't': timeout_seconds = atoi(optarg); break;
            case 'v': verbose = 1; break;
            case 's': use_tls = 1; break;
            case 'g': gen_cert = 1; break;
            case 'c': strncpy(tls_cert, optarg, sizeof(tls_cert)-1); tls_cert[sizeof(tls_cert)-1] = '\0'; break;
            case 'k': strncpy(tls_key, optarg, sizeof(tls_key)-1); tls_key[sizeof(tls_key)-1] = '\0'; break;
            case 'l': rate_limit = atoi(optarg); break;
            case 'h': print_help(argv[0]); return 0;
            default: print_help(argv[0]); return 1;
        }
    }

    if (strlen(tls_cert) == 0) strncpy(tls_cert, DEFAULT_TLS_CERT, sizeof(tls_cert)-1);
    if (strlen(tls_key) == 0) strncpy(tls_key, DEFAULT_TLS_KEY, sizeof(tls_key)-1);

    if (gen_cert) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout %s -out %s -subj '/CN=localhost'", tls_key, tls_cert);
        if (system(cmd) != 0) {
            fprintf(stderr, "Failed to generate self-signed cert/key\n");
            return 1;
        }
        use_tls = 1;
    }

    if (use_tls && (strlen(tls_cert) == 0 || strlen(tls_key) == 0)) {
        fprintf(stderr, "TLS enabled but missing --tls-cert or --tls-key\n");
        return 1;
    }

    SSL_CTX *ssl_ctx = NULL;
    if (use_tls) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ssl_ctx = SSL_CTX_new(TLS_server_method());
        if (!ssl_ctx) {
            ERR_print_errors_fp(stderr);
            return 1;
        }
        if (SSL_CTX_use_certificate_file(ssl_ctx, tls_cert, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            return 1;
        }
        if (SSL_CTX_use_PrivateKey_file(ssl_ctx, tls_key, SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr);
            return 1;
        }
    }

    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        return EXIT_FAILURE;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        return EXIT_FAILURE;
    }

    printf("HideHTTP listening on port %d (root='%s')%s\n", port, root, use_tls ? " with TLS" : "");

    RateEntry rates[MAX_IP_ENTRIES] = {0};
    int rate_count = 0;

    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));

        if (rate_limit > 0 && check_rate_limit(rates, rate_count, client_ip, rate_limit) < 0) {
            const char *body = "Too Many Requests";
            dprintf(new_socket, "HTTP/1.1 429 Too Many Requests\r\nContent-Length: %zu\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n%s", strlen(body), body);
            close(new_socket);
            continue;
        }
        if (rate_count < MAX_IP_ENTRIES) {
            int found = 0;
            for (int i = 0; i < rate_count; i++) {
                if (strcmp(rates[i].ip, client_ip) == 0) { found = 1; break; }
            }
            if (!found) {
                strncpy(rates[rate_count].ip, client_ip, INET_ADDRSTRLEN - 1);
                rates[rate_count].ip[INET_ADDRSTRLEN - 1] = '\0';
                rates[rate_count].count = 1;
                rates[rate_count].window_start = time(NULL);
                rate_count++;
            }
        }

        if (timeout_seconds > 0) {
            struct timeval timeout;
            timeout.tv_sec = timeout_seconds;
            timeout.tv_usec = 0;
            setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        }

        SSL *ssl = NULL;
        if (use_tls) {
            ssl = SSL_new(ssl_ctx);
            if (!ssl) {
                close(new_socket);
                continue;
            }
            SSL_set_fd(ssl, new_socket);
            if (SSL_accept(ssl) <= 0) {
                SSL_free(ssl);
                close(new_socket);
                continue;
            }
        }

        char buffer[BUFFER_SIZE] = {0};
        int valread = use_tls ? SSL_read(ssl, buffer, sizeof(buffer)-1) : read(new_socket, buffer, sizeof(buffer)-1);
        if (valread <= 0) {
            if (use_tls) SSL_free(ssl);
            close(new_socket);
            continue;
        }

        buffer[valread] = '\0';
        char method[16], path[256], protocol[16];
        if (sscanf(buffer, "%15s %255s %15s", method, path, protocol) < 3) {
            if (use_tls) SSL_free(ssl);
            close(new_socket);
            continue;
        }

        if (verbose) printf("[%s] %s %s %s\n", client_ip, method, path, protocol);

        if (strcmp(method, "GET") != 0) {
            const char *body = "Method Not Allowed";
            char header[256];
            int n = snprintf(header, sizeof(header), "HTTP/1.1 405 Method Not Allowed\r\nContent-Length: %zu\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n", strlen(body));
            if (use_tls) {
                SSL_write(ssl, header, n);
                SSL_write(ssl, body, strlen(body));
                SSL_shutdown(ssl); SSL_free(ssl);
            } else {
                write(new_socket, header, n);
                write(new_socket, body, strlen(body));
            }
            close(new_socket);
            continue;
        }

        if (strstr(path, "..")) {
            const char *body = "Forbidden";
            char header[256];
            int n = snprintf(header, sizeof(header), "HTTP/1.1 403 Forbidden\r\nContent-Length: %zu\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n", strlen(body));
            if (use_tls) {
                SSL_write(ssl, header, n);
                SSL_write(ssl, body, strlen(body));
                SSL_shutdown(ssl); SSL_free(ssl);
            } else {
                write(new_socket, header, n);
                write(new_socket, body, strlen(body));
            }
            close(new_socket);
            continue;
        }

        char request_path[1024];
        if (strcmp(path, "/") == 0) {
            snprintf(request_path, sizeof(request_path), "%s/index.html", root);
        } else {
            snprintf(request_path, sizeof(request_path), "%s%s", root, path);
        }

        int file_fd = open(request_path, O_RDONLY);
        if (file_fd < 0) {
            const char *body = "Not Found";
            char header[256];
            int n = snprintf(header, sizeof(header), "HTTP/1.1 404 Not Found\r\nContent-Length: %zu\r\nConnection: close\r\nContent-Type: text/plain\r\n\r\n", strlen(body));
            if (use_tls) {
                SSL_write(ssl, header, n);
                SSL_write(ssl, body, strlen(body));
                SSL_shutdown(ssl); SSL_free(ssl);
            } else {
                write(new_socket, header, n);
                write(new_socket, body, strlen(body));
            }
            close(new_socket);
            continue;
        }

        struct stat st;
        fstat(file_fd, &st);
        const char *content_type = get_content_type(request_path);
        char header[256];
        int n = snprintf(header, sizeof(header), "HTTP/1.1 200 OK\r\nContent-Length: %lld\r\nContent-Type: %s\r\nConnection: close\r\n\r\n", (long long)st.st_size, content_type);
        if (use_tls) SSL_write(ssl, header, n);
        else write(new_socket, header, n);

        ssize_t bytes_read;
        char file_buffer[BUFFER_SIZE];

        while ((bytes_read = read(file_fd, file_buffer, BUFFER_SIZE)) > 0) {
            if (use_tls) SSL_write(ssl, file_buffer, bytes_read);
            else write(new_socket, file_buffer, bytes_read);
        }

        close(file_fd);
        if (use_tls) { SSL_shutdown(ssl); SSL_free(ssl); }
        close(new_socket);
    }

    close(server_fd);
    if (ssl_ctx) SSL_CTX_free(ssl_ctx);
    return 0;
}
