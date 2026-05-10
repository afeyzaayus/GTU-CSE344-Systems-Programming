#include "../inc/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

static void print_usage(const char *prog){
    fprintf(stderr,
            "usage: %s -n <num_couriers> -i <orders.txt> -s <stats.txt>\n"
            "  -n  number of courier threads (>= 1)\n"
            "  -i  path to input orders file\n"
            "  -s  path to statistics output file\n",
            prog);
}

static int parse_positive_int(const char *s, int *out){
    if (s == NULL || *s == '\0') return -1;

    char *endptr = NULL;
    long v = strtol(s, &endptr, 10);

    if (endptr == s || *endptr != '\0') return -1;
    if (v < 1 || v > INT_MAX) return -1;

    *out = (int)v;
    return 0;
}

int parse_cli_args(int argc, char **argv,
                          int *num_couriers,
                          char **input_path,
                          char **stats_path)
{
    *num_couriers = 0;
    *input_path   = NULL;
    *stats_path   = NULL;

    int opt;
    while ((opt = getopt(argc, argv, ":n:i:s:")) != -1) {
        switch (opt) {
            case 'n':
                if (parse_positive_int(optarg, num_couriers) != 0) {
                    fprintf(stderr, "error: -n must be a positive integer\n");
                    print_usage(argv[0]);
                    return -1;
                }
                break;
            case 'i': *input_path = optarg; break;
            case 's': *stats_path = optarg; break;
            case ':':
                fprintf(stderr, "error: option -%c requires an argument\n", optopt);
                print_usage(argv[0]);
                return -1;
            case '?':
            default:
                fprintf(stderr, "error: unknown option -%c\n", optopt);
                print_usage(argv[0]);
                return -1;
        }
    }

    if (*num_couriers < 1 || *input_path == NULL || *stats_path == NULL) {
        fprintf(stderr, "error: -n, -i, and -s are all required\n");
        print_usage(argv[0]);
        return -1;
    }

    if (optind != argc) {
        fprintf(stderr, "error: unexpected positional argument: %s\n", argv[optind]);
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}