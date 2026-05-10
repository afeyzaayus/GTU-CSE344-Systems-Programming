#include "../inc/parser.h"
#include "../inc/order.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define LINE_BUF_SIZE 1024

#define INITIAL_CAPACITY 16

static int is_valid_name(const char *s){
    if (s == NULL || *s == '\0') return 0;

    size_t len = strlen(s);
    if (len > MAX_NAME_LEN) return 0;

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!isalnum(c) && c != '_') return 0;
    }
    return 1;
}

static int parse_int(const char *s, int *out){
    if (s == NULL || *s == '\0') return -1;

    errno = 0;
    char *endptr = NULL;
    long v = strtol(s, &endptr, 10);

    if (endptr == s || *endptr != '\0') return -1;
    if (errno == ERANGE) return -1;
    if (v < INT_MIN || v > INT_MAX) return -1;

    *out = (int)v;
    return 0;
}

static void rstrip_newline(char *line){
    size_t len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        line[--len] = '\0';
    }
}

static int is_blank(const char *s){
    for (; *s; ++s) {
        if (!isspace((unsigned char)*s)) return 0;
    }
    return 1;
}

static int has_duplicate_id(const Order *orders, size_t count, int id){
    for (size_t i = 0; i < count; ++i) {
        if (orders[i].id == id) return 1;
    }
    return 0;
}

static int parse_one_line(char *line, Order *out){
    char *saveptr = NULL;
    const char *delim = " \t";

    char *tok_id       = strtok_r(line, delim, &saveptr);
    char *tok_name     = strtok_r(NULL, delim, &saveptr);
    char *tok_priority = strtok_r(NULL, delim, &saveptr);
    char *tok_duration = strtok_r(NULL, delim, &saveptr);
    char *tok_extra    = strtok_r(NULL, delim, &saveptr);

    if (!tok_id || !tok_name || !tok_priority || !tok_duration) return -1;
    if (tok_extra != NULL) return -1;

    int id;
    if (parse_int(tok_id, &id) != 0) return -1;
    if (id < 0) return -1;

    if (!is_valid_name(tok_name)) return -1;

    Priority prio;
    if (priority_from_string(tok_priority, &prio) != 0) return -1;

    int duration;
    if (parse_int(tok_duration, &duration) != 0) return -1;
    if (duration <= 0) return -1;

    out->id       = id;
    out->priority = prio;
    out->duration = duration;
    strncpy(out->name, tok_name, MAX_NAME_LEN);
    out->name[MAX_NAME_LEN] = '\0';

    return 0;
}

int parse_orders_file(const char *path, Order **out_orders, size_t *out_count){
    if (out_orders == NULL || out_count == NULL) return -1;
    *out_orders = NULL;
    *out_count  = 0;

    FILE *fp = fopen(path, "r");
    if (fp == NULL) return -1;

    Order *orders = malloc(sizeof(Order) * INITIAL_CAPACITY);
    if (orders == NULL) {
        fclose(fp);
        return -1;
    }
    size_t capacity = INITIAL_CAPACITY;
    size_t count    = 0;

    char line[LINE_BUF_SIZE];
    while (fgets(line, sizeof(line), fp) != NULL) {
        rstrip_newline(line);

        if (is_blank(line)) continue;

        Order parsed;
        if (parse_one_line(line, &parsed) != 0) {
            continue;
        }

        if (has_duplicate_id(orders, count, parsed.id)) continue;

        if (count == capacity) {
            size_t new_cap = capacity * 2;
            Order *bigger = realloc(orders, sizeof(Order) * new_cap);
            if (bigger == NULL) {
                break;
            }
            orders   = bigger;
            capacity = new_cap;
        }
        orders[count++] = parsed;
    }

    fclose(fp);
    if (count == 0) {
        free(orders);
        *out_orders = NULL;
        *out_count  = 0;
        return 0;
    }

    Order *tight = realloc(orders, sizeof(Order) * count);
    if (tight != NULL) orders = tight;
    *out_orders = orders;
    *out_count  = count;
    return 0;
}