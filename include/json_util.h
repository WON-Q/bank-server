#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stddef.h>

// 계좌 문자열 최대 길이
#define MAX_ACCT_LEN 64

// 단일 계좌 입출금(JSON) 파싱
// account_number, amount
// 리턴: 0=성공, -1=파싱 실패, -2=필드 누락/타입 불일치
int parse_tx_request(const char *body,
                     char *out_acct_num, size_t max_len,
                     long *out_amount);

// 송금자→수금자 이체(JSON) 파싱
// sender_account, receiver_account, amount
// 리턴: 0=성공, -1=파싱 실패, -2=필드 누락/타입 불일치
int parse_transfer_request(const char *body,
                           char *out_sender,   size_t max_sender,
                           char *out_receiver, size_t max_receiver,
                           long *out_amount);

// 잔액 조회(JSON) 파싱
// account_number
// 리턴: 0=성공, -1=파싱 실패, -2=필드 누락/타입 불일치
int parse_balance_request(const char *body,
                          char *out_acct_num, size_t max_len);

#endif // JSON_UTIL_H

