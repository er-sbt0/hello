#ifndef CORE_TASK_H
#define CORE_TASK_H

#include <stddef.h>

typedef enum TaskStatus {
	TASK_PENDING = 0,
	TASK_RUNNING,
	TASK_DONE,
	TASK_FAILED,
	TASK_SKIPPED,
	TASK_STATUS_COUNT
} TaskStatus;

typedef struct Task Task;
typedef int (*TaskFn)(Task *self, void *ctx);

struct Task {
	char *name;
	char *description;
	int priority;
	TaskStatus status;
	TaskFn fn;
	void *user;
	int last_result;
	unsigned runs;
	unsigned failures;
};

Task *task_new(const char *name, int priority, TaskFn fn, void *user);
void task_free(Task *t);
void task_free_item(void *item);
int task_run(Task *t, void *ctx);
void task_reset(Task *t);
void task_set_priority(Task *t, int priority);
void task_set_description(Task *t, const char *description);
int task_is_done(const Task *t);
const char *task_status_name(TaskStatus status);
int task_compare_priority(const void *a, const void *b);
int task_compare_name(const void *a, const void *b);
void task_describe(const Task *t);

#endif /* CORE_TASK_H */
