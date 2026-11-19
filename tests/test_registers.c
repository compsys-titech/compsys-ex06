#include "test_support.h"
#include <inttypes.h>
#include <stdio.h>
static const uint32_t vals[] = {0,UINT32_MAX,UINT32_C(0x80000000),UINT32_C(0x7fffffff),UINT32_C(0xaaaa5555),UINT32_C(0x5555aaaa)};
static void reg_step(Register *reg, Signal en, uint32_t data, uint32_t sentinel, const char *id)
{
    uint32_t before = reg->val; Word out = test_word(sentinel); char in[120];
    snprintf(in,sizeof in,"wctl=%u wdata=%08" PRIx32 " out_init=%08" PRIx32,(unsigned)en,data,sentinel);
    register_run(reg,en,test_word(data),&out);
    test_case(id,"old output then conditional update",in,before,test_value(out),before,reg->val,en ? data : before,1);
}
void test_registers(void)
{
    Register r = {0}, other = {UINT32_C(0x13579bdf)};
    reg_step(&r,false,999,UINT32_MAX,"R_ZERO");
    for (unsigned i = 0; i < 38; ++i) {
        uint32_t v = i < 6 ? vals[i] : UINT32_C(1) << (i-6);
        reg_step(&r,true,v,0,"R_WRITE_OLD"); reg_step(&r,false,~v,UINT32_MAX,"R_HOLD");
        reg_step(&other,false,0,0,"R_INSTANCE");
        reg_step(&r,false,0,0,"R_RETAIN");
    }
    reg_step(&r,true,100,0,"R_SEQUENCE"); reg_step(&r,false,999,0,"R_SEQUENCE");
    reg_step(&r,true,200,UINT32_MAX,"R_SEQUENCE"); reg_step(&r,false,0,UINT32_MAX,"R_SEQUENCE");
}
void test_decoder(void)
{
    Word out; Signal a[5]; char in[80];
    for (unsigned repeat = 0; repeat < 2; ++repeat) for (unsigned n = 0; n < 32; ++n) {
        test_select(n,a); out = test_word(repeat ? UINT32_MAX : 0);
        snprintf(in,sizeof in,"index=%u init=%s",n,repeat ? "ones" : "zeros");
        decoder5(a,&out);
        test_case("D_ALL","one-hot and bit order",in,UINT32_C(1)<<n,test_value(out),0,0,0,1);
    }
    const unsigned order[] = {3,5,3}; out = test_word(UINT32_MAX);
    for (unsigned i = 0; i < 3; ++i) {
        test_select(order[i],a); decoder5(a,&out);
        test_case("D_REUSE","clear previous selection","3 -> 5 -> 3",UINT32_C(1)<<order[i],test_value(out),0,0,0,1);
    }
}
void test_mux(void)
{
    Word ins[32], out; Signal c[5]; char in[100];
    for (unsigned pattern = 0; pattern < 3; ++pattern) {
        for (unsigned i = 0; i < 32; ++i) ins[i] = test_word(pattern == 0 ? UINT32_C(0x80000001) ^ (i*UINT32_C(0x01010101)) : pattern == 1 ? UINT32_C(1)<<i : ~(UINT32_C(1)<<i));
        for (unsigned round = 0; round < 2; ++round) for (unsigned n = 0; n < 32; ++n) {
            unsigned sel = round ? 31-n : n; test_select(sel,c); out = test_word(round ? UINT32_MAX : 0);
            snprintf(in,sizeof in,"select=%u selected=%08" PRIx32 " pattern=%u init=%s",sel,test_value(ins[sel]),pattern,round ? "ones" : "zeros");
            mux32(ins,c,&out);
            test_case("M_ALL","32 inputs, each 32 bits",in,test_value(ins[sel]),test_value(out),0,0,0,1);
        }
    }
}
static void rf_step(RegisterFile *rf, Signal enable, unsigned a, unsigned b, unsigned wr, uint32_t data, const char *id)
{
    uint32_t before[32], expected[32], after[32]; Signal s1[5],s2[5],sw[5];
    for (unsigned i = 0; i < 32; ++i) before[i] = expected[i] = rf->r[i].val;
    expected[0] = 0; if (enable && wr) expected[wr] = data;
    test_select(a,s1); test_select(b,s2); test_select(wr,sw);
    Word o1 = test_word(UINT32_MAX), o2 = test_word(0); char in[140];
    snprintf(in,sizeof in,"write=%u read1=%u read2=%u write1=%u data=%08" PRIx32,(unsigned)enable,a,b,wr,data);
    register_file_run(rf,enable,s1,s2,sw,test_word(data),&o1,&o2);
    for (unsigned i = 0; i < 32; ++i) after[i] = rf->r[i].val;
    test_state(id,"port1 old value; all state",in,a ? before[a] : 0,test_value(o1),before,expected,after,32);
    test_state(id,"port2 old value; all state",in,b ? before[b] : 0,test_value(o2),before,expected,after,32);
}
void test_rf(void)
{
    RegisterFile rf = {{{0}}}, other = {{{0}}};
    for (unsigned i = 0; i < 32; ++i) rf_step(&rf,false,i,31-i,i,UINT32_MAX,"RF_ZERO");
    for (unsigned i = 1; i < 32; ++i) rf_step(&rf,true,i,i,i,UINT32_C(0x81230000)+i,"RF_WRITE_OLD");
    for (unsigned i = 0; i < 32; ++i) {
        rf_step(&rf,false,i,31-i,i,0,"RF_HOLD_PORTS");
        rf_step(&rf,false,31-i,i,i,UINT32_MAX,"RF_SWAP");
    }
    rf_step(&rf,true,0,0,0,UINT32_MAX,"RF_X0_WRITE");
    rf_step(&rf,false,0,31,0,0,"RF_X0_READ");
    rf_step(&rf,false,31,0,0,0,"RF_X0_READ");
    for (unsigned j = 0; j < 38; ++j) {
        uint32_t v = j < 6 ? vals[j] : UINT32_C(1) << (j-6); unsigned i = 1+j%31;
        rf_step(&rf,true,i,31,i,v,"RF_OVERLAP1");
        rf_step(&rf,true,0,i,i,~v,"RF_OVERLAP2");
        rf_step(&rf,true,i,i,i,v,"RF_OVERLAP_BOTH");
        rf_step(&other,true,i,i,i,~v,"RF_INSTANCE_WRITE");
        rf_step(&rf,false,i,i,0,0,"RF_INSTANCE_READ");
    }
    /* Repeated full sequences exercise state retention beyond a single call. */
    for (unsigned i = 0; i < 4; ++i) {
        const unsigned en[] = {1,0,1,0}; const uint32_t data[] = {100,999,200,0};
        rf_step(&rf,en[i] != 0,1,1,1,data[i],"RF_SEQUENCE");
    }
}
