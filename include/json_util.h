// include/json_util.h
#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stddef.h>

// 거래 요청 파싱: account_number (문자열) 과 amount (숫자) 추출
// 반환값:
//   0  - 성공
//  -1  - JSON 파싱 실패
//  -2  - 필드 누락 또는 타입 불일치
int parse_tx_request(const char *body,
                     char *out_acct_num, size_t max_len,
                     long *out_amount);

// 잔액 조회 요청 파싱: account_number (문자열) 추출
// 반환값:
//   0  - 성공
//  -1  - JSON 파싱 실패
//  -2  - 필드 누락 또는 타입 불일치
int parse_balance_request(const char *body,
                          char *out_acct_num, size_t max_len);

#endif // JSON_UTIL_H
