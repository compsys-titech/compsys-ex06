#include "test_support.h"
#include <stdio.h>
int main(void)
{
    RegisterFile rf = {{{0}}}; Signal select[5]; test_select(1,select);
    const uint32_t data[] = {100,999,200,0}, old[] = {0,100,100,200}, next[] = {100,100,200,200};
    const Signal enable[] = {true,false,true,false}; test_begin("example-register-file");
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t before = rf.r[1].val; Word a = test_word(UINT32_MAX), b = test_word(0); char in[80];
        snprintf(in,sizeof in,"step=%u W=%u r1=r2=w=1 data=%u",i+1,(unsigned)enable[i],(unsigned)data[i]);
        register_file_run(&rf,enable[i],select,select,select,test_word(data[i]),&a,&b);
        test_case("EX_RF_P1","slide p24; actual port1",in,old[i],test_value(a),before,rf.r[1].val,next[i],1);
        test_case("EX_RF_P2","slide p24; actual port2",in,old[i],test_value(b),before,rf.r[1].val,next[i],1);
    }
    return test_finish();
}
