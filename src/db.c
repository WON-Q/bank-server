// src/db.c

#include "db.h"
#include <string.h>
#include <mysql/mysql.h>
#include <stdio.h>

static MYSQL *conn = NULL;

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

// 2) 입금: 트랜잭션 + FOR UPDATE + UPDATE
int db_deposit(int id, long amount, long *new_balance) {
    MYSQL_STMT *stmt;
    long bal;

    // 트랜잭션 시작
    if (mysql_query(conn, "START TRANSACTION")) {
        fprintf(stderr, "[DB ERROR] START TRANSACTION: %s\n",
                mysql_error(conn));
        return -1;
    }

    // (A) SELECT … FOR UPDATE
    const char *sql_select =
        "SELECT balance FROM accounts WHERE id = ? FOR UPDATE";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_select, (unsigned long)strlen(sql_select));

    {
        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));
        param[0].buffer_type   = MYSQL_TYPE_LONG;
        param[0].buffer        = &id;
        param[0].buffer_length = sizeof(id);
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

    // (B) UPDATE
    bal += amount;
    const char *sql_update =
        "UPDATE accounts SET balance = ? WHERE id = ?";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_update, (unsigned long)strlen(sql_update));

    {
        MYSQL_BIND upd_param[2];
        memset(upd_param, 0, sizeof(upd_param));
        upd_param[0].buffer_type   = MYSQL_TYPE_LONGLONG;
        upd_param[0].buffer        = &bal;
        upd_param[0].buffer_length = sizeof(bal);
        upd_param[1].buffer_type   = MYSQL_TYPE_LONG;
        upd_param[1].buffer        = &id;
        upd_param[1].buffer_length = sizeof(id);
        mysql_stmt_bind_param(stmt, upd_param);
    }

    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // (C) COMMIT
    mysql_query(conn, "COMMIT");

    *new_balance = bal;
    return 0;
}

// 3) 출금: 입금과 동일, 다만 잔액 부족 시 ROLLBACK + -1
int db_withdraw(int id, long amount, long *new_balance) {
    MYSQL_STMT *stmt;
    long bal;

    // 트랜잭션 시작
    if (mysql_query(conn, "START TRANSACTION")) {
        fprintf(stderr, "[DB ERROR] START TRANSACTION: %s\n",
                mysql_error(conn));
        return -1;
    }

    // (A) SELECT … FOR UPDATE
    const char *sql_select =
        "SELECT balance FROM accounts WHERE id = ? FOR UPDATE";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_select, (unsigned long)strlen(sql_select));

    {
        MYSQL_BIND param[1];
        memset(param, 0, sizeof(param));
        param[0].buffer_type   = MYSQL_TYPE_LONG;
        param[0].buffer        = &id;
        param[0].buffer_length = sizeof(id);
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
        "UPDATE accounts SET balance = ? WHERE id = ?";
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt, sql_update, (unsigned long)strlen(sql_update));

    {
        MYSQL_BIND upd_param[2];
        memset(upd_param, 0, sizeof(upd_param));
        upd_param[0].buffer_type   = MYSQL_TYPE_LONGLONG;
        upd_param[0].buffer        = &bal;
        upd_param[0].buffer_length = sizeof(bal);
        upd_param[1].buffer_type   = MYSQL_TYPE_LONG;
        upd_param[1].buffer        = &id;
        upd_param[1].buffer_length = sizeof(id);
        mysql_stmt_bind_param(stmt, upd_param);
    }

    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // (C) COMMIT
    mysql_query(conn, "COMMIT");

    *new_balance = bal;
    return 0;
}

// 4) 커넥션 종료
void db_close(void) {
    if (conn) mysql_close(conn);
}

