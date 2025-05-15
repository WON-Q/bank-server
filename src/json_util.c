// src/json_util.c

#include <stdio.h>
#include <string.h>
#include "json_util.h"
#include "cJSON.h"

// 거래 요청 파싱 (account_number, amount)
int parse_tx_request(const char *body,
                     char *out_acct_num, size_t max_len,
                     long *out_amount) {
    if (!body || !out_acct_num || !out_amount) {
        fprintf(stderr, "[ERROR] parse_tx_request: null argument\n");
        return -1;
    }
    cJSON *root = cJSON_Parse(body);
    if (!root) {
        fprintf(stderr, "[ERROR] parse_tx_request: JSON parse failed\n");
        return -1;
    }
    cJSON *acct = cJSON_GetObjectItem(root, "account_number");
    cJSON *amt  = cJSON_GetObjectItem(root, "amount");
    if (!cJSON_IsString(acct) || !cJSON_IsNumber(amt)) {
        fprintf(stderr, "[ERROR] parse_tx_request: missing or wrong type\n");
        cJSON_Delete(root);
        return -2;
    }
    // 문자열 복사
    strncpy(out_acct_num, acct->valuestring, max_len - 1);
    out_acct_num[max_len - 1] = '\0';
    *out_amount = (long)amt->valuedouble;
    cJSON_Delete(root);
    return 0;
}

// 잔액 조회 요청 파싱 (account_number)
int parse_balance_request(const char *body,
                          char *out_acct_num, size_t max_len) {
    if (!body || !out_acct_num) {
        fprintf(stderr, "[ERROR] parse_balance_request: null argument\n");
        return -1;
    }
    cJSON *root = cJSON_Parse(body);
    if (!root) {
        fprintf(stderr, "[ERROR] parse_balance_request: JSON parse failed\n");
        return -1;
    }
    cJSON *acct = cJSON_GetObjectItem(root, "account_number");
    if (!cJSON_IsString(acct)) {
        fprintf(stderr, "[ERROR] parse_balance_request: missing or wrong type\n");
        cJSON_Delete(root);
        return -2;
    }
    strncpy(out_acct_num, acct->valuestring, max_len - 1);
    out_acct_num[max_len - 1] = '\0';
    cJSON_Delete(root);
    return 0;
}
