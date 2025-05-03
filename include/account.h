// include/account.h

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <pthread.h>

#define MAX_ACCOUNTS 1024

typedef struct {
    int             id;        // 계좌 번호
    long            balance;   // 잔액
    pthread_mutex_t mtx;       // 동시성 제어용 뮤텍스
} Account;

// 모듈 초기화: 뮤텍스 초기화 및 예시 계좌 세팅
void account_module_init(void);

// 입금 처리: 성공 0, 계좌 없음 -2
int account_deposit(int id, long amount, long *new_balance);

// 출금 처리: 성공 0, 잔액 부족 -1, 계좌 없음 -2
int account_withdraw(int id, long amount, long *new_balance);

#endif // ACCOUNT_H

