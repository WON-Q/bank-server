// src/http.c

#include "http.h"
#include "json_util.h"
#include "account.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#define LOG_ERR(fmt, ...) \
    fprintf(stderr, "[ERROR] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

#define BUFFER_SIZE 8192


void handle_connection(int client_fd) {
    char buffer[BUFFER_SIZE];
    // 1) 요청 읽기
    int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) {
	LOG_ERR("handle_connection: read failed or connection closed (fd=%d)", client_fd);
        close(client_fd);
        return;
    }
    buffer[bytes_read] = '\0';

    // 2) 요청 라인 파싱 (예: "POST /deposit HTTP/1.1")
    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        LOG_ERR("handle_connection: invalid request line");
	close(client_fd);
        return;
    }
    printf("Received request: %s %s\n", method, path);

    // 3) 헤더 끝 다음부터 JSON 바디 시작
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) body += 4;
    else body = "";

    // 4) 라우팅 및 비즈니스 로직 호출
    int status_code = 200;
    char resp_body[256];
    size_t resp_len = 0;

    if (strcmp(path, "/deposit") == 0 && strcmp(method, "POST") == 0) {
        int acct; long amt, new_bal;
        int rc = parse_request(body, &acct, &amt);
        if (rc == 0) {
            int dep = account_deposit(acct, amt, &new_bal);
            if (dep == 0) {
                resp_len = snprintf(resp_body, sizeof(resp_body),
                    "{\"status\":\"SUCCESS\",\"balance\":%ld}", new_bal);
            } else {
                status_code = 404;
                resp_len = snprintf(resp_body, sizeof(resp_body),
                    "{\"status\":\"FAIL\",\"reason\":\"Account not found\"}");
            }
        } else {
            status_code = 400;
            resp_len = snprintf(resp_body, sizeof(resp_body),
                "{\"status\":\"FAIL\",\"reason\":\"Invalid JSON\"}");
        }
    }
    else if (strcmp(path, "/withdraw") == 0 && strcmp(method, "POST") == 0) {
        int acct; long amt, new_bal;
        int rc = parse_request(body, &acct, &amt);
        if (rc == 0) {
            int wd = account_withdraw(acct, amt, &new_bal);
            if (wd == 0) {
                resp_len = snprintf(resp_body, sizeof(resp_body),
                    "{\"status\":\"SUCCESS\",\"balance\":%ld}", new_bal);
            } else if (wd == -1) {
                status_code = 409;
                resp_len = snprintf(resp_body, sizeof(resp_body),
                    "{\"status\":\"FAIL\",\"reason\":\"Insufficient funds\"}");
            } else {
                status_code = 404;
                resp_len = snprintf(resp_body, sizeof(resp_body),
                    "{\"status\":\"FAIL\",\"reason\":\"Account not found\"}");
            }
        } else {
            status_code = 400;
            resp_len = snprintf(resp_body, sizeof(resp_body),
                "{\"status\":\"FAIL\",\"reason\":\"Invalid JSON\"}");
        }
    }
    else {
        status_code = 404;
        resp_len = snprintf(resp_body, sizeof(resp_body),
            "{\"status\":\"FAIL\",\"reason\":\"Not Found\"}");
    }

    // 5) 응답 헤더 전송
    char header[256];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n",
        status_code,
        (status_code == 200 ? "OK" :
         status_code == 400 ? "Bad Request" :
         status_code == 404 ? "Not Found" :
         status_code == 409 ? "Conflict" : "Error"),
        resp_len
    );
    write(client_fd, header, header_len);
    write(client_fd, resp_body, resp_len);

    // 6) 연결 종료
    close(client_fd);
}

