#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "watchdog.h"

// --------------------------------------------------
// WATCHDOG THREAD
// select() ile pipe read-end'leri dinler.
// Her 3 saniyede stderr'e progress yazar.
// g_shutdown = 1 olunca çıkar.
// --------------------------------------------------
void *watchdog_thread(void *arg)
{
    t_watchdog_args *warg        = (t_watchdog_args *)arg;
    long             line_counts[MAX_FILES];
    char             buf[256];
    int              elapsed     = 0;

    memset(line_counts, 0, sizeof(line_counts));

    while (!(*warg->shutdown))
    {
        // select için fd_set hazırla
        fd_set  readfds;
        int     max_fd = 0;

        FD_ZERO(&readfds);
        for (int i = 0; i < warg->reader_count; i++)
        {
            int fd = warg->pipe_fds[i][0];
            FD_SET(fd, &readfds);
            if (fd > max_fd)
                max_fd = fd;
        }

        // 3 saniyelik timeout
        struct timeval tv;
        tv.tv_sec  = 3;
        tv.tv_usec = 0;

        int rc = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        // shutdown kontrolü — select dönüşünde hemen bak
        if (*warg->shutdown)
            break;

        if (rc < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }

        // Okunabilir pipe'lar varsa oku
        if (rc > 0)
        {
            for (int i = 0; i < warg->reader_count; i++)
            {
                int fd = warg->pipe_fds[i][0];
                if (!FD_ISSET(fd, &readfds))
                    continue;

                // Tüm mevcut satırları oku
                ssize_t n;
                while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
                {
                    buf[n] = '\0';

                    // "[R<i>] <count> lines processed" formatını parse et
                    int   ridx  = 0;
                    long  lines = 0;
                    if (sscanf(buf, "[R%d] %ld lines processed", &ridx, &lines) == 2)
                    {
                        if (ridx >= 0 && ridx < warg->reader_count)
                            line_counts[ridx] = lines;
                    }
                }
            }
        }

        // Her select dönüşünde (timeout veya data) snapshot bas
        elapsed += 3;

        // Kaç child hâlâ yaşıyor?
        int alive = 0;
        for (int i = 0; i < warg->pid_count; i++)
        {
            if (warg->all_pids[i] > 0)
            {
                int status;
                pid_t r = waitpid(warg->all_pids[i], &status, WNOHANG);
                if (r == 0)
                    alive++; // hâlâ çalışıyor
                else if (r > 0)
                    warg->all_pids[i] = -1; // bitti, işaretle
            }
        }

        // stderr'e yaz (stdout değil!)
        fprintf(stderr, "[WATCHDOG] Progress at T+%ds:", elapsed);
        for (int i = 0; i < warg->reader_count; i++)
            fprintf(stderr, " Reader%d=%ld", i, line_counts[i]);
        fprintf(stderr, " children_alive=%d\n", alive);
    }

    return (NULL);
}