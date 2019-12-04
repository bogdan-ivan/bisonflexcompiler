#include "codegen.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "AST.h"
#include "y.tab.h"

InstrType reg_instr[] = {
        ['='] = move,
        ['+'] = add,
        ['-'] = sub,
        ['*'] = mul,
        ['/'] = div_instr,
        ['&'] = and,
        ['|'] = or,
        ['<'] = slt,
        ['>'] = slt,
        ['x'] = xor,
        [EQ] = eq,
        [NE] = ne,
        [VAR] = move
};

InstrType val_instr[] = {
        ['='] = li,
        ['+'] = addi,
        ['-'] = err_instr,
        ['*'] = err_instr,
        ['/'] = err_instr,
        ['&'] = andi,
        ['|'] = ori,
        ['<'] = slti,
        ['>'] = slti,
        ['x'] = xori,
        [EQ] = eq,
        [NE] = ne,
        [VAR] = li
};


// debug
bool one_time = false;
//
FILE *output;

static bool error = false;

LOCAL void err_check(const char *err_msg_fmt, ...);
LOCAL void err(const char *fmt, ...);

LOCAL void oper_identity(Operand ok, OperandType type);
LOCAL void instr_identity(Instruction *instr, InstrType type);

LOCAL REGISTER extract_reg(Operand oper, const char* err_msg);
LOCAL int extract_val(Operand oper, const char* err_msg);

LOCAL void emit_setup_code();
LOCAL void emit_close_code();

void binary_op(char op, Instruction *instr);

void emit_instruction(Instruction *instr) {
    if (NULL == instr) {
        err("Null input. <%s>", __FUNCTION__);
        exit(65);
    }

    switch (instr->type) {
        case jmp: {
            int label;
            label = extract_val(instr->op1, "Missing label.\n");
            write("\t%s L%03d\n", instr_tostr(instr->type), label);
            return;
        }
        case beq: case bne: {
            REGISTER rhs, lhs;
            int label;

            lhs = extract_reg(instr->op1, "Missing left hand side.");
            rhs = extract_reg(instr->op2, "Missing right hand side.");
            label = extract_val(instr->op3, "Missing label.");

            write("\t%s %s, %s, L%03d\n",
                    instr_tostr(instr->type),
                    regstr(lhs), regstr(rhs), label);
            return;
        }

        case blez: {
            REGISTER op;
            int label;

            op = extract_reg(instr->op1, "Missing lhs of blez.\n");
            label = extract_val(instr->op2, "Missing label of blez.\n");

            write("\t%s %s, L%03d\n", instr_tostr(instr->type), regstr(op), label);
            return;
        }

        case li: {
            REGISTER lvalue;
            int rvalue;

            // check instruction data
            instr_identity(instr, li);

            // extract data
            lvalue = extract_reg(instr->op1, "Left hand side must be an lvalue expression.");
            rvalue = extract_val(instr->op2, "Right hand side must be an rvalue expression.");

            // emit instruction
            write("\t%s %s, %d\n", instr_tostr(instr->type), regstr(lvalue), rvalue);
            return;
        }

        case move: {
            REGISTER lhs, rhs;

            // check instruction data
            instr_identity(instr, move);

            // extract data
            lhs = extract_reg(instr->op1, "Left hand side must be an lvalue expression.");
            rhs = extract_reg(instr->op2, "Right hand side must be an lavlue expression.");

            // emit instruction
            write("\t%s %s, %s\n", instr_tostr(instr->type), regstr(lhs), regstr(rhs));
            return;
        }

        case addi: case add: {
            binary_op('+', instr);
            return;
        }

        case sub: {
            binary_op('-', instr);
            return;
        }
        case mul: {
            binary_op('*', instr);
            return;
        }

        case div_instr: {
            binary_op('/', instr);
            return;
        }

        case slt: case slti: {
            binary_op('<', instr);
            return;
        }

        case xor: case xori: {
            binary_op('x', instr);
            return;
        }

        case mfhi: case mflo: {
            REGISTER dest;
            dest = extract_reg(instr->op1, "Missing dest.");
            // emit instruction
            write("\t%s %s\n", instr_tostr(instr->type), regstr(dest));
            return;
        }

        case err_instr: {
            err("Instruction operands have wrong types (reg/val). <%s>", __FUNCTION__);
            exit(3);
        }

        case None_instr:
            err("None passed as instruction\n");
            exit(2);

        default:
            err("Not implemented. <%s>\n", __FUNCTION__);
            exit(0);
    }
}

void binary_op(char op, Instruction *instr) {
    REGISTER dest;
    REGISTER lhs;
    int rhs; // could be either REGISTER or val

    // check instruction data
    InstrType type = (Val == instr->op3.type) ? val_instr[op] : reg_instr[op];
    instr_identity(instr, type);

    // extract data
    dest = extract_reg(instr->op1, "Missing dest");
    lhs = extract_reg(instr->op2, "Missing lhs.");

    if ('/' == op) {
        write("\t%s %s, %s\n", instr_tostr(instr->type), regstr(dest), regstr(lhs));
        return;
    }
    if (reg_instr[op] == instr->type) {
        rhs = extract_reg(instr->op3, "Missing rhs rvalue.");
        write("\t%s %s, %s, %s\n", instr_tostr(instr->type), regstr(dest), regstr(lhs), regstr(rhs));
    }
    else {
        rhs = extract_val(instr->op3, "Missing rhs lvalue.");
        write("\t%s %s, %s, %d\n", instr_tostr(instr->type), regstr(dest), regstr(lhs), rhs);
    }
}

// Validate operand type.
void oper_identity(Operand ok, OperandType type) {
    if (ok.type != type) {
        err("%s type failed.", __FUNCTION__);
        error = true;
    }
}

// Validate instruction type.
void instr_identity(Instruction *instr, InstrType type) {
    if (NULL == instr) {
        err("Null input. <%s>", __FUNCTION__);
        exit(65);
    }
    if (instr->type != type) {
        err_check("Mismatched instruction type: expected ` %s `; got ` %s `.", type, instr->type);
        error = true;
    }
    if (err_instr == instr->type) {
        err("Err: err_instr input. <%s>", __FUNCTION__);
        exit(3);
    }
}

// Check the operand type and return the corresponding data.
REGISTER extract_reg(Operand oper, const char *err_msg) {
    oper_identity(oper, Reg);
    err_check("%s\n", err_msg);
    return oper.reg;
}
int extract_val(Operand oper, const char *err_msg) {
    oper_identity(oper, Val);
    err_check("%s\n", err_msg);
    return oper.val;
}

// Instruction to string
const char *const instr_name[] = {
        [add] = "add",
        [addi] = "addi",
        [sub] = "sub",
        [mul] = "mul",
        [div_instr] = "div",
        [mfhi] = "mfhi",
        [mflo] = "mflo",

        [li] = "li",
        [move] = "move",

        [jmp] = "j",
        [beq] = "beq",
        [bne] = "bne",
        [blez] = "blez",

        [and] = "and",
        [andi] = "andi",
        [or] = "or",
        [ori] = "ori",
        [xor] = "xor",
        [xori] = "xori",

        [slt] = "slt",
        [slti] = "slti"
};

const char *instr_tostr(InstrType type) {
    if (type >= NELEMS(instr_name) || type < 0) {
        err("Out of bounds access: %d > %d (size). [Debug]", type, NELEMS(instr_name));
        exit(1);
    }
    if (NULL == instr_name[type]) {
        err("Not implemented %d. <%s>", type, __FUNCTION__);
        exit(0);
    }
    return instr_name[type];
}

// Output printing
void write(const char *fmt, ...) {
    if (NULL == output) {
        err("No output file");
        exit(1);
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);
    fflush(output);
}

// Error printing
void err(const char *fmt, ...) {
    printf("Error: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

LOCAL void err_check(const char *err_msg_fmt, ...) {
    if (error) {
        printf("Error: ");
        va_list args;
        va_start(args, err_msg_fmt);
        vprintf(err_msg_fmt, args);
        va_end(args);
        exit(1);
    }
}

// Initial assembly setup
void setup_codegen(const char *output_file_path) {
    if (one_time) {
        err("Double setup [Debug]");
        exit(1);
    }

    if ((output = fopen(output_file_path, "w")) == NULL) {
        err("File ` %s ` not found.", output_file_path);
        exit(69);
    }

    emit_setup_code();

    one_time = true;
}

// Closing assembly setup + free file descriptor
void close_codegen() {
    if (NULL != output) {
        emit_close_code();
        fclose(output);
    }
}

void emit_close_code() {
    write("\tj exit\n");
}

void emit_setup_code() {
    puts("Out...");
    write("\t.text\n");
    write("\t.globl main\n");

    write("exit:\n");
    write("\tli $v0, 10\n");
    write("\tsyscall\n");

    write("main:\n");

    fflush(output);
}
