#include "compsys.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
static void address_error(uint32_t address, uint32_t base,
                          const char *component, const char *id)
{
    fprintf(stderr,"[ADDRESS] id=%s component=%s address=0x%08" PRIx32
                   " base=0x%08" PRIx32 "\n",id,component,address,base);
    exit(10);
}
uint32_t checked_memory_index(uint32_t address, uint32_t base, const char *component)
{
    if (address < base) address_error(address,base,component,"ADDR_BELOW");
    uint32_t offset = address - base;
    if (offset % UINT32_C(4) != 0) address_error(address,base,component,"ADDR_ALIGN");
    uint32_t index = offset / UINT32_C(4);
    if (index >= COMPSYS_MEMORY_WORDS) address_error(address,base,component,"ADDR_ABOVE");
    return index;
}
void checked_data_memory_control(Signal mem_write, Signal mem_read)
{
    if (mem_write && mem_read) {
        fputs("[CONTROL] id=CTRL_BOTH component=DataMemory mem_write=1 mem_read=1\n",stderr);
        exit(11);
    }
}
void inst_memory_set_inst(InstMemory *im, uint32_t addr, uint32_t inst)
{
    uint32_t index = checked_memory_index(addr,INST_MEM_START,"InstMemory");
    im->mem[index] = inst;
}
