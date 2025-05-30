// src/http.c

#include "http.h"
#include "json_util.h"
#include "account.h"

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define LOG_ERR(fmt, ...) \
    fprintf(stderr, "[ERROR] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

#define BUFFER_SIZE 8192

void handle_connection(int client_fd) {
    char buffer[BUFFER_SIZE];
    int total_read = 0;
    int header_end = -1;

    // 1) 헤더 끝까지 읽기 (\r\n\r\n)
    while (total_read < BUFFER_SIZE - 1) {
        int n = read(client_fd, buffer + total_read, BUFFER_SIZE - 1 - total_read);
        if (n <= 0) {
            LOG_ERR("handle_connection: read failed or connection closed (fd=%d)", client_fd);
            close(client_fd);
            return;
        }
        total_read += n;
        buffer[total_read] = '\0';
        char *p = strstr(buffer, "\r\n\r\n");
        if (p) {
            header_end = (int)(p - buffer) + 4;
            break;
        }
    }
    if (header_end < 0) {
        LOG_ERR("handle_connection: header not complete");
        close(client_fd);
        return;
    }

    // 2) Content-Length 파싱
    int content_length = 0;
    char *cl = strstr(buffer, "Content-Length:");
    if (cl) {
        cl += strlen("Content-Length:");
        while (*cl == ' ') cl++;
        content_length = atoi(cl);
    }

    // 3) 바디 끝까지 읽기
    int body_bytes = total_read - header_end;
    while (body_bytes < content_length && total_read < BUFFER_SIZE - 1) {
        int n = read(client_fd, buffer + total_read, BUFFER_SIZE - 1 - total_read);
        if (n <= 0) break;
        total_read += n;
        body_bytes = total_read - header_end;
    }
    buffer[total_read] = '\0';
    char *body = buffer + header_end;

    // 4) 요청 라인 파싱
    char method[16], path[256];
    if (sscanf(buffer, "%15s %255s", method, path) != 2) {
        LOG_ERR("handle_connection: invalid request line");
        close(client_fd);
        return;
    }
    printf("Received request: %s %s\n", method, path);

    // 5) 라우팅 및 비즈니스 로직
    int status_code = 200;
    char resp_body[256];
    size_t resp_len = 0;
    
    if (strcmp(path, "/deposit") == 0 && strcmp(method, "POST") == 0) {
        // 이체(입금) 요청: sender_account, receiver_account, amount
        char sender[ MAX_ACCT_LEN ], receiver[ MAX_ACCT_LEN ];
        long amount, sbal, rbal;
        int rc = parse_transfer_request(
                   body,
                   sender,   sizeof(sender),
                   receiver, sizeof(receiver),
                   &amount);
        if (rc == 0) {
            int tr = account_transfer(sender, receiver, amount, &sbal, &rbal);
            switch (tr) {
            case 0:
                resp_len = snprintf(resp_body, sizeof(resp_body),
                  "{\"status\":\"SUCCESS\",\"sender_balance\":%ld,\"receiver_balance\":%ld}",
                  sbal, rbal);
                break;
            case -2:
                status_code = 404;
                resp_len = snprintf(resp_body, sizeof(resp_body),
                  "{\"status\":\"FAIL\",\"reason\":\"Account not found\"}");
                break;
            case -3:
                status_code = 409;
                resp_len = snprintf(resp_body, sizeof(resp_body),
                  "{\"status\":\"FAIL\",\"reason\":\"Insufficient funds\"}");
                break;
            default:
                status_code = 500;
                resp_len = snprintf(resp_body, sizeof(resp_body),
                  "{\"status\":\"FAIL\",\"reason\":\"Internal error\"}");
            }
        } else {
            LOG_ERR("parse_transfer_request error code=%d", rc);
            status_code = 400;
            resp_len = snprintf(resp_body, sizeof(resp_body),
              "{\"status\":\"FAIL\",\"reason\":\"Invalid JSON\"}");
        }

    }
    // 출금 요청
    else if (strcmp(path, "/withdraw") == 0 && strcmp(method, "POST") == 0) {
        char acct_num[32]; long amt, new_bal;
        int rc = parse_tx_request(body, acct_num, sizeof(acct_num), &amt);
        if (rc == 0) {
            int wd = account_withdraw(acct_num, amt, &new_bal);
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
	    LOG_ERR("parse_tx_request error (code=%d), body=%.200s", rc, body);
            status_code = 400;
            resp_len = snprintf(resp_body, sizeof(resp_body),
                "{\"status\":\"FAIL\",\"reason\":\"Invalid JSON\"}");
        }
    }
    else if (strcmp(path, "/balance") == 0 && strcmp(method, "POST") == 0) {
        char acct_num[32]; long bal;
        int rc = parse_balance_request(body, acct_num, sizeof(acct_num));
        if (rc == 0) {
            int gb = account_get_balance(acct_num, &bal);
            if (gb == 0) {
                resp_len = snprintf(resp_body, sizeof(resp_body),
                    "{\"status\":\"SUCCESS\",\"balance\":%ld}", bal);
            } else {
                status_code = 404;
                resp_len = snprintf(resp_body, sizeof(resp_body),
                    "{\"status\":\"FAIL\",\"reason\":\"Account not found\"}");
            }
        } else {
            LOG_ERR("parse_tx_request error (code=%d), body=%.200s", rc, body);
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

    // 6) 응답 전송
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
    (void)write(client_fd, header, header_len);
    (void)write(client_fd, resp_body, resp_len);

    // 7) 연결 종료
    close(client_fd);
}

