#include "core/config.h"

#include "ds/map.h"
#include "util/log.h"
#include "util/mem.h"
#include "util/str.h"

#include <stdio.h>

struct Config {
	Map *entries;
	unsigned errors;
	unsigned lines_read;
};

static int config_is_comment(const char *line)
{
	return str_starts_with(line, "#") || str_starts_with(line, ";");
}

static int config_parse_line(Config *c, const char *line, size_t lineno)
{
	Vec *parts;
	char *key = NULL;
	char *value = NULL;
	char *trimmed = str_trim(line);
	int ok = 0;

	c->lines_read++;

	if (str_empty(trimmed) || config_is_comment(trimmed)) {
		mem_free(trimmed);
		return 1;
	}

	parts = str_split(trimmed, '=');
	if (vec_len(parts) < 2) {
		LOG_WARN("config line %zu is not a key=value pair: '%s'", lineno,
			 trimmed);
		c->errors++;
	} else {
		key = str_trim(vec_get(parts, 0));
		value = str_trim(vec_get(parts, 1));
		ok = config_set(c, key, value);
		mem_free(key);
		mem_free(value);
	}

	str_vec_free(parts);
	mem_free(trimmed);
	return ok;
}

Config *config_new(void)
{
	Config *c = mem_calloc(1, sizeof(Config));
	c->entries = map_new();
	return c;
}

void config_free(Config *c)
{
	if (c == NULL)
		return;
	map_free_values(c->entries, mem_free);
	mem_free(c);
}

int config_set(Config *c, const char *key, const char *value)
{
	char *stored;

	if (c == NULL || str_empty(key))
		return 0;

	stored = map_get(c->entries, key);
	if (stored != NULL) {
		LOG_DEBUG("config overriding '%s'", key);
		mem_free(map_remove(c->entries, key));
	}
	return map_put(c->entries, key, str_dup(value));
}

Config *config_load_string(const char *text)
{
	Config *c = config_new();
	Vec *lines = str_split(text, '\n');
	size_t i;

	for (i = 0; i < vec_len(lines); i++)
		config_parse_line(c, vec_get(lines, i), i + 1);

	str_vec_free(lines);
	LOG_INFO("config parsed %u lines into %zu keys (%u errors)",
		 c->lines_read, config_count(c), c->errors);
	return c;
}

Config *config_load_file(const char *path)
{
	FILE *fp = fopen(path, "r");
	Config *c;
	char line[512];
	size_t lineno = 0;

	if (fp == NULL) {
		LOG_WARN("config file '%s' not found, using defaults", path);
		return config_new();
	}

	c = config_new();
	while (fgets(line, (int)sizeof(line), fp) != NULL)
		config_parse_line(c, line, ++lineno);
	fclose(fp);

	LOG_INFO("config loaded '%s': %zu keys", path, config_count(c));
	return c;
}

const char *config_get(const Config *c, const char *key, const char *fallback)
{
	const char *value;

	if (c == NULL)
		return fallback;
	value = map_get(c->entries, key);
	return value != NULL ? value : fallback;
}

int config_get_int(const Config *c, const char *key, int fallback)
{
	int parsed = fallback;
	const char *raw = config_get(c, key, NULL);

	if (raw != NULL && str_parse_int(raw, &parsed))
		return parsed;
	return fallback;
}

int config_get_bool(const Config *c, const char *key, int fallback)
{
	int parsed = fallback;
	const char *raw = config_get(c, key, NULL);

	if (raw != NULL && str_parse_bool(raw, &parsed))
		return parsed;
	return fallback;
}

int config_has(const Config *c, const char *key)
{
	return c != NULL && map_has(c->entries, key);
}

size_t config_count(const Config *c)
{
	return c != NULL ? map_size(c->entries) : 0;
}

Vec *config_keys(const Config *c)
{
	return c != NULL ? map_keys(c->entries) : vec_new();
}

unsigned config_error_count(const Config *c)
{
	return c != NULL ? c->errors : 0u;
}

static void config_dump_entry(const char *key, void *value, void *ctx)
{
	(void)ctx;
	LOG_INFO("config %-16s = %s", key, (const char *)value);
}

void config_dump(const Config *c)
{
	if (c == NULL)
		return;
	LOG_INFO("config dump (%zu keys, %u parse errors)", config_count(c),
		 config_error_count(c));
	map_foreach(c->entries, config_dump_entry, NULL);
}
