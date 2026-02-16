#include <debug.h>

#define main matrix_test_det_main
#include "../tests/test_det.cpp"
#undef main

int main() {
		dbg_printf("[TSTDET] start\n");
		const int rc = matrix_test_det_main();
		dbg_printf("[TSTDET] PASS rc=%d\n", rc);
		return rc;
}

