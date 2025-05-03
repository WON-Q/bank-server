// src/main.c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>

#include "account.h"
#include "threadpool.h"

#define PORT 9090

static int listen_fd;

void handle_sigint(int signo) {
    (void)signo;
    printf("\nShutting down...\n");
    close(listen_fd);
    exit(0);
}

int main(void) {
    signal(SIGINT, handle_sigint);

    account_module_init();
    threadpool_init();

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(PORT)
    };
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(listen_fd, 10);
    printf("Bank server listening on port %d...\n", PORT);

    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        threadpool_add_task(client_fd);
    }

    return 0;
}

