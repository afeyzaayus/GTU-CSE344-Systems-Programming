#ifndef LOG_H
#define LOG_H

void log_init(void);
void log_destroy(void);

void log_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

#endif 