#include "../font.h"
#include <assert.h>

int main(void) {
    font_set_size(22);
    assert(font_get_size() == 22);
    font_set_size(17);
    assert(font_get_size() == 18);
    font_set_size(27);
    assert(font_get_size() == 26);
    return 0;
}
