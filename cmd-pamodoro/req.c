#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 4096
#define PORT 80

void make_request(const char *host, const char *path) {
    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Error: No such host\n");
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creating socket");
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        close(sock);
        return;
    }

    char request[1024];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, host);

    if (send(sock, request, strlen(request), 0) < 0) {
        perror("Error sending request");
        close(sock);
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        // Print the raw response
        printf("Response:\n%s\n", buffer);

        // Check for a redirect
        if (strstr(buffer, "HTTP/1.1 301") || strstr(buffer, "HTTP/1.1 302")) {
            char *location = strstr(buffer, "Location: ");
            if (location) {
                location += 10; // Move past "Location: "
                char *end = strstr(location, "\r\n");
                if (end) {
                    *end = '\0';
                    printf("Redirected to: %s\n", location);

                    // Extract the new host and path
                    char new_host[256];
                    char new_path[512] = "/";
                    sscanf(location, "http://%255[^/]/%511[^\n]", new_host, new_path);

                    // Recursive call to handle the redirect
                    printf("Following redirect to %s%s\n", new_host, new_path);
                    make_request(new_host, new_path);
                }
            }
        } else {
            printf("No redirect. Response body:\n%s\n", strstr(buffer, "\r\n\r\n") + 4);
        }
    } else {
        perror("Error receiving response");
    }

    close(sock);
}

int main() {
    const char *host = "fakerapi.it";
    const char *path = "/api/v2/addresses?_quantity=1";

    make_request(host, path);
    return 0;
}
