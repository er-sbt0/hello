#include "app/report.h"

#include "ds/vec.h"
#include "print.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/str.h"

#include <stdio.h>

typedef struct ReportRow {
	char *label;
	char *value;
} ReportRow;

struct Report {
	char *title;
	Vec *rows;
	size_t widest_label;
};

static ReportRow *row_new(const char *label, const char *value)
{
	ReportRow *row = mem_calloc(1, sizeof(ReportRow));
	row->label = str_dup(label != NULL ? label : "");
	row->value = str_dup(value != NULL ? value : "");
	return row;
}

static void row_free(void *item)
{
	ReportRow *row = item;
	if (row == NULL)
		return;
	mem_free(row->label);
	mem_free(row->value);
	mem_free(row);
}

static void row_track_width(Report *r, const ReportRow *row)
{
	size_t width = str_len(row->label);
	if (width > r->widest_label)
		r->widest_label = width;
}

Report *report_new(const char *title)
{
	Report *r = mem_calloc(1, sizeof(Report));
	r->title = str_dup(title != NULL ? title : "report");
	r->rows = vec_new();
	return r;
}

void report_free(Report *r)
{
	if (r == NULL)
		return;
	vec_free_items(r->rows, row_free);
	mem_free(r->title);
	mem_free(r);
}

int report_add_row(Report *r, const char *label, const char *value)
{
	ReportRow *row;

	if (r == NULL)
		return 0;
	row = row_new(label, value);
	row_track_width(r, row);
	return vec_push(r->rows, row);
}

int report_add_int(Report *r, const char *label, long value)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%ld", value);
	return report_add_row(r, label, buf);
}

int report_add_bool(Report *r, const char *label, int value)
{
	return report_add_row(r, label, value ? "yes" : "no");
}

int report_add_separator(Report *r)
{
	return report_add_row(r, "", "");
}

size_t report_row_count(const Report *r)
{
	return r != NULL ? vec_len(r->rows) : 0;
}

const char *report_title(const Report *r)
{
	return r != NULL ? r->title : "";
}

char *report_render(const Report *r)
{
	Vec *lines;
	char *joined;
	size_t i;

	if (r == NULL)
		return str_dup("");

	lines = vec_new();
	vec_push(lines, str_dup(r->title));

	for (i = 0; i < vec_len(r->rows); i++) {
		const ReportRow *row = vec_get(r->rows, i);
		char buf[256];
		snprintf(buf, sizeof(buf), "  %-*s : %s",
			 (int)r->widest_label, row->label, row->value);
		vec_push(lines, str_dup(buf));
	}

	joined = str_join(lines, "\n");
	str_vec_free(lines);
	return joined;
}

void report_print(const Report *r)
{
	size_t i;

	if (r == NULL)
		return;
	print_banner(report_title(r));
	for (i = 0; i < vec_len(r->rows); i++) {
		const ReportRow *row = vec_get(r->rows, i);
		if (str_empty(row->label) && str_empty(row->value))
			print_rule('-', 40);
		else
			print_kv(row->label, row->value);
	}
	print_rule('=', 40);
	LOG_DEBUG("printed report '%s' with %zu rows", report_title(r),
		  report_row_count(r));
}
