#ifndef FEATURES_H
#define FEATURES_H

/* Fuzzing build shim:
 * - avoid musl's internal src/include/ wrappers (they expect musl internals)
 * - still provide the attributes/macros used by musl sources (weak_alias, hidden)
 */

#include "../../include/features.h"

#define weak __attribute__((__weak__))
#define hidden __attribute__((__visibility__("hidden")))
#define weak_alias(old, new) \
	extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

#endif

