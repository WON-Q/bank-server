// src/account.c

#include "account.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>

// 프로그램 기동 시 한 번만 호출됩니다.
// 내부에서 db_init()으로 MySQL 커넥션을 맺습니다.
void account_module_init(void) {
    if (!db_init(
            "127.0.0.1",   // MySQL 호스트
            "root",   // MySQL 사용자
            "zxcasdqwe5",     // MySQL 비밀번호
            "bank",        // 데이터베이스명
            3306           // 포트
        )) {
        fprintf(stderr, "[FATAL] DB 연결 실패, 프로그램을 종료합니다\n");
        exit(EXIT_FAILURE);
    }
}

// HTTP handler에서 호출: DB에 트랜잭션 단위로 입금 처리
int account_deposit(int id, long amount, long *new_balance) {
    return db_deposit(id, amount, new_balance);
}

// HTTP handler에서 호출: DB에 트랜잭션 단위로 출금 처리
int account_withdraw(int id, long amount, long *new_balance) {
    return db_withdraw(id, amount, new_balance);
}

