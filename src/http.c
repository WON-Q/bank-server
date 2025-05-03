// src/http.c
#include "http.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>  // strlen

void handle_connection(int client_fd) {
    // 간단한 OK 응답
    const char *body = "OK";
    char header[128];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %zu\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n",
        strlen(body)
    );
    write(client_fd, header, header_len);
    write(client_fd, body, strlen(body));

    close(client_fd);
}

