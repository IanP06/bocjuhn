/* SPDX-License-Identifier: GPL-3.0-only */
/* bocjuhn / Polvo: exact lowercase credits, static dust from gfx.c. */
static void splash_draw(const char *build)
{
    (void)build;
    CV_AREA(0, 0, 240, 240, C_BLACK) {
        static const uint16_t dot[6] = {RGB(255,16,64), RGB(0,208,255), RGB(255,208,0),
                                       RGB(0,255,96), RGB(255,16,64), RGB(0,255,96)};
        cv_text_c(91, &FONT_BRAND, "bocjuhn", RGB(242,244,241));
        cv_text_c(145, &FONT_CREDIT, "created by ryuji kanagawa", RGB(181,193,190));
        for (uint32_t i=0;i<6;i++) cv_rect(88+12*(int32_t)i,184,3,3,dot[i]);
        cv_text_c(214, &FONT_CREDIT, "argentina, 2026", RGB(161,174,171));
    }
}
