// src/http.c

#include "http.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 4096

void handle_connection(int client_fd) {
    char buffer[BUFFER_SIZE];
    // 1) 요청 읽기
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    buffer[bytes_read] = '\0';

    // 2) 요청 라인 파싱 (예: "GET /path HTTP/1.1")
    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        // 잘못된 요청인 경우 그냥 닫기
        close(client_fd);
        return;
    }
    printf("Received request: %s %s\n", method, path);

    // 3) 최소 응답 작성
    const char *body = "HTTP/1.1 200 OK";
    char header[256];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        strlen(body)
    );
    write(client_fd, header, header_len);
    write(client_fd, body, strlen(body));

    // 4) 연결 종료
    close(client_fd);
}

