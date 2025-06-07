// src/batch_processor.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include "db.h"

// 단일 CSV 파일 처리: "계좌번호,구분(D/W),금액"
static void process_csv_file(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "[ERROR] 파일 열기 실패(%s): %s\n",
                filepath, strerror(errno));
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        int id;
        char type;
        long amount;
        if (sscanf(line, "%d,%c,%ld", &id, &type, &amount) != 3) {
            fprintf(stderr, "[WARN] 잘못된 라인 포맷: %s", line);
            continue;
        }

        int rc;
        long new_bal;
        if (type == 'D') {
            rc = db_deposit(id, amount, &new_bal);
        } else if (type == 'W') {
            rc = db_withdraw(id, amount, &new_bal);
        } else {
            fprintf(stderr, "[WARN] 알 수 없는 구분(%c)\n", type);
            continue;
        }

        if (rc == 0) {
            printf("[BATCH] %s 계좌=%d, 금액=%ld -> 잔액=%ld\n",
                   (type=='D' ? "입금" : "출금"), id, amount, new_bal);
        } else {
            fprintf(stderr, "[BATCH ERROR] %s 실패: 계좌=%d, 금액=%ld, 코드=%d\n",
                    (type=='D' ? "입금" : "출금"), id, amount, rc);
        }
    }
    fclose(fp);

    // 처리 완료 후 원본 삭제
    if (remove(filepath) != 0) {
        fprintf(stderr, "[ERROR] 파일 삭제 실패(%s): %s\n",
                filepath, strerror(errno));
    }
}

// 배치 모드 엔트리: 지정 디렉토리 내 모든 .csv 파일 순회
void run_batch_mode(const char *incoming_dir) {
    DIR *dp = opendir(incoming_dir);
    if (!dp) {
        fprintf(stderr, "[ERROR] 디렉토리 열기 실패(%s): %s\n",
                incoming_dir, strerror(errno));
        return;
    }

    struct dirent *ent;
    char pathbuf[512];
    while ((ent = readdir(dp)) != NULL) {
        if (ent->d_name[0] == '.') continue;  // . , .. 스킵
        size_t len = strlen(ent->d_name);
        if (len < 5 || strcmp(ent->d_name + len - 4, ".csv") != 0)
            continue;

        snprintf(pathbuf, sizeof(pathbuf),
                 "%s/%s", incoming_dir, ent->d_name);
        process_csv_file(pathbuf);
    }
    closedir(dp);
}

