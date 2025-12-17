#include <arpa/nameser.h>
#include <limits.h>
#include <stdint.h>
#include <stddef.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (!data || size == 0) return 0;
	if (size > 4096) return 0;
	if (size > (size_t)INT_MAX) return 0;

	ns_msg handle;
	if (ns_initparse((const unsigned char *)data, (int)size, &handle) != 0)
		return 0;

	for (int section = 0; section < ns_s_max; section++) {
		int count = ns_msg_count(handle, section);
		if (count < 0) continue;
		if (count > 32) count = 32;
		for (int rrnum = 0; rrnum < count; rrnum++) {
			ns_rr rr;
			(void)ns_parserr(&handle, (ns_sect)section, rrnum, &rr);
		}
	}

	return 0;
}
