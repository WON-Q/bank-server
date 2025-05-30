// src/json_util.c

#include <stdio.h>
#include <string.h>
#include "json_util.h"
#include "cJSON.h"

// 거래 요청 파싱 (sender_account_number, receiver_amount_number, amount)
int parse_transfer_request(const char *body,
                           char *out_sender, size_t max_sender,
                           char *out_receiver, size_t max_receiver,
                           long *out_amount) {
    if (!body || !out_sender || !out_receiver || !out_amount) {
        fprintf(stderr, "[ERROR] parse_transfer_request: null arg\n");
        return -1;
    }
    cJSON *root = cJSON_Parse(body);
    if (!root) {
        fprintf(stderr, "[ERROR] parse_transfer_request: parse fail\n");
        return -1;
    }
    cJSON *js_sender   = cJSON_GetObjectItem(root, "sender_account");
    cJSON *js_receiver = cJSON_GetObjectItem(root, "receiver_account");
    cJSON *js_amount   = cJSON_GetObjectItem(root, "amount");
    if (!cJSON_IsString(js_sender) ||
        !cJSON_IsString(js_receiver) ||
        !cJSON_IsNumber(js_amount)) {
        fprintf(stderr, "[ERROR] parse_transfer_request: missing/wrong type\n");
        cJSON_Delete(root);
        return -2;
    }
    strncpy(out_sender, js_sender->valuestring, max_sender - 1);
    out_sender[max_sender - 1] = '\0';
    strncpy(out_receiver, js_receiver->valuestring, max_receiver - 1);
    out_receiver[max_receiver - 1] = '\0';
    *out_amount = (long)js_amount->valuedouble;
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
