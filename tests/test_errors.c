#include "test_support.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static InstMemory error_im;
static DataMemory error_dm;
static const char *error_unit;
static unsigned error_number;
/* C17 atexit observes state at a diagnostic exit, without OS-specific traps.
   This proves final state preservation, not the absence of transient reads. */
static void unchanged_at_exit(void)
{
    for (unsigned i = 0; i < COMPSYS_MEMORY_WORDS; ++i) {
        uint32_t expected = UINT32_C(0x81230000)+i;
        if (error_im.mem[i] != expected || error_dm.mem[i] != expected) {
            fprintf(stderr,"[FAIL] E_%s_%u_STATE index=%u expected=%08" PRIx32
                    " im_actual=%08" PRIx32 " dm_actual=%08" PRIx32 "\n",
                    error_unit,error_number,i,expected,error_im.mem[i],error_dm.mem[i]);
            fflush(stderr); _Exit(5);
        }
    }
    fputs("[PASS] E_STATE unchanged at diagnostic exit\n",stderr);
}
int test_error(int argc, char **argv)
{
    if (argc != 4) return 2;
    const char *unit = argv[2]; unsigned k = (unsigned)strtoul(argv[3],NULL,10);
    uint32_t base = !strcmp(unit,"im") || !strcmp(unit,"setter") || !strcmp(unit,"index-im") ? INST_MEM_START : DATA_MEM_START;
    uint32_t invalid[] = {base-4,base+1,base+2,base+3,base+4096,UINT32_MAX};
    if (k > 5) return 2;
    uint32_t addr = invalid[k]; Word out = test_word(UINT32_MAX);
    error_unit = unit; error_number = k;
    for (unsigned i = 0; i < COMPSYS_MEMORY_WORDS; ++i)
        error_im.mem[i] = error_dm.mem[i] = UINT32_C(0x81230000)+i;
    if (atexit(unchanged_at_exit) != 0) return 7;
    printf("CASE E_%s_%u address=%08" PRIx32 " base=%08" PRIx32 " expected=abnormal\n",unit,k,addr,base); fflush(stdout);
    if (!strcmp(unit,"index-im") || !strcmp(unit,"index-dm")) (void)checked_memory_index(addr,base,base == INST_MEM_START ? "InstMemory" : "DataMemory");
    else if (!strcmp(unit,"setter")) inst_memory_set_inst(&error_im,addr,123);
    else if (!strcmp(unit,"im")) inst_memory_run(&error_im,test_word(addr),&out);
    else if (!strcmp(unit,"dm-read")) data_memory_run(&error_dm,false,true,test_word(addr),test_word(123),&out);
    else if (!strcmp(unit,"dm-write")) data_memory_run(&error_dm,true,false,test_word(addr),test_word(123),&out);
    else if (!strcmp(unit,"control")) checked_data_memory_control(true,true);
    else if (!strcmp(unit,"dm-both")) data_memory_run(&error_dm,true,true,test_word(k ? addr : base),test_word(123),&out);
    else return 2;
    fprintf(stderr,"[FAIL] E_%s_%u expected=nonreturn actual=returned output=%08" PRIx32 "\n",unit,k,test_value(out));
    return 5;
}
