// include/db.h

#ifndef DB_H
#define DB_H

#include <stdbool.h>

// 1) DB 연결 초기화
//    host: 호스트(예: "127.0.0.1")
//    user: DB 사용자 이름
//    pw:   비밀번호
//    db:   데이터베이스 이름(예: "bank")
//    port: 포트 번호(예: 3306)
//  → 성공하면 true, 실패하면 false
bool db_init(const char *host,
             const char *user,
             const char *pw,
             const char *db,
             unsigned int port);

// 2) 입금 처리
//    id: 계좌 번호
//    amount: 입금액
//    new_balance: 갱신된 잔액을 저장할 포인터
//  → 리턴값:
//     0  = 성공
//    -2  = 계좌 없음
//    -1  = 그 외(DB 오류)
int db_deposit(int id, long amount, long *new_balance);

// 3) 출금 처리
//    id: 계좌 번호
//    amount: 출금액
//    new_balance: 갱신된 잔액을 저장할 포인터
//  → 리턴값:
//     0  = 성공
//    -1  = 잔액 부족
//    -2  = 계좌 없음
//    -1  = 그 외(DB 오류)
int db_withdraw(int id, long amount, long *new_balance);

// 4) 종료 처리
//    프로그램 종료 시 호출해서 DB 커넥션을 닫음
void db_close(void);

#endif // DB_H

