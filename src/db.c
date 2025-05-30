// src/db.c

#include "db.h"
#include <string.h>
#include <mysql/mysql.h>
#include <stdio.h>

static __thread MYSQL *conn = NULL;

// 1) 커넥션 초기화
bool db_init(const char *host, const char *user,
             const char *pw, const char *db,
             unsigned int port) {
    conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "[ERROR] mysql_init failed\n");
        return false;
    }
    if (!mysql_real_connect(conn, host, user, pw, db, port, NULL, 0)) {
        fprintf(stderr, "[ERROR] mysql_real_connect: %s\n",
                mysql_error(conn));
        return false;
    }
    return true;
}

// 2) 입금 처리
int db_transfer_by_number(const char *sender,
                          const char *receiver,
                          long amount,
                          long *new_sbal,
                          long *new_rbal) {
    MYSQL_STMT *stmt;
    MYSQL_BIND  param[1], result[1];
    long sbal, rbal;

    // 1) 트랜잭션 시작
    if (mysql_query(conn, "START TRANSACTION")) {
        mysql_query(conn, "ROLLBACK");
        return -1;
    }

    // 2) sender 잔액 조회 FOR UPDATE
    const char *sql_sel = 
      "SELECT balance FROM accounts WHERE account_number=? FOR UPDATE";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_sel, (unsigned long)strlen(sql_sel));

    // 파라미터 바인딩 (sender)
    memset(param, 0, sizeof(param));
    param[0].buffer_type   = MYSQL_TYPE_STRING;
    param[0].buffer        = (char*)sender;
    param[0].buffer_length = (unsigned long)strlen(sender);
    mysql_stmt_bind_param(stmt, param);

    // 결과 바인딩 (sbal)
    memset(result, 0, sizeof(result));
    result[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    result[0].buffer        = &sbal;
    result[0].buffer_length = sizeof(sbal);
    mysql_stmt_bind_result(stmt, result);

    if (mysql_stmt_execute(stmt)
     || mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        mysql_query(conn, "ROLLBACK");
        return -2;  // sender 계좌 없음
    }
    mysql_stmt_close(stmt);

    // 3) 잔액 부족 체크
    if (sbal < amount) {
        mysql_query(conn, "ROLLBACK");
        return -3;  // 잔액 부족
    }

    // 4) receiver 잔액 조회 FOR UPDATE
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_sel, (unsigned long)strlen(sql_sel));
    // bind receiver param
    memset(param, 0, sizeof(param));
    param[0].buffer_type   = MYSQL_TYPE_STRING;
    param[0].buffer        = (char*)receiver;
    param[0].buffer_length = (unsigned long)strlen(receiver);
    mysql_stmt_bind_param(stmt, param);
    // bind result rbal
    memset(result, 0, sizeof(result));
    result[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    result[0].buffer        = &rbal;
    result[0].buffer_length = sizeof(rbal);
    mysql_stmt_bind_result(stmt, result);

    if (mysql_stmt_execute(stmt)
     || mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        mysql_query(conn, "ROLLBACK");
        return -2;  // receiver 계좌 없음
    }
    mysql_stmt_close(stmt);

    // 5) sender 업데이트
    sbal -= amount;
    const char *sql_upd = 
      "UPDATE accounts SET balance=? WHERE account_number=?";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_upd, (unsigned long)strlen(sql_upd));
    // bind 새 sbal, sender
    MYSQL_BIND upd[2];
    memset(upd, 0, sizeof(upd));
    upd[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    upd[0].buffer        = &sbal;
    upd[0].buffer_length = sizeof(sbal);
    upd[1].buffer_type   = MYSQL_TYPE_STRING;
    upd[1].buffer        = (char*)sender;
    upd[1].buffer_length = (unsigned long)strlen(sender);
    mysql_stmt_bind_param(stmt, upd);
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // 6) receiver 업데이트
    rbal += amount;
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_upd, (unsigned long)strlen(sql_upd));
    memset(upd, 0, sizeof(upd));
    upd[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    upd[0].buffer        = &rbal;
    upd[0].buffer_length = sizeof(rbal);
    upd[1].buffer_type   = MYSQL_TYPE_STRING;
    upd[1].buffer        = (char*)receiver;
    upd[1].buffer_length = (unsigned long)strlen(receiver);
    mysql_stmt_bind_param(stmt, upd);
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // 7) 커밋 & 결과 반환
    mysql_query(conn, "COMMIT");
    *new_sbal = sbal;
    *new_rbal = rbal;
    return 0;
}

// 3) 출금 처리
int db_withdraw_by_number(const char *acct_num, long amount, long *new_balance) {
    MYSQL_STMT *stmt;
    long bal;

    // 트랜잭션 시작
    if (mysql_query(conn, "START TRANSACTION")) {
        fprintf(stderr, "[DB ERROR] START TRANSACTION: %s\n",
                mysql_error(conn));
        return -1;
    }

    // (A) SELECT balance FOR UPDATE
    const char *sql_select =
        "SELECT balance FROM accounts WHERE account_number = ? FOR UPDATE";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_select, (unsigned long)strlen(sql_select));

    {
        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));
        param[0].buffer_type   = MYSQL_TYPE_STRING;
        param[0].buffer        = (char*)acct_num;
        param[0].buffer_length = (unsigned long)strlen(acct_num);
        mysql_stmt_bind_param(stmt, param);
    }
    {
        MYSQL_BIND result[1];
        memset(result, 0, sizeof(result));
        result[0].buffer_type   = MYSQL_TYPE_LONGLONG;
        result[0].buffer        = &bal;
        result[0].buffer_length = sizeof(bal);
        mysql_stmt_bind_result(stmt, result);
    }

    mysql_stmt_execute(stmt);
    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        mysql_query(conn, "ROLLBACK");
        return -2;  // 계좌 없음
    }
    mysql_stmt_close(stmt);

    // 잔액 부족 체크
    if (bal < amount) {
        mysql_query(conn, "ROLLBACK");
        return -1;  // 잔액 부족
    }

    // (B) UPDATE
    bal -= amount;
    const char *sql_update =
        "UPDATE accounts SET balance = ? WHERE account_number = ?";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_update, (unsigned long)strlen(sql_update));

    {
        MYSQL_BIND upd_param[2];
        memset(upd_param, 0, sizeof(upd_param));
        upd_param[0].buffer_type   = MYSQL_TYPE_LONGLONG;
        upd_param[0].buffer        = &bal;
        upd_param[0].buffer_length = sizeof(bal);
        upd_param[1].buffer_type   = MYSQL_TYPE_STRING;
        upd_param[1].buffer        = (char*)acct_num;
        upd_param[1].buffer_length = (unsigned long)strlen(acct_num);
        mysql_stmt_bind_param(stmt, upd_param);
    }

    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // (C) COMMIT
    mysql_query(conn, "COMMIT");

    *new_balance = bal;
    return 0;
}

// 4) 잔액 조회 처리
int db_get_balance(const char *acct_num, long *balance) {
    MYSQL_STMT *stmt;
    long bal;

    const char *sql =
        "SELECT balance FROM accounts WHERE account_number = ?";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql));

    {
        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));
        param[0].buffer_type   = MYSQL_TYPE_STRING;
        param[0].buffer        = (char*)acct_num;
        param[0].buffer_length = (unsigned long)strlen(acct_num);
        mysql_stmt_bind_param(stmt, param);
    }
    {
        MYSQL_BIND result[1];
        memset(result, 0, sizeof(result));
        result[0].buffer_type   = MYSQL_TYPE_LONGLONG;
        result[0].buffer        = &bal;
        result[0].buffer_length = sizeof(bal);
        mysql_stmt_bind_result(stmt, result);
    }

    mysql_stmt_execute(stmt);
    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        return -2;  // 계좌 없음
    }
    mysql_stmt_close(stmt);

    *balance = bal;
    return 0;
}

// 5) 커넥션 종료
void db_close(void) {
    if (conn) mysql_close(conn);
}

