#ifndef DS_VEC_H
#define DS_VEC_H

#include <stddef.h>

typedef struct Vec Vec;

typedef void (*VecVisitFn)(void *item, void *ctx);
typedef void (*VecFreeFn)(void *item);
typedef int (*VecCmpFn)(const void *a, const void *b);

Vec *vec_new(void);
Vec *vec_with_capacity(size_t capacity);
void vec_free(Vec *v);
void vec_free_items(Vec *v, VecFreeFn free_item);
int vec_push(Vec *v, void *item);
void *vec_pop(Vec *v);
void *vec_get(const Vec *v, size_t index);
void *vec_first(const Vec *v);
void *vec_last(const Vec *v);
int vec_set(Vec *v, size_t index, void *item);
size_t vec_len(const Vec *v);
size_t vec_capacity(const Vec *v);
int vec_empty(const Vec *v);
int vec_reserve(Vec *v, size_t capacity);
void vec_clear(Vec *v);
void vec_foreach(const Vec *v, VecVisitFn fn, void *ctx);
void vec_sort(Vec *v, VecCmpFn cmp);
long vec_index_of(const Vec *v, const void *item);

#endif /* DS_VEC_H */
