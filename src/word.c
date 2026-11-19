#include "compsys.h"

void word_set_value(Word *w, uint32_t value)
{
    for (unsigned i = 0; i < 32; ++i)
        w->bit[i] = ((value >> i) & UINT32_C(1)) != 0;
}

uint32_t word_get_value(Word w)
{
    uint32_t value = 0;
    for (unsigned i = 0; i < 32; ++i)
        if (w.bit[i]) value |= UINT32_C(1) << i;
    return value;
}
