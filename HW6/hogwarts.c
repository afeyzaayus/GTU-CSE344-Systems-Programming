#define _POSIX_C_SOURCE 200809L

#include "common.h"

#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>

#define MAX_INGREDIENTS        256
#define MAX_SPELLBOOK_ENTRIES  64

typedef struct {
    char name[MAX_INGREDIENT_LEN];
    int  qty;
} ingredient_t;

typedef struct {
    char name[MAX_INGREDIENT_LEN];
    int  qty;
} spell_entry_t;

typedef struct {
    int           fd;
    char          username[MAX_USERNAME_LEN];
    char          type[MAX_TYPE_LEN];  
    int           enrolled;          
    char          line_buf[MAX_LINE_LEN];
    size_t        line_len;
    time_t        last_active;
    char          ip_str[INET_ADDRSTRLEN];

    spell_entry_t spellbook[MAX_SPELLBOOK_ENTRIES];
    int           spell_count;
} client_t;

typedef struct {
    int   port;
    char *ingredients_path;
    char *logfile_path;
    int   max_clients;
    int   timeout_sec;
} config_t;

static ingredient_t g_ingredients[MAX_INGREDIENTS];
static int          g_ingredient_count = 0;

static client_t    *g_clients = NULL;
static int          g_max_clients = 0;
static int          g_timeout_sec = 0; 

static FILE        *g_logfp = NULL;
static volatile sig_atomic_t g_shutdown_flag = 0;

static void on_sigint(int signo){
    (void)signo;
    g_shutdown_flag = 1;
}

static int install_sigint_handler(void){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        perror("sigaction(SIGINT)"); return -1;
    }
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        perror("sigaction(SIGPIPE)"); return -1;
    }
    return 0;
}

static void print_usage(const char *prog){
    fprintf(stderr,
            "Usage: %s -p <tcp_port> -s <ingredients.txt> -l <logfile> "
            "-n <max_clients> -t <timeout>\n", prog);
}

static int parse_args(int argc, char *argv[], config_t *cfg){
    cfg->port = -1; cfg->ingredients_path = NULL; cfg->logfile_path = NULL;
    cfg->max_clients = -1; cfg->timeout_sec = -1;

    int opt;
    while ((opt = getopt(argc, argv, "p:s:l:n:t:")) != -1) {
        switch (opt) {
            case 'p': cfg->port = atoi(optarg); break;
            case 's': cfg->ingredients_path = optarg; break;
            case 'l': cfg->logfile_path = optarg; break;
            case 'n': cfg->max_clients = atoi(optarg); break;
            case 't': cfg->timeout_sec = atoi(optarg); break;
            default:  print_usage(argv[0]); return -1;
        }
    }
    if (cfg->port < 0 || !cfg->ingredients_path || !cfg->logfile_path ||
        cfg->max_clients < 0 || cfg->timeout_sec < 0) {
        fprintf(stderr, "Error: missing required argument(s).\n");
        print_usage(argv[0]); return -1;
    }
    if (cfg->port < 1024 || cfg->port > 65535) {
        fprintf(stderr, "Error: port must be in [1024, 65535].\n"); return -1;
    }
    if (cfg->max_clients < 1) {
        fprintf(stderr, "Error: max_clients must be >= 1.\n"); return -1;
    }
    if (cfg->timeout_sec < 1) {
        fprintf(stderr, "Error: timeout must be >= 1.\n"); return -1;
    }
    return 0;
}

static int is_valid_ingredient_name(const char *s){
    size_t len = strlen(s);
    if (len == 0 || len > 16) return 0;
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            return 0;
    }
    return 1;
}

static int load_ingredients(const char *path){
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Error: cannot open ingredients file '%s': %s\n",
                path, strerror(errno));
        return -1;
    }
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (g_ingredient_count >= MAX_INGREDIENTS) break;
        char name[64]; int qty;
        if (sscanf(line, "%63s %d", name, &qty) != 2) continue;
        if (!is_valid_ingredient_name(name)) continue;
        if (qty <= 0) continue;

        int dup = 0;
        for (int i = 0; i < g_ingredient_count; i++) {
            if (strcmp(g_ingredients[i].name, name) == 0) { dup = 1; break; }
        }
        if (dup) continue;

        strncpy(g_ingredients[g_ingredient_count].name, name,
                MAX_INGREDIENT_LEN - 1);
        g_ingredients[g_ingredient_count].name[MAX_INGREDIENT_LEN - 1] = '\0';
        g_ingredients[g_ingredient_count].qty = qty;
        g_ingredient_count++;
    }
    fclose(fp);
    return g_ingredient_count;
}

static int find_ingredient(const char *name){
    for (int i = 0; i < g_ingredient_count; i++) {
        if (strcmp(g_ingredients[i].name, name) == 0) return i;
    }
    return -1;
}

static void reset_client_slot(int idx){
    memset(&g_clients[idx], 0, sizeof(client_t));
    g_clients[idx].fd = -1;
}

static int find_free_slot(void){
    for (int i = 0; i < g_max_clients; i++)
        if (g_clients[i].fd < 0) return i;
    return -1;
}

static int username_taken(const char *uname, int except_idx){
    for (int i = 0; i < g_max_clients; i++) {
        if (i == except_idx) continue;
        if (g_clients[i].fd < 0) continue;
        if (!g_clients[i].enrolled) continue;
        if (strcmp(g_clients[i].username, uname) == 0) return 1;
    }
    return 0;
}

static void remove_client(int idx, const char *reason){
    if (g_clients[idx].fd < 0) return;
    const char *uname = g_clients[idx].enrolled
                        ? g_clients[idx].username : "-";
    log_event(g_logfp, "CLIENT_DISCONNECTED username=%s reason=%s",
              uname, reason);
    close(g_clients[idx].fd);
    reset_client_slot(idx);
}

static int spell_find(const client_t *c, const char *name){
    for (int i = 0; i < c->spell_count; i++) {
        if (strcmp(c->spellbook[i].name, name) == 0) return i;
    }
    return -1;
}

static int spell_add(client_t *c, const char *name, int qty){
    int idx = spell_find(c, name);
    if (idx >= 0) {
        c->spellbook[idx].qty += qty;
        return 1;
    }
    if (c->spell_count >= MAX_SPELLBOOK_ENTRIES) return 0;
    strncpy(c->spellbook[c->spell_count].name, name, MAX_INGREDIENT_LEN - 1);
    c->spellbook[c->spell_count].name[MAX_INGREDIENT_LEN - 1] = '\0';
    c->spellbook[c->spell_count].qty = qty;
    c->spell_count++;
    return 1;
}

static void spell_sub(client_t *c, const char *name, int qty){
    int idx = spell_find(c, name);
    if (idx < 0) return;  
    c->spellbook[idx].qty -= qty;
    if (c->spellbook[idx].qty < 0) c->spellbook[idx].qty = 0;
    if (c->spellbook[idx].qty == 0) {
        c->spellbook[idx] = c->spellbook[c->spell_count - 1];
        c->spell_count--;
    }
}

static int parse_positive_int(const char *s, int *out){
    if (!s || s[0] == '\0') return 0;
    char *end;
    long v = strtol(s, &end, 10);
    if (*end != '\0') return 0;
    if (v <= 0 || v > 1000000) return 0;
    *out = (int)v;
    return 1;
}

static void cmd_brew(client_t *c){
    char *ing = strtok(NULL, " \t");
    char *qs  = strtok(NULL, " \t");
    int qty;
    if (!ing || !qs || !parse_positive_int(qs, &qty)) {
        send_line(c->fd, "ERR BREW bad_format");
        return;
    }
    int idx = find_ingredient(ing);
    if (idx < 0) {
        send_line(c->fd, "ERR UNKNOWN_INGREDIENT");
        return;
    }
    int old_qty = g_ingredients[idx].qty;
    g_ingredients[idx].qty += qty;
    spell_add(c, ing, qty);

    char resp[128];
    snprintf(resp, sizeof(resp), "OK BREW %s %d %d",
             ing, qty, g_ingredients[idx].qty);
    send_line(c->fd, resp);
    log_event(g_logfp,
              "BREW wizard=%s ingredient=%s qty=%d old_qty=%d new_qty=%d",
              c->username, ing, qty, old_qty, g_ingredients[idx].qty);
}

static void cmd_consume(client_t *c){
    char *ing = strtok(NULL, " \t");
    char *qs  = strtok(NULL, " \t");
    int qty;
    if (!ing || !qs || !parse_positive_int(qs, &qty)) {
        send_line(c->fd, "ERR CONSUME bad_format");
        return;
    }
    int idx = find_ingredient(ing);
    if (idx < 0) {
        send_line(c->fd, "ERR UNKNOWN_INGREDIENT");
        return;
    }

    int s_idx = spell_find(c, ing);
    int spell_qty = (s_idx >= 0) ? c->spellbook[s_idx].qty : 0;
    
    if (spell_qty < qty) {
        send_line(c->fd, "ERR INSUFFICIENT_INGREDIENTS");
        return;
    }

    int old_qty = g_ingredients[idx].qty;
    g_ingredients[idx].qty -= qty;
    spell_sub(c, ing, qty);

    char resp[128];
    snprintf(resp, sizeof(resp), "OK CONSUME %s %d %d",
             ing, qty, g_ingredients[idx].qty);
    send_line(c->fd, resp);
    
    log_event(g_logfp,
          "CONSUME wizard=%s ingredient=%s qty=%d old_qty=%d new_qty=%d",
          c->username, ing, qty, old_qty, g_ingredients[idx].qty);
}

static void cmd_spellbook(client_t *c){
    if (c->spell_count == 0) {
        send_line(c->fd, "OK SPELLBOOK EMPTY");
        return;
    }
    char buf[MAX_LINE_LEN];
    int off = snprintf(buf, sizeof(buf), "OK SPELLBOOK ");
    for (int i = 0; i < c->spell_count; i++) {
        int written = snprintf(buf + off, sizeof(buf) - off,
                               "%s%s:%d",
                               (i == 0 ? "" : ","),
                               c->spellbook[i].name, c->spellbook[i].qty);
        if (written < 0 || (size_t)written >= sizeof(buf) - off) break;
        off += written;
    }
    send_line(c->fd, buf);
}

static void cmd_inspect(client_t *c){
    char *ing = strtok(NULL, " \t");
    if (!ing) {
        send_line(c->fd, "ERR INSPECT bad_format");
        return;
    }
    int idx = find_ingredient(ing);
    if (idx < 0) {
        send_line(c->fd, "ERR UNKNOWN_INGREDIENT");
        return;
    }
    char resp[128];
    snprintf(resp, sizeof(resp), "OK INSPECT %s %d",
             ing, g_ingredients[idx].qty);
    send_line(c->fd, resp);
    log_event(g_logfp, "INSPECT professor=%s ingredient=%s qty=%d",
              c->username, ing, g_ingredients[idx].qty);
}

static void cmd_scroll(client_t *c){
    char buf[MAX_LINE_LEN];
    int off = snprintf(buf, sizeof(buf), "OK SCROLL ");
    int written_any = 0;
    for (int i = 0; i < g_ingredient_count; i++) {
        int written = snprintf(buf + off, sizeof(buf) - off,
                               "%s%s:%d",
                               (written_any == 0 ? "" : ","),
                               g_ingredients[i].name, g_ingredients[i].qty);
        if (written < 0 || (size_t)written >= sizeof(buf) - off) break;
        off += written;
        written_any = 1;
    }
    send_line(c->fd, buf);
    log_event(g_logfp, "SCROLL professor=%s ingredients=%d",
              c->username, g_ingredient_count);
}

static void cmd_roster(client_t *c){
    char buf[MAX_LINE_LEN];
    int off = snprintf(buf, sizeof(buf), "OK ROSTER ");
    int written_any = 0;
    int count = 0;
    for (int i = 0; i < g_max_clients; i++) {
        if (g_clients[i].fd < 0) continue;
        if (!g_clients[i].enrolled) continue;
        count++;
        int written = snprintf(buf + off, sizeof(buf) - off,
                               "%s%s",
                               (written_any == 0 ? "" : ","),
                               g_clients[i].username);
        if (written < 0 || (size_t)written >= sizeof(buf) - off) break;
        off += written;
        written_any = 1;
    }
    send_line(c->fd, buf);
    log_event(g_logfp, "ROSTER professor=%s clients=%d", c->username, count);
}

static void handle_command(int idx, char *line,
                           int *should_close, const char **close_reason)
{
    client_t *c = &g_clients[idx];
    *should_close = 0;
    *close_reason = NULL;

    if (line[0] == '\0') return;

    char *cmd = strtok(line, " \t");
    if (!cmd) return;

    if (strcmp(cmd, "APPARATE") == 0) {
        send_line(c->fd, "OK APPARATE");
        *should_close = 1;
        *close_reason = "APPARATE";
        return;
    }

    if (strcmp(cmd, "ENROLL") == 0) {
        if (c->enrolled) {
            send_line(c->fd, "ERR ENROLL already_enrolled");
            return;
        }
        char *role  = strtok(NULL, " \t");
        char *uname = strtok(NULL, " \t");
        if (!role || !uname) {
            send_line(c->fd, "ERR ENROLL bad_format"); return;
        }
        if (strcmp(role, "WIZARD") != 0 && strcmp(role, "PROFESSOR") != 0) {
            send_line(c->fd, "ERR ENROLL bad_role"); return;
        }
        if (strlen(uname) == 0 || strlen(uname) >= MAX_USERNAME_LEN) {
            send_line(c->fd, "ERR ENROLL bad_format"); return;
        }
        if (username_taken(uname, idx)) {
            send_line(c->fd, "ERR ENROLL name_taken"); return;
        }
        strncpy(c->type, role, MAX_TYPE_LEN - 1);
        c->type[MAX_TYPE_LEN - 1] = '\0';
        strncpy(c->username, uname, MAX_USERNAME_LEN - 1);
        c->username[MAX_USERNAME_LEN - 1] = '\0';
        c->enrolled = 1;

        char resp[128];
        snprintf(resp, sizeof(resp), "OK ENROLL %s", c->username);
        send_line(c->fd, resp);
        log_event(g_logfp, "ENROLL username=%s type=%s fd=%d",
                  c->username, c->type, c->fd);
        return;
    }

    if (!c->enrolled) {
        send_line(c->fd, "ERR NOT_ENROLLED");
        return;
    }

    int is_wizard = (strcmp(c->type, "WIZARD") == 0);
    int is_prof   = (strcmp(c->type, "PROFESSOR") == 0);

    if (strcmp(cmd, "BREW") == 0) {
        if (!is_wizard) { send_line(c->fd, "ERR UNAUTHORIZED"); return; }
        cmd_brew(c);
        return;
    }
    if (strcmp(cmd, "CONSUME") == 0) {
        if (!is_wizard) { send_line(c->fd, "ERR UNAUTHORIZED"); return; }
        cmd_consume(c);
        return;
    }
    if (strcmp(cmd, "SPELLBOOK") == 0) {
        if (!is_wizard) { send_line(c->fd, "ERR UNAUTHORIZED"); return; }
        cmd_spellbook(c);
        return;
    }

    if (strcmp(cmd, "INSPECT") == 0) {
        if (!is_prof) { send_line(c->fd, "ERR UNAUTHORIZED"); return; }
        cmd_inspect(c);
        return;
    }
    if (strcmp(cmd, "SCROLL") == 0) {
        if (!is_prof) { send_line(c->fd, "ERR UNAUTHORIZED"); return; }
        cmd_scroll(c);
        return;
    }
    if (strcmp(cmd, "ROSTER") == 0) {
        if (!is_prof) { send_line(c->fd, "ERR UNAUTHORIZED"); return; }
        cmd_roster(c);
        return;
    }

    char resp[128];
    snprintf(resp, sizeof(resp), "ERR UNKNOWN %s", cmd);
    send_line(c->fd, resp);
}

static void handle_client_input(int idx){
    client_t *c = &g_clients[idx];

    size_t space = MAX_LINE_LEN - c->line_len;
    if (space == 0) {
        send_line(c->fd, "ERR TOOLONG");
        remove_client(idx, "line_too_long");
        return;
    }

    ssize_t n = read(c->fd, c->line_buf + c->line_len, space);
    if (n < 0) {
        if (errno == EINTR) return;
        remove_client(idx, "read_error");
        return;
    }
    if (n == 0) {
        remove_client(idx, "hangup");
        return;
    }
    c->line_len += (size_t)n;
    c->last_active = time(NULL);

    for (;;) {
        char *nl = memchr(c->line_buf, '\n', c->line_len);
        if (!nl) {
            if (c->line_len == MAX_LINE_LEN) {
                send_line(c->fd, "ERR TOOLONG");
                remove_client(idx, "line_too_long");
                return;
            }
            break; 
        }

        size_t line_chars = (size_t)(nl - c->line_buf);  

        char tmp[MAX_LINE_LEN + 1];
        memcpy(tmp, c->line_buf, line_chars);
        tmp[line_chars] = '\0';
        if (line_chars > 0 && tmp[line_chars - 1] == '\r') {
            tmp[line_chars - 1] = '\0';
        }

        size_t consumed = line_chars + 1; 
        size_t remain = c->line_len - consumed;
        if (remain > 0) memmove(c->line_buf, c->line_buf + consumed, remain);
        c->line_len = remain;

        int should_close = 0;
        const char *reason = NULL;
        handle_command(idx, tmp, &should_close, &reason);
        if (should_close) {
            remove_client(idx, reason ? reason : "APPARATE");
            return;
        }
    }
}

static void check_idle_timeouts(int *min_remain){
    time_t now = time(NULL);
    int min_r = g_timeout_sec; 

    for (int i = 0; i < g_max_clients; i++) {
        if (g_clients[i].fd < 0) continue;

        long elapsed = (long)(now - g_clients[i].last_active);
        if (elapsed < 0) elapsed = 0;  

        if (elapsed >= g_timeout_sec) {
            log_event(g_logfp, "TIMEOUT username=%s fd=%d elapsed=%lds",
                      g_clients[i].enrolled ? g_clients[i].username : "-",
                      g_clients[i].fd, elapsed);
            send_line(g_clients[i].fd, "TIMEOUT DISCONNECT");
            remove_client(i, "timeout");
            continue;
        }

        int r = g_timeout_sec - (int)elapsed;
        if (r < min_r) min_r = r;
    }

    if (min_r < 1) min_r = 1; 
    *min_remain = min_r;
}

static int create_listen_socket(int port, int backlog){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt"); close(fd); return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, backlog) < 0) {
        perror("listen"); close(fd); return -1;
    }
    return fd;
}

static void accept_new_connection(int listen_fd){
    struct sockaddr_in caddr;
    socklen_t clen = sizeof(caddr);
    int cfd = accept(listen_fd, (struct sockaddr *)&caddr, &clen);
    if (cfd < 0) {
        if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            perror("accept");
        return;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &caddr.sin_addr, ip_str, sizeof(ip_str));

    int slot = find_free_slot();
    if (slot < 0) {
        send_line(cfd, "ERR HOGWARTS_FULL");
        log_event(g_logfp, "REJECTED fd=%d ip=%s reason=HOGWARTS_FULL",
                  cfd, ip_str);
        close(cfd);
        return;
    }

    reset_client_slot(slot);
    g_clients[slot].fd = cfd;
    g_clients[slot].last_active = time(NULL);
    strncpy(g_clients[slot].ip_str, ip_str, INET_ADDRSTRLEN - 1);
    g_clients[slot].ip_str[INET_ADDRSTRLEN - 1] = '\0';

    log_event(g_logfp, "CLIENT_CONNECTED fd=%d ip=%s", cfd, ip_str);
}

static void run_server(int listen_fd){
    while (!g_shutdown_flag) {
        int remain;
        check_idle_timeouts(&remain);

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listen_fd, &rfds);
        int maxfd = listen_fd;
        for (int i = 0; i < g_max_clients; i++) {
            int fd = g_clients[i].fd;
            if (fd >= 0) {
                FD_SET(fd, &rfds);
                if (fd > maxfd) maxfd = fd;
            }
        }

        struct timeval tv = { .tv_sec = remain, .tv_usec = 0 };

        int n = select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (n < 0) {
            if (errno == EINTR) continue; 
            perror("select"); break;
        }
        if (n == 0) continue; 

        if (FD_ISSET(listen_fd, &rfds)) {
            accept_new_connection(listen_fd);
        }

        for (int i = 0; i < g_max_clients; i++) {
            int fd = g_clients[i].fd;
            if (fd < 0) continue;
            if (!FD_ISSET(fd, &rfds)) continue;
            handle_client_input(i);
        }
    }

    for (int i = 0; i < g_max_clients; i++) {
        if (g_clients[i].fd >= 0) {
            send_line(g_clients[i].fd, "SERVER SHUTDOWN");
            close(g_clients[i].fd);
            reset_client_slot(i);
        }
    }
    log_event(g_logfp, "SHUTDOWN signal=SIGINT");
    log_event(g_logfp, "CLEANUP_DONE clients=0");
}

int main(int argc, char *argv[]){
    config_t cfg;
    if (parse_args(argc, argv, &cfg) < 0) return 1;

    g_logfp = fopen(cfg.logfile_path, "a");
    if (!g_logfp) {
        fprintf(stderr, "Error: cannot open log file '%s': %s\n",
                cfg.logfile_path, strerror(errno));
        return 1;
    }

    int n_ing = load_ingredients(cfg.ingredients_path);
    if (n_ing < 0) { fclose(g_logfp); return 1; }

    g_max_clients = cfg.max_clients;
    g_timeout_sec = cfg.timeout_sec;
    g_clients = calloc((size_t)g_max_clients, sizeof(client_t));
    if (!g_clients) {
        fprintf(stderr, "Error: out of memory.\n");
        fclose(g_logfp); return 1;
    }
    for (int i = 0; i < g_max_clients; i++) reset_client_slot(i);

    if (install_sigint_handler() < 0) {
        free(g_clients); fclose(g_logfp); return 1;
    }

    int listen_fd = create_listen_socket(cfg.port, cfg.max_clients * 2);
    if (listen_fd < 0) {
        free(g_clients); fclose(g_logfp); return 1;
    }

    log_event(g_logfp,
              "SERVER_STARTED port=%d max_clients=%d timeout=%d ingredients=%d",
              cfg.port, cfg.max_clients, cfg.timeout_sec, n_ing);
    printf("Hogwarts is ready. Port: %d | Max Clients: %d | Timeout: %ds\n",
           cfg.port, cfg.max_clients, cfg.timeout_sec);
    fflush(stdout);

    run_server(listen_fd);

    close(listen_fd);
    free(g_clients);
    fclose(g_logfp);
    return 0;
}