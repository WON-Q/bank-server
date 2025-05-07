// src/json_util.c
#include "json_util.h"
#include "cJSON.h"

int parse_request(const char *body, int *out_account, long *out_amount) {
    if (!body || !out_account || !out_amount) return -1;
    // JSON 파싱
    cJSON *root = cJSON_Parse(body);
    if (!root) return -1;

    // account 필드 조회
    cJSON *acct = cJSON_GetObjectItem(root, "account");
    cJSON *amt  = cJSON_GetObjectItem(root, "amount");
    if (!cJSON_IsNumber(acct) || !cJSON_IsNumber(amt)) {
        cJSON_Delete(root);
        return -2;
    }

    // 값 저장
    *out_account = acct->valueint;
    *out_amount  = (long)amt->valuedouble;

    cJSON_Delete(root);
    return 0;
}
