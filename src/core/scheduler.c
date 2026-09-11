#include "core/scheduler.h"

#include "ds/list.h"
#include "ds/vec.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/str.h"

struct Scheduler {
	Vec *tasks;
	List *history;
	EventBus *bus;
	const Config *cfg;
	unsigned ticks;
	int stop_on_failure;
	int max_failures;
};

static void scheduler_record(Scheduler *s, Task *t)
{
	list_push_back(s->history, t);
	if (list_len(s->history) > 32)
		list_pop_front(s->history);
}

static void scheduler_announce(Scheduler *s, EventKind kind, const Task *t,
			       int code)
{
	event_emitf(s->bus, kind, "scheduler", t != NULL ? t->name : "-", code);
}

static int scheduler_should_stop(const Scheduler *s, const SchedulerStats *stats)
{
	if (s->stop_on_failure && stats->failed > 0)
		return 1;
	return s->max_failures >= 0 && (int)stats->failed > s->max_failures;
}

static int task_name_matches(const void *item, void *ctx)
{
	const Task *t = item;
	return t != NULL && str_eq(t->name, ctx);
}

static void scheduler_count_status(void *item, void *ctx)
{
	const Task *t = item;
	SchedulerStats *stats = ctx;

	if (t == NULL || stats == NULL)
		return;
	stats->total++;
	switch (t->status) {
	case TASK_DONE:
		stats->done++;
		break;
	case TASK_FAILED:
		stats->failed++;
		break;
	case TASK_SKIPPED:
		stats->skipped++;
		break;
	default:
		break;
	}
}

Scheduler *scheduler_new(EventBus *bus, const Config *cfg)
{
	Scheduler *s = mem_calloc(1, sizeof(Scheduler));

	s->tasks = vec_new();
	s->history = list_new();
	s->bus = bus;
	s->cfg = cfg;
	s->stop_on_failure = config_get_bool(cfg, "scheduler.stop_on_failure", 0);
	s->max_failures = config_get_int(cfg, "scheduler.max_failures", -1);
	LOG_INFO("scheduler created (stop_on_failure=%d max_failures=%d)",
		 s->stop_on_failure, s->max_failures);
	return s;
}

void scheduler_free(Scheduler *s)
{
	if (s == NULL)
		return;
	list_free(s->history);
	vec_free_items(s->tasks, task_free_item);
	mem_free(s);
}

int scheduler_add(Scheduler *s, Task *t)
{
	if (s == NULL || t == NULL)
		return 0;
	if (scheduler_find(s, t->name) != NULL) {
		LOG_WARN("scheduler already has a task named '%s'", t->name);
		return 0;
	}
	vec_push(s->tasks, t);
	LOG_DEBUG("scheduler registered '%s' (prio %d)", t->name, t->priority);
	return 1;
}

Task *scheduler_add_fn(Scheduler *s, const char *name, int priority, TaskFn fn,
		       void *user)
{
	Task *t = task_new(name, priority, fn, user);

	if (!scheduler_add(s, t)) {
		task_free(t);
		return NULL;
	}
	return t;
}

Task *scheduler_find(const Scheduler *s, const char *name)
{
	size_t i;

	if (s == NULL || name == NULL)
		return NULL;
	for (i = 0; i < vec_len(s->tasks); i++) {
		Task *t = vec_get(s->tasks, i);
		if (task_name_matches(t, (void *)name))
			return t;
	}
	return NULL;
}

size_t scheduler_count(const Scheduler *s)
{
	return s != NULL ? vec_len(s->tasks) : 0;
}

int scheduler_run_one(Scheduler *s, const char *name, void *ctx)
{
	Task *t = scheduler_find(s, name);
	int result;

	if (t == NULL) {
		LOG_ERROR("no such task '%s'", name);
		return -1;
	}

	scheduler_announce(s, EVENT_TASK_BEGIN, t, 0);
	result = task_run(t, ctx);
	scheduler_announce(s, EVENT_TASK_END, t, result);
	if (result != 0)
		scheduler_announce(s, EVENT_ERROR, t, result);
	scheduler_record(s, t);
	return result;
}

int scheduler_run_all(Scheduler *s, void *ctx)
{
	SchedulerStats stats;
	size_t i;
	int failures = 0;

	if (s == NULL)
		return -1;

	vec_sort(s->tasks, task_compare_priority);
	scheduler_announce(s, EVENT_STARTUP, NULL, (int)scheduler_count(s));

	for (i = 0; i < vec_len(s->tasks); i++) {
		Task *t = vec_get(s->tasks, i);
		if (scheduler_run_one(s, t->name, ctx) != 0)
			failures++;

		scheduler_stats(s, &stats);
		if (scheduler_should_stop(s, &stats)) {
			LOG_WARN("scheduler stopping early after %zu failures",
				 stats.failed);
			break;
		}
	}

	s->ticks++;
	return failures;
}

int scheduler_tick(Scheduler *s, void *ctx)
{
	size_t i;
	int ran = 0;

	if (s == NULL)
		return 0;
	for (i = 0; i < vec_len(s->tasks); i++) {
		Task *t = vec_get(s->tasks, i);
		if (t->status != TASK_PENDING)
			continue;
		scheduler_run_one(s, t->name, ctx);
		ran++;
	}
	s->ticks++;
	return ran;
}

void scheduler_stats(const Scheduler *s, SchedulerStats *out)
{
	if (out == NULL)
		return;
	out->total = 0;
	out->done = 0;
	out->failed = 0;
	out->skipped = 0;
	out->ticks = s != NULL ? s->ticks : 0;
	if (s != NULL)
		vec_foreach(s->tasks, scheduler_count_status, out);
}

void scheduler_reset(Scheduler *s)
{
	size_t i;

	if (s == NULL)
		return;
	for (i = 0; i < vec_len(s->tasks); i++)
		task_reset(vec_get(s->tasks, i));
	LOG_INFO("scheduler reset %zu tasks", scheduler_count(s));
}

static void scheduler_describe_task(void *item, void *ctx)
{
	(void)ctx;
	task_describe(item);
}

void scheduler_report(const Scheduler *s)
{
	SchedulerStats stats;

	if (s == NULL)
		return;
	scheduler_stats(s, &stats);
	LOG_INFO("scheduler: %zu tasks, %zu done, %zu failed, %zu skipped, %u ticks",
		 stats.total, stats.done, stats.failed, stats.skipped, stats.ticks);
	vec_foreach(s->tasks, scheduler_describe_task, NULL);
	LOG_INFO("scheduler history holds %zu entries", list_len(s->history));
}
