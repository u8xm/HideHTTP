#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 8192

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

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

    printf("HideHTTP v0.1 listening on port %d...\n", PORT);

    while(1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        
        read(new_socket, buffer, BUFFER_SIZE);
        
        int file_fd = open("index.html", O_RDONLY);
        if (file_fd < 0) {
            char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
            write(new_socket, not_found, strlen(not_found));
        } else {
            char header[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            write(new_socket, header, strlen(header));

            int bytes_read;
            while ((bytes_read = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
                write(new_socket, buffer, bytes_read);
            }
            close(file_fd);
        }

        close(new_socket);
    }

    return 0;
}
