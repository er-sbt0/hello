#include "app/app.h"
#include "print.h"
#include "util/log.h"

#include <stdio.h>

static int run_application(int argc, char **argv)
{
	AppContext *app = app_init(argc, argv);
	int failures = app_run(app);
	int errors = app_shutdown(app);

	if (failures > 0 || errors > 0)
		LOG_WARN("finished with %d task failures and %d errors",
			 failures, errors);
	return failures == 0 && errors == 0 ? 0 : 1;
}

int main(int argc, char **argv)
{
	int status = run_application(argc, argv);

	print_rule('-', 40);
	print_kv_int("exit status", status);
	return status;
}
