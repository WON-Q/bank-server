// src/threadpool.c
#include "threadpool.h"
#include "http.h"            // handle_connection() 선언
#include "db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mysql/mysql.h>     // mysql_thread_init, mysql_thread_end 선언

// 작업 큐와 포인터
static Task      queue[QUEUE_SIZE];
static int       head = 0, tail = 0;
static pthread_mutex_t queue_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  queue_cond = PTHREAD_COND_INITIALIZER;

// 워커 스레드 배열
static pthread_t workers[POOL_SIZE];

static void *worker_loop(void *arg) {
    (void)arg;

    // — 워커 스레드 전용 MySQL 초기화 —
    mysql_thread_init();
    
    // 환경 변수에서 DB 설정 읽기
    const char *host = getenv("DB_HOST");
    const char *user = getenv("DB_USER");
    const char *password = getenv("DB_PASSWORD");
    const char *database = getenv("DB_NAME");
    const char *port_str = getenv("DB_PORT");
    
    // 기본값 설정
    if (!host) host = "127.0.0.1";
    if (!user) user = "root";
    if (!password) password = "12345";
    if (!database) database = "bank";
    int port = port_str ? atoi(port_str) : 3306;
    
    if (!db_init(host, user, password, database, port)) {
        fprintf(stderr, "[FATAL] worker DB init failed\n");
        mysql_thread_end();
        return NULL;
    }

    while (1) {
        pthread_mutex_lock(&queue_mtx);
        while (head == tail) {
            // 큐가 비어 있으면 대기
            pthread_cond_wait(&queue_cond, &queue_mtx);
        }
        Task t = queue[head];
        head = (head + 1) % QUEUE_SIZE;
        pthread_mutex_unlock(&queue_mtx);

        // 실제 HTTP 처리 함수 호출
        handle_connection(t.client_fd);
    }

    db_close();
    mysql_thread_end();
    return NULL;
}

void threadpool_init(void) {
    for (int i = 0; i < POOL_SIZE; ++i) {
        pthread_create(&workers[i], NULL, worker_loop, NULL);
        pthread_detach(workers[i]);
    }
}

void threadpool_add_task(int client_fd) {
    pthread_mutex_lock(&queue_mtx);
    queue[tail] = (Task){ .client_fd = client_fd };
    tail = (tail + 1) % QUEUE_SIZE;
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mtx);
}

void threadpool_destroy(void) {
    // 필요 시 스레드 종료 로직 추가
}

