#ifndef __registers_h
#define __registers_h

#include <stdbool.h>

bool inrange(int index);
bool is_reserved(int index);

int next_available_register();
void free_register(int index);


#define REGISTER_COUNT 31

typedef enum REGISTER {
    r0, // reserved by assembler
    at, // reserved by assembler
    v0, // reserved: code for syscall (4 - print, 10 - exit)
    v1, // reserved: accumulator
    a0, // reserved: pointer to str for syscall 4 (print)
    a1, // reserved: accumulator
    a2,
    a3,
    t0,
    t1,
    t2,
    t3,
    t4,
    t5,
    t6,
    t7,
    s0,
    s1,
    s2,
    s3,
    s4,
    s5,
    s6,
    s7,
    t8,
    t9,
    k0,
    k1,
    gp,
    sp,
    s8,
    ra,
    hi, // special registers
    lo  // special registers
} REGISTER;

#define acc_reg v1
#define acc_reg2 a1
#define hi_reg hi
#define lo_reg lo
#define zero_reg r0

enum SPECIAL_REGISTERS_INDICES {
    hi_str,
    lo_str
};

const char* regstr(REGISTER reg);

#endif // !__registers_h