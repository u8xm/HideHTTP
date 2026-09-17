#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>
#include <sys/select.h>
#include "../include/network.h"
#include "../include/ssl_module.h"
#include "../include/http.h"
#include "../include/utils.h"

static void print_help(const char *program) {
    printf("Usage: %s [options]\n\n", program);
    printf("  -p, --port PORT       HTTP port (default: 8080)\n");
    printf("  -r, --root PATH       File root (default: ./assets/web)\n");
    printf("  -s, --https-port PORT HTTPS port (default: 8443)\n");
    printf("      --tls             Enable HTTPS startup\n");
    printf("  -c, --cert FILE       TLS certificate (default: server.crt)\n");
    printf("  -k, --key FILE        TLS private key (default: server.key)\n");
    printf("  -h, --help            Show this help\n");
}

static int parse_port(const char *value) {
    char *end = NULL;
    long port = strtol(value, &end, 10);
    if (*value == '\0' || *end != '\0' || port < 1 || port > 65535) return -1;
    return (int) port;
}

static void resolve_default_path(char *path, size_t path_size, const char *project_path) {
    char executable[PATH_MAX];
    ssize_t length;
    char *directory;
    size_t executable_length;
    size_t project_length;

    if (access(path, F_OK) == 0) return;

    length = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
    if (length <= 0 || (size_t) length >= sizeof(executable)) return;

    executable[length] = '\0';
    directory = strrchr(executable, '/');
    if (!directory) return;

    *directory = '\0';
    executable_length = strlen(executable);
    project_length = strlen(project_path);
    if (executable_length + project_length + 4 >= path_size) return;

    memcpy(path, executable, executable_length);
    memcpy(path + executable_length, "/../", 4);
    memcpy(path + executable_length + 4, project_path, project_length + 1);
}

int main(int argc, char **argv) {
    int http_port = 8080;
    int https_port = 8443;
    int enable_tls = 0;
    int custom_root = 0;
    int custom_certificate = 0;
    int custom_private_key = 0;
    char root[512] = "./assets/web";
    char certificate[512] = "server.crt";
    char private_key[512] = "server.key";

    static const struct option options[] = {
        {"port", required_argument, NULL, 'p'},
        {"root", required_argument, NULL, 'r'},
        {"https-port", required_argument, NULL, 's'},
        {"tls", no_argument, NULL, 't'},
        {"cert", required_argument, NULL, 'c'},
        {"key", required_argument, NULL, 'k'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    int option;
    while ((option = getopt_long(argc, argv, "p:r:s:tc:k:h", options, NULL)) != -1) {
        switch (option) {
            case 'p': http_port = parse_port(optarg); break;
            case 'r': snprintf(root, sizeof(root), "%s", optarg); custom_root = 1; break;
            case 's': https_port = parse_port(optarg); break;
            case 't': enable_tls = 1; break;
            case 'c': snprintf(certificate, sizeof(certificate), "%s", optarg); custom_certificate = 1; break;
            case 'k': snprintf(private_key, sizeof(private_key), "%s", optarg); custom_private_key = 1; break;
            case 'h': print_help(argv[0]); return 0;
            default: print_help(argv[0]); return 1;
        }
        if ((option == 'p' && http_port < 0) || (option == 's' && https_port < 0)) {
            fprintf(stderr, "Invalid port. Use a value between 1 and 65535.\n");
            return 1;
        }
    }

    if (!custom_root) resolve_default_path(root, sizeof(root), "assets/web");
    if (!custom_certificate) resolve_default_path(certificate, sizeof(certificate), "server.crt");
    if (!custom_private_key) resolve_default_path(private_key, sizeof(private_key), "server.key");

    SSL_CTX *ctx = NULL;
    if (enable_tls && access(certificate, R_OK) == 0 && access(private_key, R_OK) == 0) {
        ctx = init_ssl_context(certificate, private_key);
    }

    int http_fd = start_server(http_port);
    if (http_fd < 0) {
        perror("Unable to start HTTP server");
        cleanup_ssl(ctx);
        return 1;
    }
    int https_fd = ctx ? start_server(https_port) : -1;

    print_banner(root);

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

    close(http_fd);
    if (https_fd >= 0) close(https_fd);
    cleanup_ssl(ctx);
    return 0;
}