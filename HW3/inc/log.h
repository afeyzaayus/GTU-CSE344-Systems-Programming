#ifndef LOG_H
#define LOG_H

#include <unistd.h>

/*
 * write() POSIX standardında atomic (PIPE_BUF sınırına kadar).
 * printf/fprintf ise internal buffer kullandığı için
 * fork sonrası process'ler arası karışabilir.
 *
 * log_msg  → sabit string yaz (stdout)
 * log_fmt  → formatlı string yaz (stdout) — snprintf + write
 * log_err  → stderr'e yaz
 */

void log_msg(const char *msg);
void log_fmt(const char *fmt, ...);
void log_err(const char *fmt, ...);

#endif