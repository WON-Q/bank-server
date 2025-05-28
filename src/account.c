// src/account.c

#include "account.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>

// 프로그램 기동 시 한 번만 호출됩니다.
// 내부에서 db_init()으로 MySQL 커넥션을 맺습니다.
void account_module_init(void) {
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
    
    printf("Connecting to DB: %s@%s:%d/%s\n", user, host, port, database);
    
    if (!db_init(host, user, password, database, port)) {
        fprintf(stderr, "[FATAL] DB 연결 실패, 프로그램을 종료합니다\n");
        exit(EXIT_FAILURE);
    }
}

// HTTP handler에서 호출: DB에 트랜잭션 단위로 입금 처리
int account_deposit(const char *acct_num, long amount, long *new_balance) {
    return db_deposit_by_number(acct_num, amount, new_balance);
}

// HTTP handler에서 호출: DB에 트랜잭션 단위로 출금 처리
int account_withdraw(const char *acct_num, long amount, long *new_balance) {
    return db_withdraw_by_number(acct_num, amount, new_balance);
}

// HTTP handler에서 호출: 계좌 잔액조회(계좌번호 + 잔액)
int account_get_balance(const char *acct_num, long *balance) {
    return db_get_balance(acct_num, balance);
}
