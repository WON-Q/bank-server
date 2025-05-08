// src/db.c

#include "db.h"
#include <string.h>
#include <mysql.h>
#include <stdio.h>

static MYSQL *conn = NULL;

// 기본적으로 db와의 통신은 MySQL의 Connector/C 라이브러리 표준 API 스펙을 따른다.

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
        fprintf(stderr, "[ERROR] mysql_real_connect: %s\n", mysql_error(conn));
        return false;
    }
    return true;
}

// 2) 입금: 트랜잭션 + FOR UPDATE + UPDATE
int db_deposit(int id, long amount, long *new_balance) {
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    long bal;

    mysql_query(conn, "START TRANSACTION");

    // SELECT … FOR UPDATE
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt,
        "SELECT balance FROM accounts WHERE id = ? FOR UPDATE", -1);
    memset(bind,0,sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer      = &id;
    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);
    mysql_stmt_bind_result(stmt, bind);
    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        mysql_query(conn, "ROLLBACK");
        return -2;  // 계좌 없음
    }
    bal = *(long*)bind[0].buffer;
    mysql_stmt_close(stmt);

    // 잔액 갱신
    bal += amount;
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt,
        "UPDATE accounts SET balance = ? WHERE id = ?", -1);
    memset(bind,0,sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer      = &bal;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer      = &id;
    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    mysql_query(conn, "COMMIT");
    *new_balance = bal;
    return 0;
}

// 3) 출금: 입금과 동일, 다만 잔액 부족 시 ROLLBACK + -1
int db_withdraw(int id, long amount, long *new_balance) {
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    long bal;

    mysql_query(conn, "START TRANSACTION");

    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt,
        "SELECT balance FROM accounts WHERE id = ? FOR UPDATE", -1);
    memset(bind,0,sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONG;
    bind[0].buffer      = &id;
    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);
    mysql_stmt_bind_result(stmt, bind);
    if (mysql_stmt_fetch(stmt) != 0) {
        mysql_stmt_close(stmt);
        mysql_query(conn, "ROLLBACK");
        return -2;  // 계좌 없음
    }
    bal = *(long*)bind[0].buffer;
    mysql_stmt_close(stmt);

    if (bal < amount) {
        mysql_query(conn, "ROLLBACK");
        return -1;  // 잔액 부족
    }

    bal -= amount;
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt,
        "UPDATE accounts SET balance = ? WHERE id = ?", -1);
    memset(bind,0,sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer      = &bal;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer      = &id;
    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    mysql_query(conn, "COMMIT");
    *new_balance = bal;
    return 0;
}

// 4) 커넥션 종료
void db_close(void) {
    if (conn) mysql_close(conn);
}

