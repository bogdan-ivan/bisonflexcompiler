#ifndef __codegen_h
#define __codegen_h

#include "registers.h"
#include <stdarg.h>

#define None(tail) \
	None_ ## tail = 0

typedef enum Index3 Index3;
typedef enum OperandType OperandType;
typedef enum YieldType YieldType;
typedef enum InstrType InstrType;

typedef struct Operand Operand;
typedef struct Instruction Instruction;
typedef struct Yield Yield;

enum InstrType {
    err_instr = -1,
    None(instr),
    // arithmetic
    add,
    addi,
    sub,
    mul,
    div_instr,
    mfhi,
    mflo,

    // memory
    li,
    move,

    // branching
    jmp,
    beq,
    bne,
    blez,

    // logical
    and,
    andi,
    or,
    ori,
    xor,
    xori,

    //
    slt,
    slti,

    eq,
    eqi,
    ne,
    nei
};

enum Index3 {
    first = 1,
    second = 2,
    third = 3
};

enum OperandType {
    None(operand),
    Val,
    Reg
};

enum YieldType {
    None(yield),
    Instr,
    OP
};

struct Operand {
    OperandType type;
    union {
        int val;
        REGISTER reg;
    };
};

struct Instruction {
    InstrType type;
    Operand op1;
    Operand op2;
    Operand op3;
};

struct Yield {
    YieldType type;
    union {
        Instruction instr;
        Operand oper;
    };
};

void setup_codegen(const char *output_file_path);
void close_codegen();

const char *instr_tostr(InstrType type);

void write(const char *fmt, ...);
void emit_instruction(Instruction *instr);

extern InstrType reg_instr[];
extern InstrType val_instr[];

#endif // !_codegen_h