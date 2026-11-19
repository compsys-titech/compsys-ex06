#include "test_support.h"
#include <inttypes.h>
#include <stdio.h>
static void im_read(InstMemory *im, uint32_t address, uint32_t sentinel, const char *id)
{
    uint32_t before[1024]; for (unsigned i = 0; i < 1024; ++i) before[i] = im->mem[i];
    Word out = test_word(sentinel); uint32_t idx = (address-INST_MEM_START)/4; char in[100];
    snprintf(in,sizeof in,"address=%08" PRIx32 " index=%" PRIu32 " out_init=%08" PRIx32,address,idx,sentinel);
    inst_memory_run(im,test_word(address),&out);
    test_state(id,"read-only, all memory retained",in,before[idx],test_value(out),before,before,im->mem,1024);
}
void test_im(void)
{
    InstMemory im = {{0}};
    const uint32_t offsets[] = {0,4,8,4092};
    for (unsigned i = 0; i < 4; ++i) im_read(&im,INST_MEM_START+offsets[i],UINT32_MAX,"IM_ZERO");
    uint32_t pattern = UINT32_C(0x20260601);
    for (unsigned i = 0; i < 1024; ++i) {
        pattern = pattern*UINT32_C(1664525)+UINT32_C(1013904223);
        inst_memory_set_inst(&im,INST_MEM_START+4*i,pattern);
    }
    inst_memory_set_inst(&im,INST_MEM_START,UINT32_C(0x007302b3));
    inst_memory_set_inst(&im,INST_MEM_START+4,0);
    inst_memory_set_inst(&im,INST_MEM_START+8,UINT32_MAX);
    for (unsigned i = 0; i < 1024; ++i) im_read(&im,INST_MEM_START+4*i,i%2 ? UINT32_MAX : 0,"IM_ALL");
    for (unsigned i = 0; i < 32; ++i) {
        inst_memory_set_inst(&im,INST_MEM_START+4092,UINT32_C(1)<<i);
        im_read(&im,INST_MEM_START+4092,0,"IM_BITS");
        im_read(&im,INST_MEM_START,UINT32_MAX,"IM_RETAIN");
    }
    im_read(&im,INST_MEM_START+8,0,"IM_REPEAT"); im_read(&im,INST_MEM_START+4,UINT32_MAX,"IM_REPEAT");
}
static void dm_step(DataMemory *dm, Signal wr, Signal rd, uint32_t address, uint32_t data, uint32_t sentinel, const char *id)
{
    uint32_t before[1024],expected[1024];
    for (unsigned i = 0; i < 1024; ++i) before[i] = expected[i] = dm->mem[i];
    uint32_t idx = wr || rd ? (address-DATA_MEM_START)/4 : 0;
    uint32_t value = rd ? before[idx] : 0;
    if (wr) expected[idx] = data;
    Word out = test_word(sentinel); char in[150];
    snprintf(in,sizeof in,"W=%u R=%u address=%08" PRIx32 " data=%08" PRIx32 " out_init=%08" PRIx32,(unsigned)wr,(unsigned)rd,address,data,sentinel);
    printf("CASE %s input={%s} expected_output=%08" PRIx32 " before_selected=%08" PRIx32 " expected_after_selected=%08" PRIx32 "\n",id,in,value,before[idx],expected[idx]);
    fflush(stdout);
    data_memory_run(dm,wr,rd,test_word(address),test_word(data),&out);
    test_state(id,"output and all 1024 state words",in,value,test_value(out),before,expected,dm->mem,1024);
}
void test_dm(void)
{
    DataMemory dm = {{0}}, other = {{0}};
    const uint32_t bad[] = {DATA_MEM_START-4,DATA_MEM_START+1,DATA_MEM_START+2,DATA_MEM_START+3,DATA_MEM_START+4096,UINT32_MAX,0};
    for (unsigned i = 0; i < 7; ++i) dm_step(&dm,false,false,bad[i],UINT32_MAX,UINT32_MAX,"DM_NOOP_INVALID");
    const uint32_t offsets[] = {0,4,8,4092};
    for (unsigned i = 0; i < 4; ++i) dm_step(&dm,false,true,DATA_MEM_START+offsets[i],0,UINT32_MAX,"DM_ZERO");
    dm_step(&dm,true,false,DATA_MEM_START+4,100,UINT32_MAX,"DM_WRITE1");
    dm_step(&dm,true,false,DATA_MEM_START+8,200,UINT32_MAX,"DM_WRITE2");
    dm_step(&dm,false,true,DATA_MEM_START+8,0,0,"DM_REVERSE2");
    dm_step(&dm,false,true,DATA_MEM_START+4,0,0,"DM_REVERSE1");
    for (unsigned i = 0; i < 38; ++i) {
        const uint32_t v[] = {0,UINT32_MAX,UINT32_C(0x80000000),UINT32_C(0x7fffffff),UINT32_C(0xaaaa5555),UINT32_C(0x5555aaaa)};
        uint32_t data = i < 6 ? v[i] : UINT32_C(1) << (i-6);
        uint32_t addr = DATA_MEM_START+offsets[i%4];
        dm_step(&dm,true,false,addr,data,UINT32_MAX,"DM_WRITE_BITS");
        dm_step(&dm,false,true,addr,~data,0,"DM_READ_BITS");
        dm_step(&other,true,false,addr,~data,UINT32_MAX,"DM_INSTANCE");
        dm_step(&dm,false,true,addr,0,UINT32_MAX,"DM_RETAIN");
        dm_step(&dm,false,false,UINT32_MAX,data,UINT32_MAX,"DM_NOOP_RETAIN");
    }
}
