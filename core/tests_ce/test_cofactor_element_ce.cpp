#include <debug.h>

#define main matrix_test_cofactor_element_main
#include "../tests/test_cofactor_element.cpp"
#undef main

int main() {
		dbg_printf("[TSTCOF] start\n");
		const int rc = matrix_test_cofactor_element_main();
		dbg_printf("[TSTCOF] PASS rc=%d\n", rc);
		return rc;
}

