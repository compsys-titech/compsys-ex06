#include "test_support.h"
int main(void)
{
    InstMemory im = {{0}}; DataMemory dm = {{0}}; Word out = test_word(UINT32_MAX);
    test_begin("example-memory");
    inst_memory_set_inst(&im,INST_MEM_START,UINT32_C(0x007302b3));
    inst_memory_run(&im,test_word(INST_MEM_START),&out);
    test_case("EX_IM","slide p30","address=00400000",UINT32_C(0x007302b3),test_value(out),im.mem[0],im.mem[0],UINT32_C(0x007302b3),1);
    data_memory_run(&dm,true,false,test_word(DATA_MEM_START+4),test_word(100),&out);
    test_case("EX_DM_W1","slide p33","W=1 R=0 base+4 data=100",0,test_value(out),0,dm.mem[1],100,1);
    out = test_word(UINT32_MAX);
    data_memory_run(&dm,true,false,test_word(DATA_MEM_START+8),test_word(200),&out);
    test_case("EX_DM_W2","slide p33","W=1 R=0 base+8 data=200",0,test_value(out),0,dm.mem[2],200,1);
    data_memory_run(&dm,false,true,test_word(DATA_MEM_START+8),test_word(0),&out);
    test_case("EX_DM_R2","reverse read","W=0 R=1 base+8",200,test_value(out),200,dm.mem[2],200,1);
    data_memory_run(&dm,false,true,test_word(DATA_MEM_START+4),test_word(0),&out);
    test_case("EX_DM_R1","reverse read","W=0 R=1 base+4",100,test_value(out),100,dm.mem[1],100,1);
    return test_finish();
}
