#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_STOCKS   128
#define MAX_CLIENTS  10
#define MAX_LINE     512
#define SYMBOL_LEN   9   
#define BCAST_INTERVAL 5  

typedef struct {
    char  symbol[SYMBOL_LEN];
    float price;
} stock_t;

typedef struct {
    char symbol[SYMBOL_LEN];
    int  qty;
} holding_t;

typedef struct {
    int       fd;
    char      username[32];
    char      type[16];      
    char      line_buf[MAX_LINE];
    int       buf_len;
    int       joined;    
    holding_t portfolio[MAX_STOCKS];
    int       port_count;   
} client_t;

static stock_t   stocks[MAX_STOCKS];
static int       stock_count = 0;

static client_t  clients[MAX_CLIENTS];
static int       client_count = 0;

static volatile sig_atomic_t got_sigint = 0;

static FILE     *logfp   = NULL;  

static int                udp_fd_g   = -1;
static struct sockaddr_in bcast_addr_g;

static void server_log(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    if (logfp) {
        va_start(ap, fmt);
        vfprintf(logfp, fmt, ap);
        va_end(ap);
        fflush(logfp);
    }
    fflush(stdout);
}

static void handle_sigint(int sig)
{
    (void)sig;
    got_sigint = 1;
}

static int load_stocks(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: stocks could not opened: %s\n", path);
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char sym[SYMBOL_LEN];
        float price;

        if (sscanf(line, "%8s %f", sym, &price) != 2) continue;
        if (price <= 0.0f) continue;

        int valid = 1;
        for (int i = 0; sym[i]; i++) {
            char c = sym[i];
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                valid = 0;
                break;
            }
        }
        if (!valid) continue;

        if (stock_count >= MAX_STOCKS) break;
        strncpy(stocks[stock_count].symbol, sym, SYMBOL_LEN - 1);
        stocks[stock_count].price = price;
        stock_count++;
    }

    fclose(fp);
    return stock_count;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "Kullanım: %s -p <tcp_port> -u <udp_port> -s <stocks.txt> -l <logfile>\n",
        prog);
}

static client_t *alloc_client(void)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            memset(&clients[i], 0, sizeof(clients[i]));
            clients[i].fd = -1;  
            return &clients[i];
        }
    }
    return NULL;
}

static int username_taken(const char *name)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd >= 0 && clients[i].joined &&
            strcmp(clients[i].username, name) == 0)
            return 1;
    }
    return 0;
}

static int safe_send(int fd, const char *msg)
{
    size_t total = strlen(msg);
    size_t sent  = 0;
    while (sent < total) {
        ssize_t n = send(fd, msg + sent, total - sent, 0);
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static void disconnect_client(int idx, const char *reason)
{
    if (clients[idx].fd < 0) return;
    server_log("[SERVER] CLIENT_DISCONNECTED username=%s reason=%s\n",
               clients[idx].username[0] ? clients[idx].username : "(unknown)",
               reason);
    close(clients[idx].fd);
    clients[idx].fd      = -1;
    clients[idx].joined  = 0;
    clients[idx].username[0] = '\0';
    clients[idx].buf_len     = 0;
    client_count--;
}

static void send_price_broadcast(const char *trigger);

static stock_t *find_stock(const char *symbol)
{
    for (int i = 0; i < stock_count; i++)
        if (strcmp(stocks[i].symbol, symbol) == 0)
            return &stocks[i];
    return NULL;
}

static int process_line(int idx, char *line)
{
    client_t *c = &clients[idx];
    char resp[MAX_LINE + 64];

    if (strncmp(line, "JOIN ", 5) == 0) {
        if (c->joined) { safe_send(c->fd, "ERR ALREADY_JOINED\n"); return 0; }
        char type_str[16], uname[32];
        if (sscanf(line + 5, "%15s %31s", type_str, uname) != 2) {
            safe_send(c->fd, "ERR UNKNOWN JOIN\n"); return 0;
        }
        if (strcmp(type_str, "TRADER") != 0 && strcmp(type_str, "ANALYST") != 0) {
            safe_send(c->fd, "ERR UNKNOWN JOIN\n"); return 0;
        }
        if (username_taken(uname)) {
            safe_send(c->fd, "ERR JOIN name_taken\n"); return 0;
        }
        strncpy(c->username, uname,    sizeof(c->username) - 1);
        strncpy(c->type,     type_str, sizeof(c->type)     - 1);
        c->joined = 1;
        server_log("[SERVER] JOIN username=%s type=%s fd=%d\n",
                   c->username, c->type, c->fd);
        snprintf(resp, sizeof(resp), "OK JOIN %s\n", c->username);
        safe_send(c->fd, resp);
        return 0;
    }

    if (!c->joined) { safe_send(c->fd, "ERR NOT_JOINED\n"); return 0; }

    if (strcmp(line, "QUIT") == 0) {
        safe_send(c->fd, "OK QUIT\n");
        return -1;
    }

    if (strcmp(c->type, "ANALYST") == 0) {

        if (strncmp(line, "PRICE ", 6) == 0) {
            char sym[SYMBOL_LEN];
            if (sscanf(line + 6, "%8s", sym) != 1) {
                safe_send(c->fd, "ERR UNKNOWN_SYMBOL\n"); return 0;
            }
            stock_t *s = find_stock(sym);
            if (!s) { safe_send(c->fd, "ERR UNKNOWN_SYMBOL\n"); return 0; }
            snprintf(resp, sizeof(resp), "OK PRICE %s %.2f\n", s->symbol, s->price);
            safe_send(c->fd, resp);
            return 0;
        }

        if (strcmp(line, "REPORT") == 0) {
            char buf[4096];
            int  off = snprintf(buf, sizeof(buf), "OK REPORT ");
            for (int i = 0; i < stock_count; i++) {
                if (i > 0) { buf[off] = ','; off++; }
                off += snprintf(buf + off, (size_t)(sizeof(buf) - off),
                                "%s:%.2f", stocks[i].symbol, stocks[i].price);
            }
            buf[off++] = '\n'; buf[off] = '\0';
            safe_send(c->fd, buf);
            return 0;
        }

        if (strcmp(line, "LIST") == 0) {
            char buf[2048];
            int  off = snprintf(buf, sizeof(buf), "OK LIST ");
            int  first = 1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd >= 0 && clients[i].joined) {
                    if (!first) { buf[off] = ','; off++; }
                    off += snprintf(buf + off, (size_t)(sizeof(buf) - off),
                                    "%s", clients[i].username);
                    first = 0;
                }
            }
            buf[off++] = '\n'; buf[off] = '\0';
            safe_send(c->fd, buf);
            return 0;
        }

        if (strncmp(line, "BUY ", 4) == 0 || strncmp(line, "SELL ", 5) == 0) {
            safe_send(c->fd, "ERR UNAUTHORIZED\n"); return 0;
        }

        snprintf(resp, sizeof(resp), "ERR UNKNOWN %s\n", line);
        safe_send(c->fd, resp);
        return 0;
    }

    if (strcmp(c->type, "TRADER") == 0) {

        if (strncmp(line, "BUY ", 4) == 0) {
            char sym[SYMBOL_LEN];
            int  qty;
            if (sscanf(line + 4, "%8s %d", sym, &qty) != 2 || qty <= 0) {
                safe_send(c->fd, "ERR UNKNOWN_SYMBOL\n"); return 0;
            }
            stock_t *s = find_stock(sym);
            if (!s) { safe_send(c->fd, "ERR UNKNOWN_SYMBOL\n"); return 0; }

            float old_price = s->price;
            float new_price = old_price + qty * 0.01f;
            s->price = new_price;

            int found = 0;
            for (int i = 0; i < c->port_count; i++) {
                if (strcmp(c->portfolio[i].symbol, sym) == 0) {
                    c->portfolio[i].qty += qty;
                    found = 1; break;
                }
            }
            if (!found && c->port_count < MAX_STOCKS) {
                strncpy(c->portfolio[c->port_count].symbol, sym, SYMBOL_LEN - 1);
                c->portfolio[c->port_count].qty = qty;
                c->port_count++;
            }

            server_log("[SERVER] BUY trader=%s symbol=%s qty=%d old_price=%.2f new_price=%.2f\n",
                       c->username, sym, qty, old_price, new_price);

            snprintf(resp, sizeof(resp), "OK BUY %s %d %.2f\n", sym, qty, new_price);
            safe_send(c->fd, resp);
            send_price_broadcast("TRADE");
            return 0;
        }

        if (strncmp(line, "SELL ", 5) == 0) {
            char sym[SYMBOL_LEN];
            int  qty;
            if (sscanf(line + 5, "%8s %d", sym, &qty) != 2 || qty <= 0) {
                safe_send(c->fd, "ERR UNKNOWN_SYMBOL\n"); return 0;
            }
            stock_t *s = find_stock(sym);
            if (!s) { safe_send(c->fd, "ERR UNKNOWN_SYMBOL\n"); return 0; }

            int held = 0, port_idx = -1;
            for (int i = 0; i < c->port_count; i++) {
                if (strcmp(c->portfolio[i].symbol, sym) == 0) {
                    held     = c->portfolio[i].qty;
                    port_idx = i;
                    break;
                }
            }
            if (held < qty) {
                safe_send(c->fd, "ERR INSUFFICIENT_SHARES\n"); return 0;
            }

            float old_price = s->price;
            float new_price = old_price - qty * 0.01f;
            if (new_price < 0.01f) new_price = 0.01f;
            s->price = new_price;

            c->portfolio[port_idx].qty -= qty;
            if (c->portfolio[port_idx].qty == 0) {
                c->portfolio[port_idx] = c->portfolio[c->port_count - 1];
                c->port_count--;
            }

            server_log("[SERVER] SELL trader=%s symbol=%s qty=%d old_price=%.2f new_price=%.2f\n",
                       c->username, sym, qty, old_price, new_price);

            snprintf(resp, sizeof(resp), "OK SELL %s %d %.2f\n", sym, qty, new_price);
            safe_send(c->fd, resp);
            send_price_broadcast("TRADE");
            return 0;
        }

        if (strcmp(line, "PORTFOLIO") == 0) {
            if (c->port_count == 0) {
                safe_send(c->fd, "OK PORTFOLIO EMPTY\n");
            } else {
                char buf[2048];
                int  off = snprintf(buf, sizeof(buf), "OK PORTFOLIO ");
                for (int i = 0; i < c->port_count; i++) {
                    if (i > 0) { buf[off] = ','; off++; }
                    off += snprintf(buf + off, (size_t)(sizeof(buf) - off),
                                    "%s:%d", c->portfolio[i].symbol, c->portfolio[i].qty);
                }
                buf[off++] = '\n';
                buf[off]   = '\0';
                safe_send(c->fd, buf);
            }
            return 0;
        }

        if (strncmp(line, "PRICE ", 6) == 0 ||
            strcmp(line, "REPORT")     == 0  ||
            strcmp(line, "LIST")       == 0) {
            safe_send(c->fd, "ERR UNAUTHORIZED\n"); return 0;
        }

        snprintf(resp, sizeof(resp), "ERR UNKNOWN %s\n", line);
        safe_send(c->fd, resp);
        return 0;
    }

    safe_send(c->fd, "ERR UNKNOWN\n");
    return 0;
}

static void send_price_broadcast(const char *trigger)
{
    if (udp_fd_g < 0) return;

    char buf[4096];
    int  off = snprintf(buf, sizeof(buf), "PRICE_UPDATE ");
    for (int i = 0; i < stock_count; i++) {
        if (i > 0) { buf[off] = ' '; off++; }
        off += snprintf(buf + off, (size_t)(sizeof(buf) - off),
                        "%s:%.2f", stocks[i].symbol, stocks[i].price);
    }
    buf[off++] = '\n';
    buf[off]   = '\0';

    sendto(udp_fd_g, buf, (size_t)off, 0,
           (struct sockaddr *)&bcast_addr_g, sizeof(bcast_addr_g));

    server_log("[SERVER] PRICE_BROADCAST trigger=%s\n", trigger);
}

static void client_read(int idx)
{
    client_t *c = &clients[idx];

    int space = MAX_LINE - 1 - c->buf_len;
    if (space <= 0) {
        safe_send(c->fd, "ERR TOOLONG\n");
        c->buf_len = 0;
        return;
    }

    ssize_t n = recv(c->fd, c->line_buf + c->buf_len, (size_t)space, 0);
    if (n <= 0) {
        disconnect_client(idx, "hangup");
        return;
    }
    c->buf_len += (int)n;

    while (1) {
        char *nl = memchr(c->line_buf, '\n', (size_t)c->buf_len);
        if (!nl) break;

        *nl = '\0';
        if (nl > c->line_buf && *(nl - 1) == '\r') *(nl - 1) = '\0';

        if ((int)(nl - c->line_buf) >= MAX_LINE - 1) {
            safe_send(c->fd, "ERR TOOLONG\n");
        } else {
            int ret = process_line(idx, c->line_buf);
            if (ret < 0) {
                disconnect_client(idx, "QUIT");
                return;
            }
        }

        int consumed = (int)(nl - c->line_buf) + 1;
        c->buf_len  -= consumed;
        if (c->buf_len > 0)
            memmove(c->line_buf, c->line_buf + consumed, (size_t)c->buf_len);
    }
}

static int rebuild_fdset(int tcp_fd, fd_set *rfds)
{
    FD_ZERO(rfds);
    FD_SET(tcp_fd, rfds);
    int maxfd = tcp_fd;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd >= 0) {
            FD_SET(clients[i].fd, rfds);
            if (clients[i].fd > maxfd)
                maxfd = clients[i].fd;
        }
    }
    return maxfd;
}

int main(int argc, char *argv[])
{
    int   tcp_port = -1, udp_port = -1;
    char *stocks_path = NULL, *log_path = NULL;

    for (int i = 1; i < argc; i++) {
        if      (!strcmp(argv[i], "-p") && i+1 < argc) tcp_port   = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-u") && i+1 < argc) udp_port   = atoi(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i+1 < argc) stocks_path = argv[++i];
        else if (!strcmp(argv[i], "-l") && i+1 < argc) log_path    = argv[++i];
        else { usage(argv[0]); return 1; }
    }

    if (tcp_port < 1024 || udp_port < 1024 || !stocks_path || !log_path) {
        usage(argv[0]);
        return 1;
    }

    logfp = fopen(log_path, "w");
    if (!logfp) {
        fprintf(stderr, "ERROR: log file could not opened: %s\n", log_path);
        return 1;
    }

    if (load_stocks(stocks_path) <= 0) {
        fprintf(stderr, "ERROR: stocks could not uploaded.\n");
        return 1;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i].fd = -1;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_fd < 0) { perror("socket TCP"); return 1; }

    int opt = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)tcp_port);

    if (bind(tcp_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind TCP"); return 1;
    }
    if (listen(tcp_fd, 10) < 0) {
        perror("listen"); return 1;
    }

    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_fd < 0) { perror("socket UDP"); return 1; }

    int bcast = 1;
    setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));

    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family      = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port        = 0;  
    bind(udp_fd, (struct sockaddr*)&udp_addr, sizeof(udp_addr));

    struct sockaddr_in bcast_addr;
    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family      = AF_INET;
    bcast_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    bcast_addr.sin_port        = htons((uint16_t)udp_port);
    udp_fd_g    = udp_fd;
    bcast_addr_g = bcast_addr;

    server_log("[SERVER] SERVER_START tcp_port=%d udp_port=%d stocks=%d\n",
               tcp_port, udp_port, stock_count);

    struct timespec last_bcast;
    clock_gettime(CLOCK_MONOTONIC, &last_bcast);

    while (!got_sigint) {

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_ms = (now.tv_sec  - last_bcast.tv_sec)  * 1000
                        + (now.tv_nsec - last_bcast.tv_nsec) / 1000000;
        long remain_ms  = BCAST_INTERVAL * 1000 - elapsed_ms;
        if (remain_ms < 0) remain_ms = 0;

        struct timeval tv;
        tv.tv_sec  = remain_ms / 1000;
        tv.tv_usec = (remain_ms % 1000) * 1000;

        fd_set rfds;
        int maxfd = rebuild_fdset(tcp_fd, &rfds);

        int ready = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (ready < 0) {
            if (errno == EINTR) continue; 
            perror("select");
            break;
        }

        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed_ms = (now.tv_sec  - last_bcast.tv_sec)  * 1000
                   + (now.tv_nsec - last_bcast.tv_nsec) / 1000000;

        if (elapsed_ms >= BCAST_INTERVAL * 1000 || ready == 0) {
            send_price_broadcast("PERIODIC");
            clock_gettime(CLOCK_MONOTONIC, &last_bcast);
        }

        if (ready == 0) continue;  

        if (FD_ISSET(tcp_fd, &rfds)) {
            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            int cli_fd = accept(tcp_fd, (struct sockaddr*)&cli_addr, &cli_len);

            if (cli_fd >= 0) {
                client_t *c = alloc_client();
                if (!c) {
                    close(cli_fd);
                } else {
                    c->fd = cli_fd;
                    client_count++;
                    server_log("[SERVER] CLIENT_CONNECTED fd=%d ip=%s\n",
                               cli_fd, inet_ntoa(cli_addr.sin_addr));
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd < 0) continue;
            if (!FD_ISSET(clients[i].fd, &rfds)) continue;
            client_read(i);
        }
    }

    server_log("[SERVER] SHUTDOWN signal=SIGINT\n");

    const char *shutdown_msg = "SERVER SHUTDOWN\n";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd >= 0) {
            send(clients[i].fd, shutdown_msg, strlen(shutdown_msg), 0);
            close(clients[i].fd);
            clients[i].fd = -1;
        }
    }

    close(tcp_fd);
    close(udp_fd);

    server_log("[SERVER] CLEANUP_DONE clients=0\n");
    if (logfp) fclose(logfp);
    return 0;
}