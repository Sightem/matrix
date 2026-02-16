#include <assert.h>
#include <debug.h>
#include <stdlib.h>

extern "C" void __assert_fail_loc(const struct __assert_loc* loc) {
		dbg_printf("[assert] %s:%u\n", loc->__file, (unsigned)loc->__line);
		dbg_printf("[assert] %s\n", loc->__function);
		dbg_printf("[assert] %s\n", loc->__assertion);
		abort();
}
