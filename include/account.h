// include/account.h

#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <pthread.h>

#define MAX_ACCOUNTS 1024

// In-Memory 기반 Account 모델
typedef struct {
    int             id;        // 계좌 식별 ID
    long            balance;   // 잔액
    pthread_mutex_t mtx;       // 동시성 제어용 뮤텍스
} Account;

// 모듈 초기화: 뮤텍스 초기화 및 예시 계좌 세팅
void account_module_init(void);

// 입금 처리 (account_number 기준)
int account_deposit(const char *acct_num, long amount, long *new_balance);

// 출금 처리 (account_number 기준)
int account_withdraw(const char *acct_num, long amount, long *new_balance);

// 잔액 조회 (account_number 기준)
int account_get_balance(const char *acct_num, long *balance);

#endif // ACCOUNT_H

