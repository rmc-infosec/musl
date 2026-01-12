#include <regex.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint32_t fnv1a32(const uint8_t *data, size_t size)
{
	uint32_t h = 2166136261u;
	for (size_t i = 0; i < size; i++) {
		h ^= (uint32_t)data[i];
		h *= 16777619u;
	}
	return h;
}

static char *dup_cstring(const uint8_t *data, size_t size, size_t max_len)
{
	if (size > max_len) size = max_len;
	char *s = malloc(size + 1);
	if (!s) return 0;
	memcpy(s, data, size);
	s[size] = 0;
	return s;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (!data || size == 0) return 0;

	/* Format: pattern "\n" text ["\n" flags] */
	const uint8_t *nl1 = memchr(data, '\n', size);
	if (!nl1) return 0;
	const uint8_t *pat = data;
	size_t pat_len = (size_t)(nl1 - data);

	const uint8_t *rest = nl1 + 1;
	size_t rest_len = size - pat_len - 1;

	const uint8_t *nl2 = memchr(rest, '\n', rest_len);
	const uint8_t *txt = rest;
	size_t txt_len = nl2 ? (size_t)(nl2 - rest) : rest_len;

	const uint8_t *flags = nl2 ? (nl2 + 1) : 0;
	size_t flags_len = nl2 ? (rest_len - txt_len - 1) : 0;

	/* Keep inputs bounded; regex engines can be computationally expensive. */
	enum { MAX_PAT = 2048, MAX_TXT = 4096 };
	char *pattern = dup_cstring(pat, pat_len, MAX_PAT);
	char *text = dup_cstring(txt, txt_len, MAX_TXT);
	if (!pattern || !text) {
		free(pattern);
		free(text);
		return 0;
	}

	uint8_t h = (flags && flags_len) ? flags[0] : 0;
	int cflags = 0;
	if (h & 0x01) cflags |= REG_EXTENDED;
	if (h & 0x02) cflags |= REG_ICASE;
	if (h & 0x04) cflags |= REG_NEWLINE;
	if (h & 0x08) cflags |= REG_NOSUB;

	int eflags = 0;
	if (h & 0x10) eflags |= REG_NOTBOL;
	if (h & 0x20) eflags |= REG_NOTEOL;

	uint32_t mix = (flags && flags_len > 1) ? fnv1a32(flags + 1, flags_len - 1) : 0;

	regex_t re;
	memset(&re, 0, sizeof(re));
	if (regcomp(&re, pattern, cflags) == 0) {
		size_t nmatch = mix % 9;
		regmatch_t m[8];
		regmatch_t *pm = nmatch ? m : 0;
		regexec(&re, text, nmatch, pm, eflags);
		regfree(&re);
	}

	free(pattern);
	free(text);
	return 0;
}
