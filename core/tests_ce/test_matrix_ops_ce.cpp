#include <debug.h>

#define main matrix_test_matrix_ops_main
#include "../tests/test_matrix_ops.cpp"
#undef main

int main() {
		dbg_printf("[TSTMOPS] start\n");
		const int rc = matrix_test_matrix_ops_main();
		dbg_printf("[TSTMOPS] PASS rc=%d\n", rc);
		return rc;
}

