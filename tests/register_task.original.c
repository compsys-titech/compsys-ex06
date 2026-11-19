#include "compsys.h"
#include <stdio.h>
#include <stdlib.h>

/* TODO-BEGIN A_HELPERS */
/* Optional helpers belong here. */
/* TODO-END A_HELPERS */

void register_run(Register *reg, Signal wctl, Word wdata, Word *rdata)
{
    /* TODO-BEGIN REGISTER */
    (void)reg; (void)wctl; (void)wdata; (void)rdata;
    fputs("[UNFINISHED] REGISTER\n",stderr);
    exit(3);
    /* TODO-END REGISTER */
}

void decoder5(Signal a[5], Word *b)
{
    /* TODO-BEGIN DECODER5 */
    (void)a; (void)b;
    fputs("[UNFINISHED] DECODER5\n",stderr);
    exit(3);
    /* TODO-END DECODER5 */
}

void mux32(Word ins[32], Signal ctls[5], Word *out)
{
    /* TODO-BEGIN MUX32 */
    (void)ins; (void)ctls; (void)out;
    fputs("[UNFINISHED] MUX32\n",stderr);
    exit(3);
    /* TODO-END MUX32 */
}

void register_file_run(RegisterFile *rf, Signal register_write,
    Signal read1[5], Signal read2[5], Signal write1[5], Word wdata,
    Word *rdata1, Word *rdata2)
{
    /* TODO-BEGIN REGISTER_FILE */
    (void)rf; (void)register_write; (void)read1; (void)read2;
    (void)write1; (void)wdata; (void)rdata1; (void)rdata2;
    fputs("[UNFINISHED] REGISTER_FILE\n",stderr);
    exit(3);
    /* TODO-END REGISTER_FILE */
}
