// include/json_util.h
#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include <stddef.h>

// JSON 요청 바디에서 'account' (int)와 'amount' (long) 값을 파싱합니다.
// body: JSON 문자열(null-terminated)
// out_account, out_amount: 파싱된 값을 저장할 포인터
// 반환값:
//   0  - 성공
//  -1  - JSON 파싱 실패
//  -2  - 필드 누락 또는 타입 불일치
int parse_request(const char *body, int *out_account, long *out_amount);

#endif // JSON_UTIL_H
