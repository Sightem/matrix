#include <debug.h>

#define main matrix_test_rational_main
#include "../tests/test_rational.cpp"
#undef main

int main() {
		dbg_printf("[TSTRAT] start\n");
		const int rc = matrix_test_rational_main();
		dbg_printf("[TSTRAT] PASS rc=%d\n", rc);
		return rc;
}

