#ifndef DS_LIST_H
#define DS_LIST_H

#include <stddef.h>

typedef struct ListNode ListNode;
typedef struct List List;

typedef int (*ListPredFn)(const void *item, void *ctx);
typedef void (*ListVisitFn)(void *item, void *ctx);
typedef void (*ListFreeFn)(void *item);

List *list_new(void);
void list_free(List *l);
void list_free_items(List *l, ListFreeFn free_item);
int list_push_front(List *l, void *item);
int list_push_back(List *l, void *item);
void *list_pop_front(List *l);
void *list_pop_back(List *l);
void *list_front(const List *l);
void *list_back(const List *l);
size_t list_len(const List *l);
int list_empty(const List *l);
void *list_find(const List *l, ListPredFn pred, void *ctx);
int list_remove_if(List *l, ListPredFn pred, void *ctx, ListFreeFn free_item);
void list_foreach(const List *l, ListVisitFn fn, void *ctx);

#endif /* DS_LIST_H */
