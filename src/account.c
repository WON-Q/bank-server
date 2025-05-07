// src/account.c

#include "account.h"
#include <stdio.h>
#define LOG_ERR(fmt, ...) \
    fprintf(stderr, "[ERROR] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

static Account accounts[MAX_ACCOUNTS];
static int     account_count = 0;

void account_module_init(void) {
    // 테스트용 임시 계좌 2개 초기화
    account_count = 2;
    accounts[0].id      = 1001;
    accounts[0].balance = 10000000;
    pthread_mutex_init(&accounts[0].mtx, NULL);

    accounts[1].id      = 1002;
    accounts[1].balance =  50000000;
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
    if (idx < 0){
	LOG_ERR("account_deposit: account %d not found", id); // 계좌 없음
    	return -2;
    }

    Account *acc = &accounts[idx];
    pthread_mutex_lock(&acc->mtx);
    acc->balance += amount;
    *new_balance = acc->balance;
    pthread_mutex_unlock(&acc->mtx);
    return 0;
}

int account_withdraw(int id, long amount, long *new_balance) {
    int idx = find_index(id);
    if (idx < 0){
	LOG_ERR("account_withdraw: account %d not found", id); // 계좌 없음
	return -2;
    }

    Account *acc = &accounts[idx];
    pthread_mutex_lock(&acc->mtx);
    if (acc->balance < amount) {
	LOG_ERR("account_withdraw: insufficient funds for account %d (balance=%ld, req=%ld)", id, acc->balance, amount);
        pthread_mutex_unlock(&acc->mtx);
        return -1;  // 잔액 부족
    }
    acc->balance -= amount;
    *new_balance = acc->balance;
    pthread_mutex_unlock(&acc->mtx);
    return 0;
}

