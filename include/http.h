#ifndef HTTP_H
#define HTTP_H

#include "network.h"

void handle_request(ClientConn client, const char *root_dir);

void send_response(ClientConn client, const char *status, const char *type, const char *body, long len);

#endif