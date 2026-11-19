#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdio.h>
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ != 201710L
#error This course requires C17.
#endif
int main(void)
{
    bool ok = true;
    uint32_t x = UINT32_MAX;
    printf("C17 bool=%d uint32=%" PRIu32 "\n", (int)ok, x);
    puts("RESULT check PASS");
    return 0;
}
