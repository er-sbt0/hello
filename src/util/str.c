#include "util/str.h"

#include "util/log.h"
#include "util/mem.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int str_is_space(char ch)
{
	return isspace((unsigned char)ch) != 0;
}

static char str_lower_char(char ch)
{
	return (char)tolower((unsigned char)ch);
}

static char str_upper_char(char ch)
{
	return (char)toupper((unsigned char)ch);
}

static char *str_map(const char *s, char (*fn)(char))
{
	char *copy;
	size_t i;

	if (s == NULL)
		return NULL;
	copy = str_dup(s);
	for (i = 0; copy[i] != '\0'; i++)
		copy[i] = fn(copy[i]);
	return copy;
}

size_t str_len(const char *s)
{
	return s != NULL ? strlen(s) : 0;
}

char *str_dup(const char *s)
{
	return str_ndup(s, str_len(s));
}

char *str_ndup(const char *s, size_t n)
{
	char *copy;

	if (s == NULL)
		return NULL;
	copy = mem_alloc(n + 1);
	memcpy(copy, s, n);
	copy[n] = '\0';
	return copy;
}

int str_empty(const char *s)
{
	return str_len(s) == 0;
}

int str_eq(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
		return a == b;
	return strcmp(a, b) == 0;
}

int str_ieq(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
		return a == b;
	while (*a != '\0' && *b != '\0') {
		if (str_lower_char(*a) != str_lower_char(*b))
			return 0;
		a++;
		b++;
	}
	return *a == *b;
}

int str_starts_with(const char *s, const char *prefix)
{
	size_t n = str_len(prefix);
	if (s == NULL || prefix == NULL || str_len(s) < n)
		return 0;
	return strncmp(s, prefix, n) == 0;
}

int str_ends_with(const char *s, const char *suffix)
{
	size_t sl = str_len(s);
	size_t xl = str_len(suffix);
	if (s == NULL || suffix == NULL || sl < xl)
		return 0;
	return strcmp(s + (sl - xl), suffix) == 0;
}

unsigned long str_hash(const char *s)
{
	unsigned long hash = 5381ul;
	size_t i;

	if (s == NULL)
		return 0ul;
	for (i = 0; s[i] != '\0'; i++)
		hash = ((hash << 5) + hash) + (unsigned char)s[i];
	return hash;
}

char *str_trim(const char *s)
{
	size_t start = 0;
	size_t end;

	if (s == NULL)
		return NULL;
	end = str_len(s);
	while (s[start] != '\0' && str_is_space(s[start]))
		start++;
	while (end > start && str_is_space(s[end - 1]))
		end--;
	return str_ndup(s + start, end - start);
}

char *str_to_lower(const char *s)
{
	return str_map(s, str_lower_char);
}

char *str_to_upper(const char *s)
{
	return str_map(s, str_upper_char);
}

char *str_repeat(char ch, size_t count)
{
	char *buf = mem_alloc(count + 1);
	memset(buf, ch, count);
	buf[count] = '\0';
	return buf;
}

Vec *str_split(const char *s, char sep)
{
	Vec *parts = vec_new();
	const char *start;
	const char *cursor;

	if (s == NULL)
		return parts;

	start = s;
	for (cursor = s;; cursor++) {
		if (*cursor == sep || *cursor == '\0') {
			vec_push(parts, str_ndup(start, (size_t)(cursor - start)));
			if (*cursor == '\0')
				break;
			start = cursor + 1;
		}
	}
	LOG_DEBUG("split produced %zu parts", vec_len(parts));
	return parts;
}

char *str_join(const Vec *parts, const char *sep)
{
	size_t total = 0;
	size_t sep_len = str_len(sep);
	size_t count = vec_len(parts);
	size_t i;
	char *buf;
	char *cursor;

	for (i = 0; i < count; i++)
		total += str_len(vec_get(parts, i));
	if (count > 1)
		total += sep_len * (count - 1);

	buf = mem_alloc(total + 1);
	cursor = buf;
	for (i = 0; i < count; i++) {
		const char *part = vec_get(parts, i);
		size_t part_len = str_len(part);
		if (i > 0 && sep_len > 0) {
			memcpy(cursor, sep, sep_len);
			cursor += sep_len;
		}
		if (part_len > 0) {
			memcpy(cursor, part, part_len);
			cursor += part_len;
		}
	}
	*cursor = '\0';
	return buf;
}

void str_vec_free(Vec *parts)
{
	vec_free_items(parts, mem_free);
}

int str_parse_int(const char *s, int *out)
{
	char *end = NULL;
	long value;

	if (str_empty(s))
		return 0;
	value = strtol(s, &end, 10);
	if (end == s || (end != NULL && *end != '\0')) {
		LOG_WARN("cannot parse int from '%s'", s);
		return 0;
	}
	if (out != NULL)
		*out = (int)value;
	return 1;
}

int str_parse_bool(const char *s, int *out)
{
	static const char *const truthy[] = { "1", "true", "yes", "on" };
	static const char *const falsy[] = { "0", "false", "no", "off" };
	size_t i;

	if (str_empty(s))
		return 0;
	for (i = 0; i < sizeof(truthy) / sizeof(truthy[0]); i++) {
		if (str_ieq(s, truthy[i])) {
			if (out != NULL)
				*out = 1;
			return 1;
		}
	}
	for (i = 0; i < sizeof(falsy) / sizeof(falsy[0]); i++) {
		if (str_ieq(s, falsy[i])) {
			if (out != NULL)
				*out = 0;
			return 1;
		}
	}
	LOG_WARN("cannot parse bool from '%s'", s);
	return 0;
}
