#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (!data || size == 0) return 0;

	/* Treat input as both:
	 * - raw address bytes for inet_ntop
	 * - a C string for inet_pton
	 */

	unsigned char v4[4] = {0};
	unsigned char v6[16] = {0};
	if (size >= sizeof(v4)) memcpy(v4, data, sizeof(v4));
	if (size >= sizeof(v6)) memcpy(v6, data, sizeof(v6));
	else memcpy(v6, data, size);

	char tmp[128];
	socklen_t small = (socklen_t)(data[0] % 8);
	(void)inet_ntop(AF_INET, v4, tmp, small);
	(void)inet_ntop(AF_INET6, v6, tmp, small);
	(void)inet_ntop(AF_INET, v4, tmp, sizeof(tmp));
	(void)inet_ntop(AF_INET6, v6, tmp, sizeof(tmp));
	(void)inet_ntop(0x7fffffff, v6, tmp, sizeof(tmp));

	enum { MAX_STR = 256 };
	size_t n = size > MAX_STR ? MAX_STR : size;
	char *s = malloc(n + 1);
	if (!s) return 0;
	memcpy(s, data, n);
	s[n] = 0;
	for (size_t i = 0; i < n; i++) {
		if (s[i] == '\n' || s[i] == '\r') {
			s[i] = 0;
			break;
		}
	}

	unsigned char out4[4];
	unsigned char out6[16];

	(void)inet_pton(0x7fffffff, s, out4);
	if (inet_pton(AF_INET, s, out4) == 1)
		(void)inet_ntop(AF_INET, out4, tmp, sizeof(tmp));
	if (inet_pton(AF_INET6, s, out6) == 1)
		(void)inet_ntop(AF_INET6, out6, tmp, sizeof(tmp));

	free(s);
	return 0;
}
