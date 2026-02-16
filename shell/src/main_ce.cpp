#include "matrix_shell/app.hpp"

#include "matrix_shell/ui.hpp"

#include <graphx.h>
#include <keypadc.h>

int main(void) {
		gfx_Begin();
		gfx_SetDrawBuffer();

		kb_SetMode(MODE_3_CONTINUOUS);

		gfx_palette[matrix_shell::ui::color::kBlack] = gfx_RGBTo1555(0, 0, 0);
		gfx_palette[matrix_shell::ui::color::kWhite] = gfx_RGBTo1555(255, 255, 255);
		gfx_palette[matrix_shell::ui::color::kLightGray] = gfx_RGBTo1555(230, 230, 230);
		gfx_palette[matrix_shell::ui::color::kDarkGray] = gfx_RGBTo1555(160, 160, 160);
		gfx_palette[matrix_shell::ui::color::kBlue] = gfx_RGBTo1555(40, 90, 255);

		static matrix_shell::App app;
		if (!app.init()) {
				gfx_End();
				return 1;
		}

		while (app.step())
				;

		gfx_End();
		kb_Reset();
		return 0;
}
