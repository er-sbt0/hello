#ifndef UTIL_STR_H
#define UTIL_STR_H

#include "ds/vec.h"

#include <stddef.h>

char *str_dup(const char *s);
char *str_ndup(const char *s, size_t n);
size_t str_len(const char *s);
int str_eq(const char *a, const char *b);
int str_ieq(const char *a, const char *b);
int str_starts_with(const char *s, const char *prefix);
int str_ends_with(const char *s, const char *suffix);
int str_empty(const char *s);
unsigned long str_hash(const char *s);
char *str_trim(const char *s);
char *str_to_lower(const char *s);
char *str_to_upper(const char *s);
char *str_repeat(char ch, size_t count);
Vec *str_split(const char *s, char sep);
char *str_join(const Vec *parts, const char *sep);
void str_vec_free(Vec *parts);
int str_parse_int(const char *s, int *out);
int str_parse_bool(const char *s, int *out);

#endif /* UTIL_STR_H */
