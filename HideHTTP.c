#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#define PORT 9999
#define BUFFER_SIZE 8192

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen");
        exit(EXIT_FAILURE);
    }

    printf("HideHTTP WebServer listening on port %d...\n", PORT);

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) continue;

        struct timeval timeout;      
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

        while(1) {
            char buffer[BUFFER_SIZE] = {0};
            int valread = read(new_socket, buffer, BUFFER_SIZE);
            
            if (valread <= 0) break;

            char method[16], path[256], protocol[16];
            if (sscanf(buffer, "%s %s %s", method, path, protocol) < 3) break;

            char *filename = (strcmp(path, "/") == 0) ? "index.html" : path + 1;

            int file_fd = open(filename, O_RDONLY);
            if (file_fd < 0) {
                char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\nConnection: keep-alive\r\n\r\nNot Found";
                write(new_socket, not_found, strlen(not_found));
            } else {
                struct stat st;
                fstat(file_fd, &st);
                
                char response_header[512];
                sprintf(response_header, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %lld\r\nConnection: keep-alive\r\n\r\n", (long long)st.st_size);
                write(new_socket, response_header, strlen(response_header));

                int bytes_read;
                char file_buffer[BUFFER_SIZE];
                while ((bytes_read = read(file_fd, file_buffer, BUFFER_SIZE)) > 0) {
                    write(new_socket, file_buffer, bytes_read);
                }
                close(file_fd);
            }

            if (strstr(buffer, "Connection: close")) break;
        }
        close(new_socket);
    }
    return 0;
}
