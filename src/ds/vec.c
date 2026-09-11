#include "ds/vec.h"

#include "util/log.h"
#include "util/mem.h"

#include <stdlib.h>

#define VEC_MIN_CAPACITY 4

struct Vec {
	void **items;
	size_t len;
	size_t capacity;
};

static int vec_grow(Vec *v, size_t needed)
{
	size_t capacity = v->capacity ? v->capacity : VEC_MIN_CAPACITY;
	void **items;

	while (capacity < needed)
		capacity *= 2;

	items = mem_realloc(v->items, v->capacity * sizeof(void *),
			    capacity * sizeof(void *));
	v->items = items;
	v->capacity = capacity;
	LOG_DEBUG("vec %p grew to capacity %zu", (void *)v, capacity);
	return 1;
}

Vec *vec_new(void)
{
	return vec_with_capacity(VEC_MIN_CAPACITY);
}

Vec *vec_with_capacity(size_t capacity)
{
	Vec *v = mem_calloc(1, sizeof(Vec));
	if (capacity > 0)
		vec_grow(v, capacity);
	return v;
}

void vec_free(Vec *v)
{
	if (v == NULL)
		return;
	mem_free(v->items);
	mem_free(v);
}

void vec_free_items(Vec *v, VecFreeFn free_item)
{
	size_t i;
	if (v == NULL)
		return;
	if (free_item != NULL) {
		for (i = 0; i < v->len; i++)
			free_item(v->items[i]);
	}
	vec_free(v);
}

int vec_reserve(Vec *v, size_t capacity)
{
	if (v == NULL)
		return 0;
	if (capacity <= v->capacity)
		return 1;
	return vec_grow(v, capacity);
}

int vec_push(Vec *v, void *item)
{
	if (v == NULL)
		return 0;
	if (v->len + 1 > v->capacity && !vec_grow(v, v->len + 1))
		return 0;
	v->items[v->len++] = item;
	return 1;
}

void *vec_pop(Vec *v)
{
	if (v == NULL || v->len == 0)
		return NULL;
	return v->items[--v->len];
}

void *vec_get(const Vec *v, size_t index)
{
	if (v == NULL || index >= v->len)
		return NULL;
	return v->items[index];
}

void *vec_first(const Vec *v)
{
	return vec_get(v, 0);
}

void *vec_last(const Vec *v)
{
	if (v == NULL || v->len == 0)
		return NULL;
	return vec_get(v, v->len - 1);
}

int vec_set(Vec *v, size_t index, void *item)
{
	if (v == NULL || index >= v->len)
		return 0;
	v->items[index] = item;
	return 1;
}

size_t vec_len(const Vec *v)
{
	return v != NULL ? v->len : 0;
}

size_t vec_capacity(const Vec *v)
{
	return v != NULL ? v->capacity : 0;
}

int vec_empty(const Vec *v)
{
	return vec_len(v) == 0;
}

void vec_clear(Vec *v)
{
	if (v != NULL)
		v->len = 0;
}

void vec_foreach(const Vec *v, VecVisitFn fn, void *ctx)
{
	size_t i;
	if (v == NULL || fn == NULL)
		return;
	for (i = 0; i < v->len; i++)
		fn(v->items[i], ctx);
}

void vec_sort(Vec *v, VecCmpFn cmp)
{
	if (v == NULL || cmp == NULL || v->len < 2)
		return;
	qsort(v->items, v->len, sizeof(void *), cmp);
}

long vec_index_of(const Vec *v, const void *item)
{
	size_t i;
	if (v == NULL)
		return -1;
	for (i = 0; i < v->len; i++) {
		if (v->items[i] == item)
			return (long)i;
	}
	return -1;
}
