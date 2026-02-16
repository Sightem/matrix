#include <debug.h>

#define main matrix_test_cramer_main
#include "../tests/test_cramer.cpp"
#undef main

int main() {
		dbg_printf("[TSTCRAM] start\n");
		const int rc = matrix_test_cramer_main();
		dbg_printf("[TSTCRAM] PASS rc=%d\n", rc);
		return rc;
}

