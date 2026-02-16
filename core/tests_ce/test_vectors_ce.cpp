#include <debug.h>

#define main matrix_test_vectors_main
#include "../tests/test_vectors.cpp"
#undef main

int main() {
		dbg_printf("[TSTVEC] start\n");
		const int rc = matrix_test_vectors_main();
		dbg_printf("[TSTVEC] PASS rc=%d\n", rc);
		return rc;
}

