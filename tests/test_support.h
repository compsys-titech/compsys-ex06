#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H
#include "compsys.h"
Word test_word(uint32_t value);
uint32_t test_value(Word w);
void test_select(unsigned n, Signal out[5]);
void test_begin(const char *group);
void test_case(const char *id, const char *reason, const char *input,
    uint32_t expected, uint32_t actual, uint32_t before, uint32_t after,
    uint32_t expected_after, int invariant);
void test_state(const char *id, const char *reason, const char *input,
    uint32_t expected, uint32_t actual, const uint32_t *before,
    const uint32_t *expected_after, const uint32_t *actual_after, unsigned n);
int test_finish(void);
void test_parts(void); void test_registers(void); void test_decoder(void);
void test_mux(void); void test_rf(void); void test_im(void); void test_dm(void);
void student_a(void); void student_b(void);
int test_error(int argc, char **argv);
#endif
