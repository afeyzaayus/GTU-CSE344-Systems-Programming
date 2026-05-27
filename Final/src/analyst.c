/*
 * analyst.c  –  TCP analyst istemcisi
 * Kullanim: ./analyst <server_ip> <tcp_port> <username>
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LINE 512

static volatile sig_atomic_t got_sigint = 0;
static void handle_sigint(int sig) { (void)sig; got_sigint = 1; }

static char g_username[32];

static void client_log(const char *fmt, ...)
{
    printf("[ANALYST %s] ", g_username);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
}

static int safe_send(int fd, const char *msg)
{
    size_t total = strlen(msg), sent = 0;
    while (sent < total) {
        ssize_t n = send(fd, msg + sent, total - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        fprintf(stderr, "Kullanim: %s <server_ip> <tcp_port> <username>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int         tcp_port  = atoi(argv[2]);
    strncpy(g_username, argv[3], sizeof(g_username) - 1);

    if (tcp_port < 1024) {
        fprintf(stderr, "ERROR: port >= 1024 olmali\n"); return 1;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in srv;
    memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port   = htons((uint16_t)tcp_port);
    if (inet_pton(AF_INET, server_ip, &srv.sin_addr) <= 0) {
        fprintf(stderr, "ERROR: gecersiz IP: %s\n", server_ip); return 1;
    }
    if (connect(fd, (struct sockaddr *)&srv, sizeof(srv)) < 0) {
        perror("connect"); return 1;
    }

    client_log("CONNECTED server=%s:%d\n", server_ip, tcp_port);

    /* JOIN gönder */
    char join_msg[MAX_LINE];
    snprintf(join_msg, sizeof(join_msg), "JOIN ANALYST %s\n", g_username);
    client_log("SENT JOIN\n");
    safe_send(fd, join_msg);

    char srv_buf[MAX_LINE];
    int  srv_len = 0;

    while (!got_sigint) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        int maxfd = fd > STDIN_FILENO ? fd : STDIN_FILENO;

        int ready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) continue;
            break;
        }

        /* sunucudan gelen veri */
        if (FD_ISSET(fd, &rfds)) {
            int space = MAX_LINE - 1 - srv_len;
            ssize_t n = recv(fd, srv_buf + srv_len, (size_t)space, 0);
            if (n <= 0) {
                client_log("DISCONNECTED reason=shutdown\n");
                break;
            }
            srv_len += (int)n;

            while (1) {
                char *nl = memchr(srv_buf, '\n', (size_t)srv_len);
                if (!nl) break;
                *nl = '\0';
                if (nl > srv_buf && *(nl-1) == '\r') *(nl-1) = '\0';

                if (strcmp(srv_buf, "SERVER SHUTDOWN") == 0) {
                    client_log("DISCONNECTED reason=shutdown\n");
                    close(fd);
                    return 0;
                }

                client_log("RECEIVED %s\n", srv_buf);

                int consumed = (int)(nl - srv_buf) + 1;
                srv_len -= consumed;
                if (srv_len > 0)
                    memmove(srv_buf, srv_buf + consumed, (size_t)srv_len);
            }
        }

        /* stdin'den komut */
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            char line[MAX_LINE];
            if (!fgets(line, sizeof(line), stdin)) {
                /* Ctrl+D */
                client_log("SENT QUIT\n");
                safe_send(fd, "QUIT\n");
                char tmp[MAX_LINE];
                ssize_t n = recv(fd, tmp, sizeof(tmp)-1, 0);
                if (n > 0) {
                    tmp[n] = '\0';
                    char *nl = strchr(tmp, '\n'); if (nl) *nl = '\0';
                    client_log("RECEIVED %s\n", tmp);
                }
                client_log("DISCONNECTED reason=QUIT\n");
                break;
            }

            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
                line[--len] = '\0';

            if (len == 0) continue;

            if (strcmp(line, "QUIT") == 0) {
                client_log("SENT QUIT\n");
                safe_send(fd, "QUIT\n");
                char tmp[MAX_LINE];
                ssize_t n = recv(fd, tmp, sizeof(tmp)-1, 0);
                if (n > 0) {
                    tmp[n] = '\0';
                    char *nl = strchr(tmp, '\n'); if (nl) *nl = '\0';
                    client_log("RECEIVED %s\n", tmp);
                }
                client_log("DISCONNECTED reason=QUIT\n");
                break;
            }

            client_log("SENT %s\n", line);
            line[MAX_LINE-2] = '\0';
            size_t llen = strlen(line);
            line[llen] = '\n'; line[llen+1] = '\0';
            safe_send(fd, line);
        }
    }

    close(fd);
    return 0;
}