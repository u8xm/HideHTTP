#include <stdio.h>
#include <string.h>
#include "../include/utils.h"

const char* get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    return "text/plain";
}

void print_banner(int http_port, int https_port, const char *root) {
    printf("\033[1;35m"); 
    printf("  _    _ _     _      _    _ _______ _______ _____  \n");
    printf(" | |  | (_)   | |    | |  | |__   __|__   __|  __ \\ \n");
    printf(" | |__| |_  __| | ___| |__| |  | |     | |  | |__) |\n");
    printf(" |  __  | |/ _` |/ _ \\  __  |  | |     | |  |  ___/ \n");
    printf(" | |  | | | (_| |  __/ |  | |  | |     | |  | |     \n");
    printf(" |_|  |_|_|\\__,_|\\___|_|  |_|  |_|     |_|  |_|     \n");
    printf("\033[0m");
    
    printf("\n\033[1;33m[ Specs ]\033[0m\n");
    printf("------------------------------------------\n");
    printf(" STATUS     : \033[1;32mRunning\033[0m\n");
    printf(" HTTP PORT  : %d\n", http_port);
    if (https_port > 0) 
        printf(" HTTPS PORT : %d \033[1;32m(SSL ENABLED)\033[0m\n", https_port);
    else 
        printf(" HTTPS PORT : \033[1;31mDISABLED\033[0m\n");
    printf(" ROOT DIR   : %s\n", root);
    printf("------------------------------------------\n");
}