#include "compsys.h"
void register_set_value(Register *reg, uint32_t value) { reg->val = value; }
uint32_t register_get_value(const Register *reg) { return reg->val; }
