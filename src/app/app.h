#ifndef APP_APP_H
#define APP_APP_H

#include "app/report.h"
#include "core/config.h"
#include "core/event.h"
#include "core/scheduler.h"

typedef struct AppContext {
	Config *config;
	EventBus *bus;
	Scheduler *scheduler;
	Report *report;
	const char *name;
	int verbose;
	unsigned tasks_begun;
	unsigned tasks_ended;
	unsigned errors_seen;
} AppContext;

AppContext *app_init(int argc, char **argv);
int app_run(AppContext *app);
int app_shutdown(AppContext *app);
const char *app_name(const AppContext *app);
void app_note(AppContext *app, const char *label, const char *value);

#endif /* APP_APP_H */
