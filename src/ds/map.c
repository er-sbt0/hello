#include "ds/map.h"

#include "ds/list.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/str.h"

#define MAP_DEFAULT_BUCKETS 16
#define MAP_MAX_LOAD 0.75

typedef struct MapEntry {
	char *key;
	void *value;
	unsigned long hash;
} MapEntry;

struct Map {
	List **buckets;
	size_t bucket_count;
	size_t size;
};

static MapEntry *entry_new(const char *key, void *value)
{
	MapEntry *entry = mem_calloc(1, sizeof(MapEntry));
	entry->key = str_dup(key);
	entry->value = value;
	entry->hash = str_hash(key);
	return entry;
}

static void entry_free(void *item)
{
	MapEntry *entry = item;
	if (entry == NULL)
		return;
	mem_free(entry->key);
	mem_free(entry);
}

static int entry_matches(const void *item, void *ctx)
{
	const MapEntry *entry = item;
	const char *key = ctx;
	return entry != NULL && str_eq(entry->key, key);
}

static size_t map_index(const Map *m, const char *key)
{
	return (size_t)(str_hash(key) % m->bucket_count);
}

static List *map_bucket(const Map *m, const char *key)
{
	return m->buckets[map_index(m, key)];
}

static MapEntry *map_lookup(const Map *m, const char *key)
{
	if (m == NULL || key == NULL)
		return NULL;
	return list_find(map_bucket(m, key), entry_matches, (void *)key);
}

static void map_rehash(Map *m)
{
	List **old_buckets = m->buckets;
	size_t old_count = m->bucket_count;
	size_t new_count = old_count * 2;
	size_t i;

	m->buckets = mem_calloc(new_count, sizeof(List *));
	m->bucket_count = new_count;
	for (i = 0; i < new_count; i++)
		m->buckets[i] = list_new();

	for (i = 0; i < old_count; i++) {
		MapEntry *entry;
		while ((entry = list_pop_front(old_buckets[i])) != NULL)
			list_push_front(map_bucket(m, entry->key), entry);
		list_free(old_buckets[i]);
	}
	mem_free(old_buckets);
	LOG_DEBUG("map %p rehashed %zu -> %zu buckets", (void *)m, old_count,
		  new_count);
}

Map *map_new(void)
{
	return map_with_buckets(MAP_DEFAULT_BUCKETS);
}

Map *map_with_buckets(size_t buckets)
{
	Map *m = mem_calloc(1, sizeof(Map));
	size_t i;

	m->bucket_count = buckets > 0 ? buckets : MAP_DEFAULT_BUCKETS;
	m->buckets = mem_calloc(m->bucket_count, sizeof(List *));
	for (i = 0; i < m->bucket_count; i++)
		m->buckets[i] = list_new();
	return m;
}

void map_free(Map *m)
{
	map_free_values(m, NULL);
}

void map_free_values(Map *m, MapFreeFn free_value)
{
	size_t i;

	if (m == NULL)
		return;
	for (i = 0; i < m->bucket_count; i++) {
		if (free_value != NULL) {
			MapEntry *entry;
			for (entry = list_front(m->buckets[i]); entry != NULL;
			     entry = list_front(m->buckets[i])) {
				free_value(entry->value);
				list_pop_front(m->buckets[i]);
				entry_free(entry);
			}
		}
		list_free_items(m->buckets[i], entry_free);
	}
	mem_free(m->buckets);
	mem_free(m);
}

int map_put(Map *m, const char *key, void *value)
{
	MapEntry *entry;

	if (m == NULL || key == NULL)
		return 0;

	entry = map_lookup(m, key);
	if (entry != NULL) {
		entry->value = value;
		return 1;
	}

	if (map_load_factor(m) >= MAP_MAX_LOAD)
		map_rehash(m);

	list_push_front(map_bucket(m, key), entry_new(key, value));
	m->size++;
	return 1;
}

void *map_get(const Map *m, const char *key)
{
	MapEntry *entry = map_lookup(m, key);
	return entry != NULL ? entry->value : NULL;
}

int map_has(const Map *m, const char *key)
{
	return map_lookup(m, key) != NULL;
}

void *map_remove(Map *m, const char *key)
{
	MapEntry *entry = map_lookup(m, key);
	void *value;

	if (entry == NULL)
		return NULL;
	value = entry->value;
	list_remove_if(map_bucket(m, key), entry_matches, (void *)key, entry_free);
	m->size--;
	return value;
}

size_t map_size(const Map *m)
{
	return m != NULL ? m->size : 0;
}

size_t map_bucket_count(const Map *m)
{
	return m != NULL ? m->bucket_count : 0;
}

double map_load_factor(const Map *m)
{
	if (m == NULL || m->bucket_count == 0)
		return 0.0;
	return (double)m->size / (double)m->bucket_count;
}

typedef struct MapVisitCtx {
	MapVisitFn fn;
	void *ctx;
	Vec *keys;
} MapVisitCtx;

static void map_visit_entry(void *item, void *ctx)
{
	MapEntry *entry = item;
	MapVisitCtx *visit = ctx;

	if (entry == NULL || visit == NULL)
		return;
	if (visit->keys != NULL)
		vec_push(visit->keys, entry->key);
	if (visit->fn != NULL)
		visit->fn(entry->key, entry->value, visit->ctx);
}

Vec *map_keys(const Map *m)
{
	MapVisitCtx visit;
	size_t i;

	visit.fn = NULL;
	visit.ctx = NULL;
	visit.keys = vec_new();

	if (m == NULL)
		return visit.keys;
	for (i = 0; i < m->bucket_count; i++)
		list_foreach(m->buckets[i], map_visit_entry, &visit);
	return visit.keys;
}

void map_foreach(const Map *m, MapVisitFn fn, void *ctx)
{
	MapVisitCtx visit;
	size_t i;

	if (m == NULL || fn == NULL)
		return;
	visit.fn = fn;
	visit.ctx = ctx;
	visit.keys = NULL;
	for (i = 0; i < m->bucket_count; i++)
		list_foreach(m->buckets[i], map_visit_entry, &visit);
}

static void map_dump_entry(const char *key, void *value, void *ctx)
{
	const char *tag = ctx;
	LOG_INFO("map[%s] %s = %p", tag ? tag : "-", key, value);
}

void map_dump(const Map *m, const char *tag)
{
	LOG_INFO("map[%s]: %zu entries in %zu buckets (load %.2f)",
		 tag ? tag : "-", map_size(m), map_bucket_count(m),
		 map_load_factor(m));
	map_foreach(m, map_dump_entry, (void *)tag);
}
