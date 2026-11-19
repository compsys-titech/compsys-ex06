#include "test_support.h"
#include <stdio.h>
int main(void)
{
    Register reg = {0}; const uint32_t data[] = {100,999,200,0};
    const Signal enable[] = {true,false,true,false}; const uint32_t old[] = {0,100,100,200};
    const uint32_t next[] = {100,100,200,200};
    test_begin("example-register");
    for (unsigned i = 0; i < 4; ++i) {
        uint32_t before = reg.val; Word out = test_word(UINT32_MAX); char in[80];
        snprintf(in,sizeof in,"step=%u W=%u data=%u",i+1,(unsigned)enable[i],(unsigned)data[i]);
        register_run(&reg,enable[i],test_word(data[i]),&out);
        test_case("EX_R","slide p14; actual task output",in,old[i],test_value(out),before,reg.val,next[i],1);
    }
    return test_finish();
}
