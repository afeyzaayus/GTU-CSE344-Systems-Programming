#define _POSIX_C_SOURCE 200809L

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <errno.h>

volatile sig_atomic_t g_shutdown = 0;

void handle_sigint(int sig) {
    (void)sig;
    g_shutdown = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <tcp_port> <username>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *username = argv[3];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); close(sockfd); return 1;
    }

    printf("[PROFESSOR %s] CONNECTED server=%s:%d\n", username, ip, port);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ENROLL PROFESSOR %s", username);
    send_line(sockfd, cmd);

    char net_buf[MAX_LINE_LEN];
    size_t net_len = 0;

    while (!g_shutdown) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        int max_fd = (sockfd > STDIN_FILENO) ? sockfd : STDIN_FILENO;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) break; 
            perror("select"); break;
        }

        if (FD_ISSET(sockfd, &read_fds)) {
            ssize_t n = read(sockfd, net_buf + net_len, sizeof(net_buf) - net_len - 1);
            if (n <= 0) break; 
            net_len += n;
            net_buf[net_len] = '\0';

            char *nl;
            while ((nl = strchr(net_buf, '\n')) != NULL) {
                *nl = '\0';
                char *line = net_buf;
                if (nl > net_buf && *(nl - 1) == '\r') *(nl - 1) = '\0'; 
                
                printf("[PROFESSOR %s] RECEIVED %s\n", username, line);

                if (strstr(line, "TIMEOUT DISCONNECT") || strstr(line, "SERVER SHUTDOWN")) {
                    printf("[PROFESSOR %s] DISCONNECTED reason=shutdown/timeout\n", username);
                    close(sockfd);
                    return 0;
                }

                size_t consumed = (nl - net_buf) + 1;
                memmove(net_buf, net_buf + consumed, net_len - consumed);
                net_len -= consumed;
                net_buf[net_len] = '\0';
            }
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char input[256];
            if (fgets(input, sizeof(input), stdin)) {
                trim_newline(input);
                if (strlen(input) > 0) {
                    send_line(sockfd, input);
                    printf("[PROFESSOR %s] SENT %s\n", username, input);
                }
            }
        }
    }

    send_line(sockfd, "APPARATE");
    printf("[PROFESSOR %s] SENT APPARATE\n", username);
    
    char resp[128];
    if (read(sockfd, resp, sizeof(resp)) > 0) {
        resp[strcspn(resp, "\r\n")] = '\0';
        printf("[PROFESSOR %s] RECEIVED %s\n", username, resp);
    }
    
    printf("[PROFESSOR %s] DISCONNECTED reason=APPARATE\n", username);
    close(sockfd);
    return 0;
}