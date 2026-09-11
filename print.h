#ifndef PRINT_H
#define PRINT_H

#include <stddef.h>

void print_hello(void);
void print_greeting(const char *who);
void print_banner(const char *title);
void print_rule(char ch, size_t width);
void print_kv(const char *key, const char *value);
void print_kv_int(const char *key, long value);
void print_bullet(const char *text);
void print_indented(int depth, const char *text);
void print_columns(const char *left, const char *right, size_t width);
size_t print_line_count(void);

#endif /* PRINT_H */
