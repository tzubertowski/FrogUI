#include "../font.h"
#include <assert.h>

int main(void) {
    static uint16_t framebuffer[854 * 480];
    font_init();
    assert(font_measure_text("FrogUI") > 0);
    font_draw_text(framebuffer, 854, 480, 0, 0, "FrogUI", 0xffff);
    int rendered = 0;
    for (unsigned i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); i++)
        rendered |= framebuffer[i] != 0;
    assert(rendered);
    font_set_size(22);
    assert(font_get_size() == 22);
    font_set_size(17);
    assert(font_get_size() == 18);
    font_set_size(27);
    assert(font_get_size() == 23);
    return 0;
}
