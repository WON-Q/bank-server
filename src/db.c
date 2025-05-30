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
    long sbal, rbal;

    // 트랜잭션 시작
    if (mysql_query(conn, "START TRANSACTION")) {
        mysql_query(conn, "ROLLBACK");
        return -1;
    }

    // (1) sender 조회 FOR UPDATE
    const char *sql_sel = 
      "SELECT balance FROM accounts WHERE account_number=? FOR UPDATE";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_sel, strlen(sql_sel));
    {
        MYSQL_BIND param = {0};
        param.buffer_type   = MYSQL_TYPE_STRING;
        param.buffer        = (char*)sender;
        param.buffer_length = strlen(sender);
        mysql_stmt_bind_param(stmt, &param);
    }
    {
        MYSQL_BIND result = {0};
        result.buffer_type   = MYSQL_TYPE_LONGLONG;
        result.buffer        = &sbal;
        result.buffer_length = sizeof(sbal);
        mysql_stmt_bind_result(stmt, &result);
    }
    mysql_stmt_execute(stmt);
    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        mysql_query(conn, "ROLLBACK");
        return -2;  // sender 계좌 없음
    }
    mysql_stmt_close(stmt);

    // 잔액 부족
    if (sbal < amount) {
        mysql_query(conn, "ROLLBACK");
        return -3;
    }

    // (2) receiver 조회 FOR UPDATE
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_sel, strlen(sql_sel));
    {
        MYSQL_BIND param = {0};
        param.buffer_type   = MYSQL_TYPE_STRING;
        param.buffer        = (char*)receiver;
        param.buffer_length = strlen(receiver);
        mysql_stmt_bind_param(stmt, &param);
    }
    {
        MYSQL_BIND result = {0};
        result.buffer_type   = MYSQL_TYPE_LONGLONG;
        result.buffer        = &rbal;
        result.buffer_length = sizeof(rbal);
        mysql_stmt_bind_result(stmt, &result);
    }
    mysql_stmt_execute(stmt);
    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        mysql_query(conn, "ROLLBACK");
        return -2;  // receiver 계좌 없음
    }
    mysql_stmt_close(stmt);

    // (3) sender UPDATE
    sbal -= amount;
    const char *sql_upd = 
      "UPDATE accounts SET balance=? WHERE account_number=?";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_upd, strlen(sql_upd));
    {
        MYSQL_BIND params[2] = {0};
        params[0].buffer_type   = MYSQL_TYPE_LONGLONG;
        params[0].buffer        = &sbal;
        params[0].buffer_length = sizeof(sbal);
        params[1].buffer_type   = MYSQL_TYPE_STRING;
        params[1].buffer        = (char*)sender;
        params[1].buffer_length = strlen(sender);
        mysql_stmt_bind_param(stmt, params);
    }
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // (4) receiver UPDATE
    rbal += amount;
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_upd, strlen(sql_upd));
    {
        MYSQL_BIND params[2] = {0};
        params[0].buffer_type   = MYSQL_TYPE_LONGLONG;
        params[0].buffer        = &rbal;
        params[0].buffer_length = sizeof(rbal);
        params[1].buffer_type   = MYSQL_TYPE_STRING;
        params[1].buffer        = (char*)receiver;
        params[1].buffer_length = strlen(receiver);
        mysql_stmt_bind_param(stmt, params);
    }
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // (5) 커밋
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

