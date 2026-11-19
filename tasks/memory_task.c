#include "compsys.h"
#include <stdio.h>
#include <stdlib.h>

/* TODO-BEGIN B_HELPERS */
/* Optional helpers belong here. */
/* TODO-END B_HELPERS */

void inst_memory_run(InstMemory *im, Word addr, Word *inst)
{
    /* TODO-BEGIN INST_MEMORY */
    (void)im; (void)addr; (void)inst;
    fputs("[UNFINISHED] INST_MEMORY\n",stderr);
    exit(3);
    /* TODO-END INST_MEMORY */
}

void data_memory_run(DataMemory *dm, Signal mem_write, Signal mem_read,
    Word addr, Word wdata, Word *rdata)
{
    /* TODO-BEGIN DATA_MEMORY */
    (void)dm; (void)mem_write; (void)mem_read;
    (void)addr; (void)wdata; (void)rdata;
    fputs("[UNFINISHED] DATA_MEMORY\n",stderr);
    exit(3);
    /* TODO-END DATA_MEMORY */
}
