// include/threadpool.h
#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>

// 워커 스레드 수와 큐 크기 (필요에 따라 조정)
#define POOL_SIZE 4
#define QUEUE_SIZE 128

// 작업(Task) 구조체: client_fd만 저장
typedef struct {
    int client_fd;
} Task;

// 스레드풀 초기화 (worker 스레드 생성, 큐 뮤텍스/condvar 초기화)
void threadpool_init(void);

// 작업 큐에 client_fd 등록 (accept() 직후 호출)
void threadpool_add_task(int client_fd);

// 스레드풀 정리 (프로세스 종료 시 필요하면 호출)
void threadpool_destroy(void);

#endif // THREADPOOL_H

