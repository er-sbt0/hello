#include "app/commands.h"

#include "app/app.h"
#include "app/report.h"
#include "ds/map.h"
#include "ds/vec.h"
#include "print.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/str.h"

#include <stdint.h>
#include <stdio.h>

typedef struct CommandSpec {
	const char *name;
	int priority;
	TaskFn fn;
	const char *description;
} CommandSpec;

/* --- small math helpers, deliberately recursive --------------------------- */

long math_fib(int n)
{
	if (n < 2)
		return n;
	return math_fib(n - 1) + math_fib(n - 2);
}

long math_gcd(long a, long b)
{
	if (b == 0)
		return a < 0 ? -a : a;
	return math_gcd(b, a % b);
}

int math_is_even(int n)
{
	if (n == 0)
		return 1;
	return math_is_odd(n - 1);
}

int math_is_odd(int n)
{
	if (n == 0)
		return 0;
	return math_is_even(n - 1);
}

int math_collatz_steps(long n)
{
	if (n <= 1)
		return 0;
	if (math_is_even((int)(n % 2) == 0 ? 0 : 1))
		return 1 + math_collatz_steps(n / 2);
	return 1 + math_collatz_steps(3 * n + 1);
}

/* --- task callbacks ------------------------------------------------------- */

int cmd_greet(Task *self, void *ctx)
{
	AppContext *app = ctx;
	const char *who = config_get(app->config, "app.greeting", "world");

	print_bullet("greeting phase");
	print_greeting(who);
	task_set_description(self, "prints the configured greeting");
	app_note(app, "greeted", who);
	return 0;
}

int cmd_dump_config(Task *self, void *ctx)
{
	AppContext *app = ctx;
	Vec *keys = config_keys(app->config);
	size_t i;

	task_set_description(self, "lists every configuration key");
	for (i = 0; i < vec_len(keys); i++) {
		const char *key = vec_get(keys, i);
		print_columns(key, config_get(app->config, key, "?"), 26);
	}
	vec_free(keys);
	config_dump(app->config);
	return 0;
}

static void word_count_tally(void *item, void *ctx)
{
	const char *word = item;
	Map *counts = ctx;
	size_t current;

	if (str_empty(word))
		return;
	current = (size_t)(uintptr_t)map_get(counts, word);
	map_put(counts, word, (void *)(uintptr_t)(current + 1));
}

static void word_count_print(const char *key, void *value, void *ctx)
{
	(void)ctx;
	print_kv_int(key, (long)(uintptr_t)value);
}

int cmd_word_count(Task *self, void *ctx)
{
	AppContext *app = ctx;
	const char *sample = config_get(app->config, "words.sample", "");
	char *lowered = str_to_lower(sample);
	Vec *words = str_split(lowered, ' ');
	Map *counts = map_new();

	task_set_description(self, "counts word frequencies in the sample text");
	vec_foreach(words, word_count_tally, counts);

	print_bullet("word frequencies");
	map_foreach(counts, word_count_print, NULL);
	LOG_INFO("counted %zu distinct words from %zu tokens", map_size(counts),
		 vec_len(words));

	report_add_int(app->report, "distinct words", (long)map_size(counts));
	map_free(counts);
	str_vec_free(words);
	mem_free(lowered);
	return 0;
}

int cmd_fibonacci(Task *self, void *ctx)
{
	AppContext *app = ctx;
	int n = config_get_int(app->config, "fib.n", 12);
	long value = math_fib(n);
	char buf[64];

	task_set_description(self, "computes a fibonacci number recursively");
	snprintf(buf, sizeof(buf), "fib(%d) = %ld", n, value);
	print_bullet(buf);
	report_add_int(app->report, "fibonacci", value);
	return 0;
}

int cmd_parity_scan(Task *self, void *ctx)
{
	AppContext *app = ctx;
	int evens = 0;
	int odds = 0;
	int i;

	task_set_description(self, "mutual recursion parity check");
	for (i = 0; i < 12; i++) {
		if (math_is_even(i))
			evens++;
		if (math_is_odd(i))
			odds++;
	}
	print_kv_int("evens in 0..11", evens);
	print_kv_int("odds in 0..11", odds);
	report_add_int(app->report, "gcd(462,1071)", math_gcd(462, 1071));
	return evens == odds ? 0 : 1;
}

int cmd_collatz(Task *self, void *ctx)
{
	AppContext *app = ctx;
	long best = 0;
	int best_steps = 0;
	long n;

	task_set_description(self, "finds the longest collatz chain under 64");
	for (n = 1; n < 64; n++) {
		int steps = math_collatz_steps(n);
		if (steps > best_steps) {
			best_steps = steps;
			best = n;
		}
	}
	print_kv_int("longest collatz start", best);
	print_kv_int("longest collatz steps", best_steps);
	report_add_int(app->report, "collatz best", best);
	return 0;
}

int cmd_memory_stats(Task *self, void *ctx)
{
	AppContext *app = ctx;
	MemStats stats;

	task_set_description(self, "snapshots allocator counters");
	mem_stats(&stats);
	print_kv_int("allocations", (long)stats.total_allocs);
	print_kv_int("frees", (long)stats.total_frees);
	print_kv_int("live blocks", (long)stats.live_blocks);
	mem_report("mid-run");
	report_add_int(app->report, "live blocks", (long)stats.live_blocks);
	return 0;
}

int cmd_event_stats(Task *self, void *ctx)
{
	AppContext *app = ctx;
	int i;

	task_set_description(self, "summarizes event bus traffic");
	for (i = 0; i < EVENT_KIND_COUNT; i++) {
		EventKind kind = (EventKind)i;
		print_columns(event_kind_name(kind),
			      task_status_name(TASK_DONE), 18);
		LOG_DEBUG("channel %s has %zu subscribers",
			  event_kind_name(kind),
			  event_subscriber_count(app->bus, kind));
	}
	report_add_int(app->report, "event channels", EVENT_KIND_COUNT);
	return 0;
}

int cmd_always_fails(Task *self, void *ctx)
{
	AppContext *app = ctx;

	task_set_description(self, "deliberately fails to exercise error paths");
	LOG_WARN("'%s' is about to fail on purpose", self->name);
	event_emitf(app->bus, EVENT_ERROR, "commands", self->name, 42);
	return 42;
}

/* --- registration --------------------------------------------------------- */

static const CommandSpec g_commands[] = {
	{ "greet",       90, cmd_greet,        "say hello" },
	{ "config-dump", 80, cmd_dump_config,  "print configuration" },
	{ "word-count",  70, cmd_word_count,   "tally sample words" },
	{ "fibonacci",   60, cmd_fibonacci,    "recursive fib" },
	{ "parity",      50, cmd_parity_scan,  "mutual recursion" },
	{ "collatz",     40, cmd_collatz,      "collatz chains" },
	{ "mem-stats",   30, cmd_memory_stats, "allocator counters" },
	{ "event-stats", 20, cmd_event_stats,  "event bus traffic" },
	{ "boom",        10, cmd_always_fails, "always fails" }
};

static Task *cmd_register_one(Scheduler *s, const CommandSpec *spec, void *app)
{
	Task *t = scheduler_add_fn(s, spec->name, spec->priority, spec->fn, app);

	if (t == NULL) {
		LOG_ERROR("could not register command '%s'", spec->name);
		return NULL;
	}
	task_set_description(t, spec->description);
	return t;
}

int cmd_register_all(Scheduler *s, void *app)
{
	size_t i;
	int registered = 0;

	for (i = 0; i < sizeof(g_commands) / sizeof(g_commands[0]); i++) {
		if (cmd_register_one(s, &g_commands[i], app) != NULL)
			registered++;
	}
	LOG_INFO("registered %d of %zu commands", registered,
		 sizeof(g_commands) / sizeof(g_commands[0]));
	return registered;
}
