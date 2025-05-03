// src/http.c
#include "http.h"
#include <unistd.h>
#include <stdio.h>

void handle_connection(int client_fd) {
    // TODO: read(), 파싱, account_deposit/withdraw 호출, write(), close()
    printf("Accepted fd=%d\n", client_fd);
    close(client_fd);
}
