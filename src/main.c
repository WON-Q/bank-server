// src/main.c
#include <stdio.h>
#include "account.h"

int main(void) {
    account_module_init();
    printf("Bank server starting on port 9090...\n");
    return 0;
}

