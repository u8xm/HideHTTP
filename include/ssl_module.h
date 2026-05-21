#ifndef SSL_MODULE_H
#define SSL_MODULE_H

#include <openssl/ssl.h>

SSL_CTX* init_ssl_context(const char *cert_file, const char *key_file);

void cleanup_ssl(SSL_CTX *ctx);

#endif