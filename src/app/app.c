#include "app/app.h"

#include "app/commands.h"
#include "print.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/str.h"

static const char *const DEFAULT_CONFIG =
	"# demo configuration\n"
	"app.name = source-explorer-demo\n"
	"app.verbose = yes\n"
	"app.greeting = explorer\n"
	"scheduler.stop_on_failure = no\n"
	"scheduler.max_failures = 3\n"
	"report.title = Run Report\n"
	"words.sample = the quick brown fox jumps over the lazy dog the fox\n"
	"fib.n = 18\n";

static void on_task_begin(const Event *ev, void *ctx)
{
	AppContext *app = ctx;
	app->tasks_begun++;
	LOG_DEBUG("[%s] begin %s", event_kind_name(ev->kind), ev->message);
}

static void on_task_end(const Event *ev, void *ctx)
{
	AppContext *app = ctx;
	app->tasks_ended++;
	if (app->verbose)
		print_columns(ev->message,
			      ev->code == 0 ? "ok" : "FAILED", 24);
}

static void on_error(const Event *ev, void *ctx)
{
	AppContext *app = ctx;
	app->errors_seen++;
	LOG_ERROR("task '%s' reported error code %d", ev->message, ev->code);
}

static void on_lifecycle(const Event *ev, void *ctx)
{
	const AppContext *app = ctx;
	LOG_INFO("[%s] %s from %s (code %d)", app_name(app),
		 event_kind_name(ev->kind), ev->source, ev->code);
}

static void app_wire_events(AppContext *app)
{
	event_subscribe(app->bus, EVENT_TASK_BEGIN, on_task_begin, app);
	event_subscribe(app->bus, EVENT_TASK_END, on_task_end, app);
	event_subscribe(app->bus, EVENT_ERROR, on_error, app);
	event_subscribe(app->bus, EVENT_STARTUP, on_lifecycle, app);
	event_subscribe(app->bus, EVENT_SHUTDOWN, on_lifecycle, app);
}

static void app_apply_argv(AppContext *app, int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];
		if (str_eq(arg, "-q") || str_eq(arg, "--quiet")) {
			app->verbose = 0;
			log_set_level(LOG_LEVEL_WARN);
		} else if (str_eq(arg, "-v") || str_eq(arg, "--verbose")) {
			app->verbose = 1;
			log_set_level(LOG_LEVEL_DEBUG);
		} else if (str_starts_with(arg, "--set=")) {
			Vec *kv = str_split(arg + 6, '=');
			if (vec_len(kv) >= 2)
				config_set(app->config, vec_get(kv, 0),
					   vec_get(kv, 1));
			str_vec_free(kv);
		} else {
			LOG_WARN("ignoring unknown argument '%s'", arg);
		}
	}
}

AppContext *app_init(int argc, char **argv)
{
	AppContext *app = mem_calloc(1, sizeof(AppContext));

	log_init(LOG_LEVEL_INFO, stderr);
	app->config = config_load_string(DEFAULT_CONFIG);
	app->verbose = config_get_bool(app->config, "app.verbose", 1);
	app_apply_argv(app, argc, argv);

	app->name = config_get(app->config, "app.name", "app");
	app->bus = event_bus_new();
	app->report = report_new(config_get(app->config, "report.title",
					    "Report"));
	app->scheduler = scheduler_new(app->bus, app->config);

	app_wire_events(app);
	cmd_register_all(app->scheduler, app);

	LOG_INFO("initialized '%s' with %zu tasks", app_name(app),
		 scheduler_count(app->scheduler));
	return app;
}

int app_run(AppContext *app)
{
	int failures;

	if (app == NULL)
		return -1;

	print_banner(app_name(app));
	print_hello();
	print_greeting(config_get(app->config, "app.greeting", "friend"));

	failures = scheduler_run_all(app->scheduler, app);
	scheduler_report(app->scheduler);

	app_note(app, "tasks", "see below");
	report_add_int(app->report, "tasks registered",
		       (long)scheduler_count(app->scheduler));
	report_add_int(app->report, "task failures", failures);
	report_add_int(app->report, "events begun", app->tasks_begun);
	report_add_int(app->report, "events ended", app->tasks_ended);
	report_add_bool(app->report, "verbose", app->verbose);
	report_add_separator(app->report);
	report_add_int(app->report, "config keys",
		       (long)config_count(app->config));
	report_add_int(app->report, "lines printed", (long)print_line_count());
	report_print(app->report);

	return failures;
}

int app_shutdown(AppContext *app)
{
	int errors;

	if (app == NULL)
		return -1;

	errors = (int)app->errors_seen;
	event_emitf(app->bus, EVENT_SHUTDOWN, "app", "shutting down", errors);
	event_bus_report(app->bus);
	log_summary();

	scheduler_free(app->scheduler);
	report_free(app->report);
	event_bus_free(app->bus);
	config_free(app->config);
	mem_free(app);

	mem_report("shutdown");
	log_shutdown();
	return errors;
}

const char *app_name(const AppContext *app)
{
	return (app != NULL && app->name != NULL) ? app->name : "app";
}

void app_note(AppContext *app, const char *label, const char *value)
{
	if (app == NULL)
		return;
	report_add_row(app->report, label, value);
}
