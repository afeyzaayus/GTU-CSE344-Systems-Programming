#ifndef ORDER_H
#define ORDER_H

#include <stddef.h>

#define MAX_NAME_LEN 32
#define NAME_BUF_SIZE (MAX_NAME_LEN + 1)
#define SIM_UNIT_MS 500

typedef enum {
    PRIO_EXPRESS  = 1, 
    PRIO_STANDARD = 2,
    PRIO_ECONOMY  = 3   
} Priority;

typedef struct {
    int      id;
    char     name[NAME_BUF_SIZE];
    Priority priority;
    int      duration;  
} Order;

int priority_from_string(const char *s, Priority *out);

const char *priority_to_string(Priority p);

#endif 