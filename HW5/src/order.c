#include "../inc/order.h"
#include <string.h>

int priority_from_string(const char *s, Priority *out){
    if (s == NULL || out == NULL) return -1;

    if (strcmp(s, "EXPRESS") == 0) {
        *out = PRIO_EXPRESS;
        return 0;
    }
    if (strcmp(s, "STANDARD") == 0) {
        *out = PRIO_STANDARD;
        return 0;
    }
    if (strcmp(s, "ECONOMY") == 0) {
        *out = PRIO_ECONOMY;
        return 0;
    }
    return -1;
}

const char *priority_to_string(Priority p){
    switch (p) {
        case PRIO_EXPRESS:  return "EXPRESS";
        case PRIO_STANDARD: return "STANDARD";
        case PRIO_ECONOMY:  return "ECONOMY";
    }
    return "UNKNOWN";
}