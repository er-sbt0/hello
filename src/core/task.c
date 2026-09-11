#include "core/task.h"

#include "util/log.h"
#include "util/mem.h"
#include "util/str.h"

static const char *const g_status_names[TASK_STATUS_COUNT] = {
	"pending", "running", "done", "failed", "skipped"
};

static void task_mark(Task *t, TaskStatus status)
{
	LOG_DEBUG("task '%s': %s -> %s", t->name, task_status_name(t->status),
		  task_status_name(status));
	t->status = status;
}

Task *task_new(const char *name, int priority, TaskFn fn, void *user)
{
	Task *t = mem_calloc(1, sizeof(Task));

	t->name = str_dup(name != NULL ? name : "anonymous");
	t->description = str_dup("");
	t->priority = priority;
	t->fn = fn;
	t->user = user;
	t->status = TASK_PENDING;
	return t;
}

void task_free(Task *t)
{
	if (t == NULL)
		return;
	mem_free(t->name);
	mem_free(t->description);
	mem_free(t);
}

void task_free_item(void *item)
{
	task_free(item);
}

void task_set_description(Task *t, const char *description)
{
	if (t == NULL)
		return;
	mem_free(t->description);
	t->description = str_dup(description != NULL ? description : "");
}

void task_set_priority(Task *t, int priority)
{
	if (t != NULL)
		t->priority = priority;
}

void task_reset(Task *t)
{
	if (t == NULL)
		return;
	task_mark(t, TASK_PENDING);
	t->last_result = 0;
}

int task_run(Task *t, void *ctx)
{
	int result;

	if (t == NULL)
		return -1;
	if (t->fn == NULL) {
		task_mark(t, TASK_SKIPPED);
		LOG_WARN("task '%s' has no callback, skipping", t->name);
		return 0;
	}

	task_mark(t, TASK_RUNNING);
	t->runs++;
	result = t->fn(t, ctx);
	t->last_result = result;

	if (result == 0) {
		task_mark(t, TASK_DONE);
	} else {
		t->failures++;
		task_mark(t, TASK_FAILED);
		LOG_ERROR("task '%s' failed with code %d", t->name, result);
	}
	return result;
}

int task_is_done(const Task *t)
{
	return t != NULL && t->status == TASK_DONE;
}

const char *task_status_name(TaskStatus status)
{
	if (status >= 0 && status < TASK_STATUS_COUNT)
		return g_status_names[status];
	return "unknown";
}

int task_compare_priority(const void *a, const void *b)
{
	const Task *ta = *(const Task *const *)a;
	const Task *tb = *(const Task *const *)b;

	if (ta->priority != tb->priority)
		return tb->priority - ta->priority;
	return task_compare_name(a, b);
}

int task_compare_name(const void *a, const void *b)
{
	const Task *ta = *(const Task *const *)a;
	const Task *tb = *(const Task *const *)b;

	if (str_eq(ta->name, tb->name))
		return 0;
	return str_hash(ta->name) < str_hash(tb->name) ? -1 : 1;
}

void task_describe(const Task *t)
{
	if (t == NULL)
		return;
	LOG_INFO("task %-14s prio=%2d status=%-7s runs=%u failures=%u %s",
		 t->name, t->priority, task_status_name(t->status), t->runs,
		 t->failures, t->description);
}
