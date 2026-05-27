/*
 * ticker.c  –  UDP broadcast dinleyici
 * Kullanım: ./ticker <udp_port>
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile sig_atomic_t stop = 0;

static void handle_sigint(int sig) { (void)sig; stop = 1; }

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Kullanim: %s <udp_port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port < 1024) {
        fprintf(stderr, "ERROR: port >= 1024 olmali\n");
        return 1;
    }

    /* SIGINT handler */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    /* UDP soket aç */
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    /* SO_BROADCAST ve SO_REUSEADDR */
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* verilen porta bind et */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    printf("[TICKER] LISTENING udp_port=%d\n", port);
    fflush(stdout);

    char buf[4096];
    while (!stop) {
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n < 0) break;
        buf[n] = '\0';

        /* \r\n veya \n sonu temizle */
        while (n > 0 && (buf[n-1] == '\n' || buf[n-1] == '\r'))
            buf[--n] = '\0';

        printf("[TICKER] RECEIVED %s\n", buf);
        fflush(stdout);
    }

    close(fd);
    return 0;
}