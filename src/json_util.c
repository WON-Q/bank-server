// src/json_util.c

#include <stdio.h>
#include "json_util.h"
#include "cJSON.h"
#define LOG_ERR(fmt, ...) \
    fprintf(stderr, "[ERROR] " fmt " (%s:%d)\n", ##__VA_ARGS__, __FILE__, __LINE__)

int parse_request(const char *body, int *out_account, long *out_amount) {
    if (!body || !out_account || !out_amount){
	LOG_ERR("parse_request: null argument");
	return -1;
    }
    // JSON 파싱
    cJSON *root = cJSON_Parse(body);
    if (!root){
	LOG_ERR("parse_request: JSON parse failed (invalid JSON)");
        return -1;
    }

    // account 필드 조회
    cJSON *acct = cJSON_GetObjectItem(root, "account");
    cJSON *amt  = cJSON_GetObjectItem(root, "amount");
    if (!cJSON_IsNumber(acct) || !cJSON_IsNumber(amt)) {
        LOG_ERR("parse_request: missing or non-number field");
	cJSON_Delete(root);
        return -2;
    }

    // 값 저장
    *out_account = acct->valueint;
    *out_amount  = (long)amt->valuedouble;

    cJSON_Delete(root);
    return 0;
}
