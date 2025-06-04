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
    if (!db_init(
            "34.22.64.79",   // MySQL 호스트
            "root",        // 사용자
            "12345",  // 비밀번호
            "bank",        // DB명
            3306)) {
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

// 작업 큐(백로그 큐)에 client_fd 등록 (accept() 시스템 콜 호출 직후)
// -> 성공: 0, 작업 큐(백로그 큐)가 다 찼을 시: -1
int threadpool_add_task(int client_fd) {
    pthread_mutex_lock(&queue_mtx);
    // 다음 위치가 head랑 같으면 큐 풀이므로 에러
    int next = (tail + 1) % QUEUE_SIZE;
    if (next == head) {
        pthread_mutex_unlock(&queue_mtx);
        return -1;
    }
    queue[tail] = (Task){ .client_fd = client_fd };
    tail = next;
    pthread_cond_signal(&queue_cond);
    pthread_mutex_unlock(&queue_mtx);
    return 0;
}


void threadpool_destroy(void) {
    // 필요 시 스레드 종료 로직 추가
}

