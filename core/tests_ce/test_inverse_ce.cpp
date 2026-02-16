#include <debug.h>

#define main matrix_test_inverse_main
#include "../tests/test_inverse.cpp"
#undef main

int main() {
		dbg_printf("[TSTINV] start\n");
		const int rc = matrix_test_inverse_main();
		dbg_printf("[TSTINV] PASS rc=%d\n", rc);
		return rc;
}

