#include <debug.h>

#define main matrix_test_spaces_main
#include "../tests/test_spaces.cpp"
#undef main

int main() {
		dbg_printf("[TSTSPC] start\n");
		const int rc = matrix_test_spaces_main();
		dbg_printf("[TSTSPC] PASS rc=%d\n", rc);
		return rc;
}

