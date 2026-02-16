#include <debug.h>

#define main matrix_test_minor_matrix_main
#include "../tests/test_minor_matrix.cpp"
#undef main

int main() {
		dbg_printf("[TSTMIN] start\n");
		const int rc = matrix_test_minor_matrix_main();
		dbg_printf("[TSTMIN] PASS rc=%d\n", rc);
		return rc;
}

