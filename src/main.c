// src/main.c

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <mysql/mysql.h>

#include "account.h"
#include "db.h"           
#include "threadpool.h"

static int listen_fd;

// Ctrl+C(SIGINT) 시 안전하게 종료: 소켓 닫고 DB 커넥션 해제
void handle_sigint(int signo) {
    (void)signo;
    printf("\nShutting down...\n");
    if (listen_fd > 0) close(listen_fd);
    db_close();
    exit(EXIT_SUCCESS);
}

int main(void) {
    // SIGINT 핸들러 등록
    signal(SIGINT, handle_sigint);

    // 환경 변수에서 포트 번호 읽기
    const char *port_str = getenv("SERVER_PORT");
    int port = port_str ? atoi(port_str) : 9090;

    // MYSQL 클라이언트 라이브러리 전역 초기화
    mysql_library_init(0, NULL, NULL);

    // 계좌 모듈 초기화 (MySQL 커넥션 연결)
    account_module_init();

    // 스레드풀 초기화
    threadpool_init();

    // 서버 TCP 소켓 생성 · 바인드 · 리스닝
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port)
    };
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listen_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("Bank server listening on port %d...\n", port);

    // 5) 연결 수락 후 스레드풀에 작업 위임
    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        threadpool_add_task(client_fd);
    }

    // 종료 전 DB 연결 해제(실제로 도달하진 않지만 혹시 모를 강제 종료 상황에 대비)
    db_close();
    return 0;
}

