/* Add your own case IDs, selection reasons, inputs and expected states here.
   This entire file may be edited. Do not call these functions from tasks. */
#include "test_support.h"
void student_a(void)
{
    Register reg = {10}; Word out = test_word(UINT32_MAX);
    register_run(&reg,true,test_word(20),&out);
    test_case("S_A_01","read-before-write at a known state","before=10 W=1 data=20",10,test_value(out),10,reg.val,20,1);
}
void student_b(void)
{
    DataMemory dm = {{0}}; Word out = test_word(UINT32_MAX);
    data_memory_run(&dm,false,false,test_word(UINT32_MAX),test_word(123),&out);
    test_case("S_B_01","unused address must be ignored","W=0 R=0 addr=ffffffff",0,test_value(out),0,dm.mem[0],0,1);
}
