#include <resolv.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static uint8_t next_u8(const uint8_t *data, size_t size, size_t *idx)
{
	if (!data || !idx || *idx >= size) return 0;
	return data[(*idx)++];
}

static void gen_label(const uint8_t *data, size_t size, size_t *idx, char *out, size_t out_cap)
{
	if (!out || out_cap == 0) return;
	if (out_cap < 2) {
		out[0] = 0;
		return;
	}

	size_t limit = out_cap - 1;
	if (limit > 63) limit = 63;
	size_t len = 1 + (size_t)(next_u8(data, size, idx) % (uint8_t)limit);
	for (size_t i = 0; i < len; i++) {
		uint8_t b = next_u8(data, size, idx);
		uint8_t v = b % 36;
		out[i] = (v < 26) ? (char)('a' + v) : (char)('0' + (v - 26));
	}
	out[len] = 0;
}

static void gen_names(const uint8_t *data, size_t size, char *name1, size_t name1_cap, char *name2, size_t name2_cap)
{
	size_t idx = 1;
	char p1[64], p2[64], s1[64], s2[64];
	gen_label(data, size, &idx, p1, sizeof(p1));
	gen_label(data, size, &idx, p2, sizeof(p2));
	gen_label(data, size, &idx, s1, sizeof(s1));
	gen_label(data, size, &idx, s2, sizeof(s2));

	const char *dot1 = (next_u8(data, size, &idx) & 1) ? "." : "";
	const char *dot2 = (next_u8(data, size, &idx) & 1) ? "." : "";

	if (name1 && name1_cap)
		(void)snprintf(name1, name1_cap, "%s.%s.%s%s", p1, s1, s2, dot1);
	if (name2 && name2_cap)
		(void)snprintf(name2, name2_cap, "%s.%s.%s%s", p2, s1, s2, dot2);
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (!data || size == 0) return 0;
	if (size > 4096) return 0;

	size_t off = data[0] % size;
	const unsigned char *base = (const unsigned char *)data;
	const unsigned char *end = base + size;
	const unsigned char *src = base + off;

	(void)dn_skipname(src, end);

	char out[256];
	(void)dn_expand(base, end, src, out, (int)sizeof(out));

	/* Also exercise name compression (dn_comp) with a synthetic message. */
	char name1[256], name2[256];
	gen_names(data, size, name1, sizeof(name1), name2, sizeof(name2));

	unsigned char msg[512];
	memset(msg, 0, sizeof(msg));

	unsigned char *dnptrs[16];
	dnptrs[0] = msg;
	dnptrs[1] = 0;

	unsigned char **lastdnptr = dnptrs + (sizeof(dnptrs) / sizeof(dnptrs[0]));

	int pos = 12;
	int n1 = dn_comp(name1, msg + pos, (int)(sizeof(msg) - (size_t)pos), dnptrs, lastdnptr);
	if (n1 > 0) {
		const unsigned char *mend = msg + pos + (size_t)n1;
		(void)dn_skipname(msg + pos, mend);
		(void)dn_expand(msg, mend, msg + pos, out, (int)sizeof(out));
		pos += n1;
	}

	if (pos >= 0 && (size_t)pos < sizeof(msg)) {
		int n2 = dn_comp(name2, msg + pos, (int)(sizeof(msg) - (size_t)pos), dnptrs, lastdnptr);
		if (n2 > 0) {
			const unsigned char *mend = msg + pos + (size_t)n2;
			(void)dn_skipname(msg + pos, mend);
			(void)dn_expand(msg, mend, msg + pos, out, (int)sizeof(out));
		}
	}
	return 0;
}
