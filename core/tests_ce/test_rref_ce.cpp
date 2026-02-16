#include <debug.h>

#define main matrix_test_rref_main
#include "../tests/test_rref.cpp"
#undef main

int main() {
		dbg_printf("[TSTRREF] start\n");
		const int rc = matrix_test_rref_main();
		dbg_printf("[TSTRREF] PASS rc=%d\n", rc);
		return rc;
}

