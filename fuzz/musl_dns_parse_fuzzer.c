#include <limits.h>
#include <resolv.h>
#include <stdint.h>
#include <stddef.h>

int __dns_parse(const unsigned char *r, int rlen,
                int (*callback)(void *, int, const void *, int, const void *, int),
                void *ctx);

static int cb(void *ctx, int rrtype, const void *rdata, int rdlen, const void *pkt, int pktlen)
{
	(void)ctx;
	(void)rrtype;
	if (!pkt || pktlen <= 0 || !rdata || rdlen < 0) return 0;

	const unsigned char *base = (const unsigned char *)pkt;
	const unsigned char *end = base + (size_t)pktlen;
	const unsigned char *src = (const unsigned char *)rdata;
	if (src < base || src >= end) return 0;

	char out[256];
	(void)dn_expand(base, end, src, out, (int)sizeof(out));
	return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (!data || size == 0) return 0;
	if (size > 4096) return 0;
	if (size > (size_t)INT_MAX) return 0;
	(void)__dns_parse((const unsigned char *)data, (int)size, cb, 0);
	return 0;
}
