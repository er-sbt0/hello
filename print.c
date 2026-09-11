#include "print.h"

#include "util/mem.h"
#include "util/str.h"

#include <stdio.h>

#define PRINT_DEFAULT_WIDTH 40

static size_t g_lines_printed;

static void print_raw(const char *text)
{
	printf("%s\n", text != NULL ? text : "");
	g_lines_printed++;
}

void print_hello(void)
{
	print_greeting("world");
}

void print_greeting(const char *who)
{
	char *upper = str_to_upper(who);
	printf("hello, %s!\n", upper != NULL ? upper : "?");
	g_lines_printed++;
	mem_free(upper);
}

void print_rule(char ch, size_t width)
{
	char *rule = str_repeat(ch, width > 0 ? width : PRINT_DEFAULT_WIDTH);
	print_raw(rule);
	mem_free(rule);
}

void print_banner(const char *title)
{
	print_rule('=', PRINT_DEFAULT_WIDTH);
	print_raw(title);
	print_rule('=', PRINT_DEFAULT_WIDTH);
}

void print_kv(const char *key, const char *value)
{
	printf("  %-20s %s\n", key != NULL ? key : "", value != NULL ? value : "");
	g_lines_printed++;
}

void print_kv_int(const char *key, long value)
{
	char buf[32];
	snprintf(buf, sizeof(buf), "%ld", value);
	print_kv(key, buf);
}

void print_bullet(const char *text)
{
	printf("  - %s\n", text != NULL ? text : "");
	g_lines_printed++;
}

void print_indented(int depth, const char *text)
{
	char *pad = str_repeat(' ', (size_t)(depth > 0 ? depth * 2 : 0));
	printf("%s%s\n", pad, text != NULL ? text : "");
	g_lines_printed++;
	mem_free(pad);
}

void print_columns(const char *left, const char *right, size_t width)
{
	printf("%-*s %s\n", (int)(width > 0 ? width : PRINT_DEFAULT_WIDTH / 2),
	       left != NULL ? left : "", right != NULL ? right : "");
	g_lines_printed++;
}

size_t print_line_count(void)
{
	return g_lines_printed;
}
