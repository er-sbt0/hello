#ifndef UTIL_MEM_H
#define UTIL_MEM_H

#include <stddef.h>

typedef struct MemStats {
	size_t live_blocks;
	size_t total_allocs;
	size_t total_frees;
	size_t bytes_requested;
	size_t peak_blocks;
} MemStats;

void *mem_alloc(size_t size);
void *mem_calloc(size_t count, size_t size);
void *mem_realloc(void *ptr, size_t old_size, size_t new_size);
void mem_free(void *ptr);
void mem_stats(MemStats *out);
void mem_reset_stats(void);
void mem_report(const char *tag);
int mem_has_leaks(void);

#endif /* UTIL_MEM_H */
