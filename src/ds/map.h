#ifndef DS_MAP_H
#define DS_MAP_H

#include "ds/vec.h"

#include <stddef.h>

typedef struct Map Map;

typedef void (*MapVisitFn)(const char *key, void *value, void *ctx);
typedef void (*MapFreeFn)(void *value);

Map *map_new(void);
Map *map_with_buckets(size_t buckets);
void map_free(Map *m);
void map_free_values(Map *m, MapFreeFn free_value);
int map_put(Map *m, const char *key, void *value);
void *map_get(const Map *m, const char *key);
int map_has(const Map *m, const char *key);
void *map_remove(Map *m, const char *key);
size_t map_size(const Map *m);
size_t map_bucket_count(const Map *m);
double map_load_factor(const Map *m);
Vec *map_keys(const Map *m);
void map_foreach(const Map *m, MapVisitFn fn, void *ctx);
void map_dump(const Map *m, const char *tag);

#endif /* DS_MAP_H */
