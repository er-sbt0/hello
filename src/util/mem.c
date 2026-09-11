#include "util/mem.h"

#include "util/log.h"

#include <stdlib.h>
#include <string.h>

static MemStats g_mem;

static void mem_track_alloc(size_t size)
{
	g_mem.total_allocs++;
	g_mem.live_blocks++;
	g_mem.bytes_requested += size;
	if (g_mem.live_blocks > g_mem.peak_blocks)
		g_mem.peak_blocks = g_mem.live_blocks;
}

static void mem_track_free(void)
{
	g_mem.total_frees++;
	if (g_mem.live_blocks > 0)
		g_mem.live_blocks--;
}

static void mem_fail(size_t size)
{
	LOG_FATAL("out of memory requesting %zu bytes", size);
	abort();
}

void *mem_alloc(size_t size)
{
	void *ptr = malloc(size ? size : 1);
	if (ptr == NULL)
		mem_fail(size);
	mem_track_alloc(size);
	return ptr;
}

void *mem_calloc(size_t count, size_t size)
{
	void *ptr = calloc(count ? count : 1, size ? size : 1);
	if (ptr == NULL)
		mem_fail(count * size);
	mem_track_alloc(count * size);
	return ptr;
}

void *mem_realloc(void *ptr, size_t old_size, size_t new_size)
{
	void *next = realloc(ptr, new_size ? new_size : 1);
	if (next == NULL)
		mem_fail(new_size);
	if (ptr == NULL)
		mem_track_alloc(new_size);
	else if (new_size > old_size)
		g_mem.bytes_requested += new_size - old_size;
	return next;
}

void mem_free(void *ptr)
{
	if (ptr == NULL)
		return;
	mem_track_free();
	free(ptr);
}

void mem_stats(MemStats *out)
{
	if (out != NULL)
		*out = g_mem;
}

void mem_reset_stats(void)
{
	memset(&g_mem, 0, sizeof(g_mem));
}

int mem_has_leaks(void)
{
	return g_mem.live_blocks != 0;
}

void mem_report(const char *tag)
{
	MemStats stats;
	mem_stats(&stats);
	LOG_INFO("mem[%s]: allocs=%zu frees=%zu live=%zu peak=%zu bytes=%zu",
		 tag ? tag : "-", stats.total_allocs, stats.total_frees,
		 stats.live_blocks, stats.peak_blocks, stats.bytes_requested);
	if (mem_has_leaks())
		LOG_WARN("mem[%s]: %zu blocks still live", tag ? tag : "-",
			 stats.live_blocks);
}
