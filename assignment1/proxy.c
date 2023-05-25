#include <sys/socket.h> 
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

#define BUFFER_SIZE 4096

void handle_request(int client_sock);

int main(int argc, char *argv[]) {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    pthread_t tid;

    // Create server socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }
    printf("Server socket created\n");
    printf("server_sock = %d\n\n", server_sock);


    // Bind socket to port, make sure port is > 8000
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() error");
        exit(EXIT_FAILURE);
    }
    printf("Server socket bound\n");

    // Listen for incoming connections
    if (listen(server_sock, 10) == -1) {
        perror("listen() error");
        exit(EXIT_FAILURE);
    }

    printf("Proxy server listening...\n");

    // Accept incoming connections and handle requests in separate threads
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
    
        if (client_sock == -1) {
            perror("accept() error");
            continue;
        }
        printf("client: %d\n", client_sock);

        handle_request(client_sock);

        
        // if (pthread_create(&tid, NULL, (void *)&handle_request, &client_sock) != 0) {
        //     perror("pthread_create() error");
        //     close(client_sock);
        //     continue;
        // }

    }

   
    close(server_sock);
    return 0;
}

void handle_request(int client_sock) {

    printf("client_sock: %d\n", client_sock);

    char buffer[BUFFER_SIZE];
    int server_sock, bytes_received, bytes_sent, content_length = -1;
    int port = 80;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Read request from client
    bytes_received = recv(client_sock, &buffer, BUFFER_SIZE, 0);
    printf("bytes_received: %d\n", bytes_received);
    if (bytes_received == -1) {
        perror("recv() error");
        close(client_sock);
        return;
    }

    printf("buffer: %s\n", buffer);

    // Parse GET request to get needed information
    char *method = strtok(buffer, " \t\r\n");
    char *url = strtok(NULL, " \t\r\n");
    char *http_version = strtok(NULL, " \t\r\n");


    if (method == NULL || url == NULL || http_version == NULL) {
        fprintf(stderr, "Invalid request: %s\n", buffer);
        close(client_sock);
        return;
    }
    if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    }
    
    char *hostname = strtok(url, "/");
    char *path = strtok(NULL, "\r\n");
    if (path == NULL) {
        path = "/";
    }

    printf("method: %s\n", method);
    printf("url: %s\n", url);
    printf("http_version: %s\n\n", http_version);
    printf("hostname: %s\n", hostname);
    printf("path: %s\n\n", path);
    printf("port: %d\n\n", port);    

    // Connect to server
   server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket() error");
        close(client_sock);
        return;
    }
    printf("2nd socket created\n");

    struct hostent *he = gethostbyname(hostname);
    if (he == NULL) {
        perror("gethostbyname() error");
        close(client_sock);
        close(server_sock);
        return;
    }
    printf("host name received\n");

    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() error");
        close(client_sock);
        close(server_sock);
        return;
    }
    printf("socket connected\n");

    char request[BUFFER_SIZE];
    sprintf(request, "%s /%s %s\r\nHost: %s\r\n\r\n", method, path, http_version, hostname);

    printf("request: %s\n", request);

    // sending data to the server 
    bytes_sent = send(server_sock, request, strlen(request), 0);
    if (bytes_sent == -1) {
        perror("send() error");
        close(client_sock);
        close(server_sock);
        return;
    }
    printf("bytes sent: %d\n", bytes_sent);

    // Receive response from server
    // What the server receives, you return to the browser
    while ((bytes_received = recv(server_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        bytes_sent = send(client_sock, buffer, bytes_received, 0);
        if (bytes_sent == -1) {
            perror("send() error");
            close(server_sock);
            close(client_sock);
            return;
        }
    }

    if (bytes_received == -1) {
        perror("recv() error");
        close(server_sock);
        close(client_sock);
        return;
    }
      printf("bytes received: %d\n", bytes_received);


    // Close sockets
    close(server_sock);
    close(client_sock);
    printf("closing sockets \n");

}

