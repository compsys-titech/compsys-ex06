#include "test_support.h"
#include <inttypes.h>
#include <stdio.h>
static const char *group;
static unsigned cases, failures;
/* Independent conversions: expectations never call a task implementation. */
Word test_word(uint32_t value)
{
    Word w = {{false}};
    for (unsigned i = 0; i < 32; ++i) w.bit[i] = (value & (UINT32_C(1) << i)) != 0;
    return w;
}
uint32_t test_value(Word w)
{
    uint32_t v = 0;
    for (unsigned i = 0; i < 32; ++i) if (w.bit[i]) v |= UINT32_C(1) << i;
    return v;
}
void test_select(unsigned n, Signal out[5])
{ for (unsigned i = 0; i < 5; ++i) out[i] = (n & (1u << i)) != 0; }
void test_begin(const char *name) { group = name; cases = 0; failures = 0; }
void test_case(const char *id, const char *reason, const char *input,
    uint32_t expected, uint32_t actual, uint32_t before, uint32_t after,
    uint32_t expected_after, int invariant)
{
    int ok = expected == actual && after == expected_after && invariant;
    ++cases; if (!ok) ++failures;
    printf("[%s] %s #%u input={%s} expected=%08" PRIx32 " actual=%08" PRIx32
           " before=%08" PRIx32 " after=%08" PRIx32 " expected_after=%08" PRIx32
           " invariant=%s reason=%s\n",ok ? "PASS" : "FAIL",id,cases,input,
           expected,actual,before,after,expected_after,invariant ? "yes" : "NO",reason);
}
void test_state(const char *id, const char *reason, const char *input,
    uint32_t expected, uint32_t actual, const uint32_t *before,
    const uint32_t *expected_after, const uint32_t *actual_after, unsigned n)
{
    unsigned i = 0;
    while (i < n && expected_after[i] == actual_after[i]) ++i;
    unsigned changed = 0;
    while (changed < n && before[changed] == actual_after[changed]) ++changed;
    unsigned show = i < n ? i : changed < n ? changed : 0;
    if (i < n) printf("[STATE] %s first_diff_index=%u\n",id,i);
    test_case(id,reason,input,expected,actual,before[show],actual_after[show],expected_after[show],i == n);
}
int test_finish(void)
{
    printf("RESULT %s %s cases=%u failures=%u\n",group,failures ? "FAIL" : "PASS",cases,failures);
    return failures ? 5 : 0;
}
