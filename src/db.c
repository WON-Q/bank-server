#include "db.h"
#include <mysql/mysql.h>
#include <stdio.h>

static MYSQL *conn = NULL;

// MySQL Connector/C 표준에서 제공하는 메서드 활용
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

int db_deposit(int id, long amount, long *new_balance) {
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    long bal;

    mysql_query(conn, "START TRANSACTION");

    // 1) SELECT balance FOR UPDATE
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt,
      "SELECT balance FROM accounts WHERE id = ? FOR UPDATE", -1);
    // bind id
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

    // 2) UPDATE accounts SET balance = ? WHERE id = ?
    bal += amount;
    stmt = mysql_stmt_init(conn);
    mysql_stmt_prepare(stmt,
      "UPDATE accounts SET balance = ? WHERE id = ?", -1);
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer      = &bal;
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer      = &id;
    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);
    mysql_stmt_close(stmt);

    // 3) COMMIT
    mysql_query(conn, "COMMIT");

    *new_balance = bal;
    return 0;
}

