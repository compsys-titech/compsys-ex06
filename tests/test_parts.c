#include "test_support.h"
#include <stdio.h>
void test_parts(void)
{
    const uint32_t values[] = {0,UINT32_MAX,UINT32_C(0x80000000),UINT32_C(0x7fffffff),UINT32_C(0xaaaa5555),UINT32_C(0x5555aaaa)};
    for (unsigned i = 0; i < 38; ++i) {
        uint32_t v = i < 6 ? values[i] : UINT32_C(1) << (i-6);
        Word w = test_word(~v); word_set_value(&w,v);
        test_case("P_WORD_SET","all 32 wires","uint32 -> Word",v,test_value(w),0,0,0,1);
        test_case("P_WORD_GET","independent input","Word -> uint32",v,word_get_value(test_word(v)),0,0,0,1);
        Register reg = {0}; register_set_value(&reg,v);
        test_case("P_REGISTER_HELPER","setter/getter","value",v,register_get_value(&reg),0,reg.val,v,1);
    }
    const uint32_t offsets[] = {0,4,8,4092};
    InstMemory im = {{0}}; uint32_t before[1024] = {0}, expected[1024] = {0};
    for (unsigned b = 0; b < 2; ++b) for (unsigned i = 0; i < 4; ++i) {
        uint32_t base = b ? DATA_MEM_START : INST_MEM_START;
        uint32_t idx = checked_memory_index(base+offsets[i],base,b ? "DataMemory" : "InstMemory");
        test_case("P_INDEX","byte address conversion","base + offset",offsets[i]/4,idx,0,0,0,1);
    }
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < 1024; ++j) before[j] = im.mem[j];
        expected[offsets[i]/4] = values[i];
        inst_memory_set_inst(&im,INST_MEM_START+offsets[i],values[i]);
        test_state("P_IM_SET","only selected word","four addresses",0,0,before,expected,im.mem,1024);
    }
    for (unsigned mode = 0; mode < 3; ++mode) {
        checked_data_memory_control(mode == 2,mode == 1);
        test_case("P_CONTROL","valid controls","00 / 01 / 10",0,0,0,0,0,1);
    }
}
