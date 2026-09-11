#ifndef APP_REPORT_H
#define APP_REPORT_H

#include <stddef.h>

typedef struct Report Report;

Report *report_new(const char *title);
void report_free(Report *r);
int report_add_row(Report *r, const char *label, const char *value);
int report_add_int(Report *r, const char *label, long value);
int report_add_bool(Report *r, const char *label, int value);
int report_add_separator(Report *r);
size_t report_row_count(const Report *r);
char *report_render(const Report *r);
void report_print(const Report *r);
const char *report_title(const Report *r);

#endif /* APP_REPORT_H */
