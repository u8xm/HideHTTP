#include <openssl/ssl.h>
#include <openssl/err.h>
#include "../include/ssl_module.h"

SSL_CTX* init_ssl_context(const char *cert_file, const char *key_file) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    const SSL_METHOD *method = TLS_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);

    if (!ctx) {
        perror("Impossible de créer le contexte SSL");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return NULL;
    }

    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr, "La clé privée ne correspond pas au certificat public\n");
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

void cleanup_ssl(SSL_CTX *ctx) {
    if (ctx) {
        SSL_CTX_free(ctx);
    }
    EVP_cleanup();
}