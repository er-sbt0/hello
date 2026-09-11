#include "ds/list.h"

#include "util/log.h"
#include "util/mem.h"

struct ListNode {
	void *item;
	ListNode *next;
	ListNode *prev;
};

struct List {
	ListNode *head;
	ListNode *tail;
	size_t len;
};

static ListNode *node_new(void *item)
{
	ListNode *node = mem_calloc(1, sizeof(ListNode));
	node->item = item;
	return node;
}

static void node_unlink(List *l, ListNode *node)
{
	if (node->prev != NULL)
		node->prev->next = node->next;
	else
		l->head = node->next;

	if (node->next != NULL)
		node->next->prev = node->prev;
	else
		l->tail = node->prev;

	l->len--;
}

List *list_new(void)
{
	return mem_calloc(1, sizeof(List));
}

void list_free(List *l)
{
	list_free_items(l, NULL);
}

void list_free_items(List *l, ListFreeFn free_item)
{
	ListNode *node;
	ListNode *next;

	if (l == NULL)
		return;
	for (node = l->head; node != NULL; node = next) {
		next = node->next;
		if (free_item != NULL)
			free_item(node->item);
		mem_free(node);
	}
	mem_free(l);
}

int list_push_front(List *l, void *item)
{
	ListNode *node;

	if (l == NULL)
		return 0;
	node = node_new(item);
	node->next = l->head;
	if (l->head != NULL)
		l->head->prev = node;
	else
		l->tail = node;
	l->head = node;
	l->len++;
	return 1;
}

int list_push_back(List *l, void *item)
{
	ListNode *node;

	if (l == NULL)
		return 0;
	node = node_new(item);
	node->prev = l->tail;
	if (l->tail != NULL)
		l->tail->next = node;
	else
		l->head = node;
	l->tail = node;
	l->len++;
	return 1;
}

void *list_pop_front(List *l)
{
	ListNode *node;
	void *item;

	if (l == NULL || l->head == NULL)
		return NULL;
	node = l->head;
	item = node->item;
	node_unlink(l, node);
	mem_free(node);
	return item;
}

void *list_pop_back(List *l)
{
	ListNode *node;
	void *item;

	if (l == NULL || l->tail == NULL)
		return NULL;
	node = l->tail;
	item = node->item;
	node_unlink(l, node);
	mem_free(node);
	return item;
}

void *list_front(const List *l)
{
	return (l != NULL && l->head != NULL) ? l->head->item : NULL;
}

void *list_back(const List *l)
{
	return (l != NULL && l->tail != NULL) ? l->tail->item : NULL;
}

size_t list_len(const List *l)
{
	return l != NULL ? l->len : 0;
}

int list_empty(const List *l)
{
	return list_len(l) == 0;
}

void *list_find(const List *l, ListPredFn pred, void *ctx)
{
	const ListNode *node;

	if (l == NULL || pred == NULL)
		return NULL;
	for (node = l->head; node != NULL; node = node->next) {
		if (pred(node->item, ctx))
			return node->item;
	}
	return NULL;
}

int list_remove_if(List *l, ListPredFn pred, void *ctx, ListFreeFn free_item)
{
	ListNode *node;
	ListNode *next;
	int removed = 0;

	if (l == NULL || pred == NULL)
		return 0;
	for (node = l->head; node != NULL; node = next) {
		next = node->next;
		if (!pred(node->item, ctx))
			continue;
		node_unlink(l, node);
		if (free_item != NULL)
			free_item(node->item);
		mem_free(node);
		removed++;
	}
	if (removed > 0)
		LOG_DEBUG("list %p removed %d nodes", (const void *)l, removed);
	return removed;
}

void list_foreach(const List *l, ListVisitFn fn, void *ctx)
{
	const ListNode *node;

	if (l == NULL || fn == NULL)
		return;
	for (node = l->head; node != NULL; node = node->next)
		fn(node->item, ctx);
}
