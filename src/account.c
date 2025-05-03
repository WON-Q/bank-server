// src/account.c

#include "account.h"
#include <stdio.h>

static Account accounts[MAX_ACCOUNTS];
static int     account_count = 0;

void account_module_init(void) {
    // 예시로 2개 계좌 초기화
    account_count = 2;
    accounts[0].id      = 1001;
    accounts[0].balance = 10000;
    pthread_mutex_init(&accounts[0].mtx, NULL);

    accounts[1].id      = 1002;
    accounts[1].balance =  5000;
    pthread_mutex_init(&accounts[1].mtx, NULL);
}

static int find_index(int id) {
    for (int i = 0; i < account_count; ++i) {
        if (accounts[i].id == id) return i;
    }
    return -1;
}

int account_deposit(int id, long amount, long *new_balance) {
    int idx = find_index(id);
    if (idx < 0) return -2;  // 계좌 없음

    Account *acc = &accounts[idx];
    pthread_mutex_lock(&acc->mtx);
    acc->balance += amount;
    *new_balance = acc->balance;
    pthread_mutex_unlock(&acc->mtx);
    return 0;
}

int account_withdraw(int id, long amount, long *new_balance) {
    int idx = find_index(id);
    if (idx < 0) return -2;  // 계좌 없음

    Account *acc = &accounts[idx];
    pthread_mutex_lock(&acc->mtx);
    if (acc->balance < amount) {
        pthread_mutex_unlock(&acc->mtx);
        return -1;  // 잔액 부족
    }
    acc->balance -= amount;
    *new_balance = acc->balance;
    pthread_mutex_unlock(&acc->mtx);
    return 0;
}

