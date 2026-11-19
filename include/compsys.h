#ifndef COMPSYS_H
#define COMPSYS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef bool Signal;
typedef struct { Signal bit[32]; } Word;
void word_set_value(Word *w, uint32_t value);
uint32_t word_get_value(Word w);
#define COMPSYS_MEMORY_WORDS 1024u
#define INST_MEM_START UINT32_C(0x00400000)
#define DATA_MEM_START UINT32_C(0x10010000)
typedef struct { uint32_t val; } Register;
typedef struct { Register r[32]; } RegisterFile;
typedef struct { uint32_t mem[COMPSYS_MEMORY_WORDS]; } InstMemory;
typedef struct { uint32_t mem[COMPSYS_MEMORY_WORDS]; } DataMemory;
void register_run(Register *reg, Signal wctl, Word wdata, Word *rdata);
void decoder5(Signal a[5], Word *b);
void mux32(Word ins[32], Signal ctls[5], Word *out);
void register_file_run(RegisterFile *rf, Signal register_write,
    Signal read1[5], Signal read2[5], Signal write1[5], Word wdata,
    Word *rdata1, Word *rdata2);
void register_set_value(Register *reg, uint32_t value);
uint32_t register_get_value(const Register *reg);
void inst_memory_run(InstMemory *im, Word addr, Word *inst);
void data_memory_run(DataMemory *dm, Signal mem_write, Signal mem_read,
    Word addr, Word wdata, Word *rdata);
void inst_memory_set_inst(InstMemory *im, uint32_t addr, uint32_t inst);
uint32_t checked_memory_index(uint32_t address, uint32_t base, const char *component);
void checked_data_memory_control(Signal mem_write, Signal mem_read);
#endif
