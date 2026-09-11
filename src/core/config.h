#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#include "ds/vec.h"

#include <stddef.h>

typedef struct Config Config;

Config *config_new(void);
Config *config_load_string(const char *text);
Config *config_load_file(const char *path);
void config_free(Config *c);
int config_set(Config *c, const char *key, const char *value);
const char *config_get(const Config *c, const char *key, const char *fallback);
int config_get_int(const Config *c, const char *key, int fallback);
int config_get_bool(const Config *c, const char *key, int fallback);
int config_has(const Config *c, const char *key);
size_t config_count(const Config *c);
Vec *config_keys(const Config *c);
void config_dump(const Config *c);
unsigned config_error_count(const Config *c);

#endif /* CORE_CONFIG_H */
