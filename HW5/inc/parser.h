#ifndef PARSER_H
#define PARSER_H

#include "order.h"
#include <stddef.h>

int parse_orders_file(const char *path, Order **out_orders, size_t *out_count);
int parse_cli_args(int argc, char **argv,
                          int *num_couriers,
                          char **input_path,
                          char **stats_path);

#endif 