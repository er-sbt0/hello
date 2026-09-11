#ifndef CORE_SCHEDULER_H
#define CORE_SCHEDULER_H

#include "core/config.h"
#include "core/event.h"
#include "core/task.h"

#include <stddef.h>

typedef struct Scheduler Scheduler;

typedef struct SchedulerStats {
	size_t total;
	size_t done;
	size_t failed;
	size_t skipped;
	unsigned ticks;
} SchedulerStats;

Scheduler *scheduler_new(EventBus *bus, const Config *cfg);
void scheduler_free(Scheduler *s);
int scheduler_add(Scheduler *s, Task *t);
Task *scheduler_add_fn(Scheduler *s, const char *name, int priority, TaskFn fn,
		       void *user);
Task *scheduler_find(const Scheduler *s, const char *name);
size_t scheduler_count(const Scheduler *s);
int scheduler_run_all(Scheduler *s, void *ctx);
int scheduler_run_one(Scheduler *s, const char *name, void *ctx);
int scheduler_tick(Scheduler *s, void *ctx);
void scheduler_stats(const Scheduler *s, SchedulerStats *out);
void scheduler_report(const Scheduler *s);
void scheduler_reset(Scheduler *s);

#endif /* CORE_SCHEDULER_H */
