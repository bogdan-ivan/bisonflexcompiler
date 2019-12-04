#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include "scope.h"
#include "buffer.h"
#include "AST.h"
#include "y.tab.h"
#include "codegen.h"
#include "common.h"

extern int yylineno;
static int cc;

#define val(value) \
    (Operand) { Val, .val = (value) }
#define reg(index) \
    { Reg, .reg = (index) }

bool semantic_error = false;
static void err(const char *fmt, ...);

void yield_identity(YieldType type);
void none_check(const char *err_msg);
// extract operand from yield
Operand extract_operand_none_checked(const char* errmsg);
Operand extract_operand();

// a <=> b ?
LOCAL bool same_oper(Operand *one, Operand *two);

// aux
Yield yield;
Instruction instr;
Operand none_oper;

Operand acc = reg(acc_reg);
Operand acc2 = reg(acc_reg2);
Operand h1 = reg(hi_reg);
Operand l0 = reg(lo_reg);
Operand zero = reg(zero_reg);

LOCAL Yield yield_val(int value);
LOCAL Yield yield_reg(REGISTER reg);
LOCAL Yield yield_none();
LOCAL Yield yield_op(Operand op);

LOCAL void make_instr(InstrType type, Operand op1, Operand op2, Operand op3);
LOCAL void make_binary_customizable(ASTNode *p, Operand perf_dest);

Yield generate(ASTNode *node, Operand *pref_dest) {
    int lbl1, lbl2;

    yield_none();

    if (!node) return yield_none();

    switch (node->type) {
        case LITERAL:
            return yield_val(node->literal.value);
        case ID: {
            if (!used_identifier_recursive(node->id.name)) {
                err("Undefined identifier ` %s `.", node->id.name);
                return yield_none();
            }
            return yield_reg(scope_register_index(node->id.name));
        }
        case OPER:
            switch (node->oper.type) {
                case IF: {
                    write("\t# IF\n");
                    Operand cond = (generate(node->oper.ops[0], &acc), extract_operand());
                    // acc2 = 0
                    make_instr(li, acc2, val(0), none_oper);

                    // if cond is rvalue save it in accumulator
                    if (Val == cond.type) {
                        make_instr(li, acc, cond, none_oper);
                        cond = acc;
                    }

                    if (buf_size(node->oper.ops) > 2) {
                        // if ... else
                        // (cond) == 0; jump to else
                        lbl1 = ++cc;
                        make_instr(beq, cond, acc2, val(lbl1));

                        // if-stmt
                        generate(node->oper.ops[1], &acc);
                        lbl2 = ++cc;
                        // fallthrough: jump after else
                        make_instr(jmp, val(lbl2), none_oper, none_oper);

                        // else
                        write("L%03d:\n", lbl1);
                        generate(node->oper.ops[2], &acc);

                        // end of if-else
                        write("L%03d:\n", lbl2);
                    } else {
                        // if ...
                        // (cond) == 0; jump to end
                        lbl1 = ++cc;
                        make_instr(beq, cond, acc2, val(lbl1));

                        generate(node->oper.ops[1], &acc);
                        // end of if
                        write("L%03d:\n", lbl1);
                    }

                    return yield_none();
                }

                case WHILE: {
                    write("\t# WHILE\n");
                    // loop label
                    lbl1 = ++cc;
                    write("L%03d:\n", lbl1);

                    // generate cond
                    Operand cond = (generate(node->oper.ops[0], &acc), extract_operand());
                    // if cond is rvalue save it in accumulator (!this is never called, i think emit_binary puts lvalues into registers)
                    if (Val == cond.type) {
                        make_instr(li, acc, cond, none_oper);
                        cond = acc;
                    }

                    // acc2 = 0
                    make_instr(li, acc2, val(0), none_oper);
                    // (cond) == 0; jump to end
                    lbl2 = ++cc;
                    make_instr(beq, cond, acc2, val(lbl2));

                    // generate body
                    generate(node->oper.ops[1], &acc);

                    // loop back
                    make_instr(jmp, val(lbl1), none_oper, none_oper);

                    // end
                    write("L%03d:\n", lbl2);
                    return yield_none();
                }
                case VAR:
                case '=': {
                    // left side must be an lvalue
                    if (ID != node->oper.ops[0]->type) {
                        err("Left hand side of assignment must be lvalue.\n");
                    }
                    char const *name = node->oper.ops[0]->id.name;

                    Yield toreturn;
                    Operand lhs;

                    if (VAR == node->oper.type) {
                        write("\t# VAR\n");
                        declare_identifier(name);
                        // return if no initializer
                        if (NULL == node->oper.ops[1]) { return yield_none(); }
                        // generate lhs (to pass as dest register)
                        lhs = (generate(node->oper.ops[0], &acc), extract_operand());
                        toreturn = yield_none();
                        // masquerade as '=' from now on
                        node->oper.type = '=';
                    } else {
                        write("\t# =\n");
                        if (!used_identifier_recursive(name)) {
                            err("Undefined identifier ` %s `.", name);
                        }
                        // generate lhs (to pass as dest register and return register)
                        lhs = (generate(node->oper.ops[0], &acc), extract_operand());
                        toreturn = yield;
                    }
                    make_binary_customizable(node, lhs);

                    return yield = toreturn;
                }

                case BLOCK: {
                    // setup new env
                    Environment prev = env;
                    env.identifiers = NULL;
                    env.prev = &prev;
                    // execute
                    generate(node->oper.ops[0], &acc);
                    // cleanup
                    destruct_environment();
                    env = prev;
                    break;
                    break;
                }
                case ';':
                    generate(node->oper.ops[0], &acc);
                    return generate(node->oper.ops[1], &acc);
                case '+':
                case '-':
                case '*': {
                    write("\t# %c\n", node->oper.type);
                    make_binary_customizable(node, *pref_dest);
                    return yield;
                }
                case '/':
                case '%': {
                    write("\t# %c\n", node->oper.type);
                    char op = (char) node->oper.type;
                    // '/' from now on
                    node->oper.type = '/';
                    make_binary_customizable(node, acc);

                    InstrType type = ('/' == op) ? mflo : mfhi;
                    make_instr(type, *pref_dest, none_oper, none_oper);

                    return yield_op(*pref_dest);
                }

                case '<': {
                    write("\t# %c\n", node->oper.type);
                    make_binary_customizable(node, *pref_dest);
                    return yield;
                }

                case '>': {
                    write("\t# %c\n", node->oper.type);
                    // swap rhs with lhs
                    ASTNode *aux;
                    aux = node->oper.ops[0];
                    node->oper.ops[0] = node->oper.ops[1];
                    node->oper.ops[1] = aux;

                    make_binary_customizable(node, *pref_dest);
                    return yield;
                }

                case NE: case EQ: {
                    write("\t# NE/EQ\n");
                    // lhs
                    Operand lhs = (generate(node->oper.ops[0], pref_dest),
                            extract_operand_none_checked("Logical operator: lhs is missing."));

                    // load lhs into register if it's a value
                    if (Val == lhs.type) {
                        make_instr(li, *pref_dest, lhs, none_oper);
                        lhs = *pref_dest;
                    }

                    // rhs
                    Operand rhs = (generate(node->oper.ops[1], pref_dest),
                            extract_operand_none_checked("Logical operator: rhs is missing."));

                    // !=
                    InstrType xor_type = (Val == rhs.type) ? xori : xor;
                    make_instr(xor_type, *pref_dest, lhs, rhs);

                    if (NE == node->oper.type) { return yield_op(*pref_dest); }

                    // acc2 = 0
                    make_instr(li, acc2, val(0), none_oper);
                    // `!=` ; jump to false
                    lbl1 = ++cc;
                    make_instr(bne, *pref_dest, acc2, val(lbl1));

                    // fallthrough: dest = true
                    make_instr(li, *pref_dest, val(1), none_oper);

                    // jump over false
                    lbl2 = ++cc;
                    make_instr(jmp, val(lbl2), none_oper, none_oper);

                    // false label
                    write("L%03d:\n", lbl1);

                    // dest = false
                    make_instr(li, *pref_dest, val(0), none_oper);

                    write("L%03d:\n", lbl2);
                    return yield_op(*pref_dest);
                }

                case LE: case GE: {
                    write("\t# LE/GE\n");
                    if (GE == node->oper.type) {
                        // swap rhs with lhs
                        ASTNode *aux;
                        aux = node->oper.ops[0];
                        node->oper.ops[0] = node->oper.ops[1];
                        node->oper.ops[1] = aux;
                    }

                    // lhs
                    Operand lhs = (generate(node->oper.ops[0], pref_dest),
                            extract_operand_none_checked("Logical operator: lhs is missing."));
                    // load lhs into register if it's a value
                    if (Val == lhs.type) {
                        make_instr(li, *pref_dest, lhs, none_oper);
                        lhs = *pref_dest;
                    }

                    // rhs
                    Operand rhs = (generate(node->oper.ops[1], pref_dest),
                            extract_operand_none_checked("Logical operator: rhs is missing."));
                    // load rhs into register if it's a value
                    if (Val == rhs.type) {
                        make_instr(li, acc2, rhs, none_oper);
                        rhs = acc2;
                    }

                    if (same_oper(&lhs, &rhs)) {
                        err("Overwriting data. [Debug]\n");
                        exit(2);
                    }

                    // dest = lhs - rhs
                    make_instr(sub, *pref_dest, lhs, rhs);

                    // if dest <= 0 then lhs <= rhs -> branch to true
                    lbl1 = ++cc;
                    make_instr(blez, *pref_dest, val(lbl1), none_oper);

                    // fallthrough: dest = false
                    make_instr(li, *pref_dest, val(0), none_oper);

                    // jump to the end
                    lbl2 = ++cc;
                    make_instr(jmp, val(lbl2), none_oper, none_oper);

                    // dest = true
                    write("L%03d:\n", lbl1);
                    make_instr(li, *pref_dest, val(1), none_oper);

                    // end
                    write("L%03d:\n", lbl2);

                    yield.type = OP;
                    yield.oper = *pref_dest;
                    return yield_op(*pref_dest);
                }

                case AND: case OR: {
                    node->oper.type = (AND == node->oper.type) ? '&' : '|';
                    const char op = (char)node->oper.type;
                    write("\t# %c\n", op);

                    // get lhs
                    Operand lhs = (generate(node->oper.ops[0], pref_dest),
                            extract_operand_none_checked("Logical operator: lhs is missing."));
                    // if rhs is not in pref_dest move,li there
                    if (Val == lhs.type) {
                        make_instr(li, *pref_dest, lhs, none_oper);
                        lhs = *pref_dest;
                    } else if (!same_oper(pref_dest, &lhs)) { // useless?
                        make_instr(move, *pref_dest, lhs, none_oper);
                        lhs = *pref_dest;
                    }

                    // AND lhs == 0 => jump
                    // OR  lhs != 0 => jump
                    InstrType shortcut = ('&' == op) ? beq : bne;

                    // acc2 = 0
                    make_instr(li, acc2, val(0), none_oper);
                    // (cond) == 0; jump to end
                    lbl1 = ++cc;
                    make_instr(shortcut, lhs, acc2, val(lbl1));

                    // rhs
                    Operand rhs = (generate(node->oper.ops[1], pref_dest),
                            extract_operand_none_checked("Logical operator: rhs is missing."));

                    // if rhs is not in pref_dest move,li there
                    if (Val == rhs.type) {
                        make_instr(li, *pref_dest, rhs, none_oper);
                        rhs = *pref_dest;
                    } else if (!same_oper(pref_dest, &rhs)) {  // useless?
                        make_instr(move, *pref_dest, rhs, none_oper);
                        rhs = *pref_dest;
                    }

                    // end
                    write("L%03d:\n", lbl1);
                    // return last value
                    return yield_op(*pref_dest);
                }
            }
    }

    return yield_none();
}

void make_binary_customizable(ASTNode *p, Operand perf_dest) {
    static char left_missing[] = "Operator '?' is missing left hand side.\n";
    static char right_missing[] = "Operator '?' is missing right hand side.\n";
    static int op_index = 10;
    // get operator as char
    const char op = (char) p->oper.type;
    if (op != p->oper.type) {
        err("Not 1 char operator.\n");
        exit(1);
    }
    // replace char in strings
    left_missing[op_index] = right_missing[op_index] = (char) p->oper.type;

    // get lhs
    Operand lhs = (generate(p->oper.ops[0], &acc),
            extract_operand_none_checked(left_missing));

    // for operator '='
    if ('=' == p->oper.type) {
        // enable assign optimization
        generate(p->oper.ops[1], &perf_dest);
    } else {
        generate(p->oper.ops[1], &acc);
    }

    // get rhs
    Operand rhs = extract_operand_none_checked(right_missing);

    // if dest is different from lhs (ex: first = first * 3; here `dest` is same as `lhs`)
    // load lhs into dest
//    if (!same_oper(&lhs, &perf_dest)) { // useless??
//        // copy first value to reg
//        cp_type = (Val == lhs.type) ? li : move;
//        make_instr(cp_type, perf_dest, lhs, none_oper);
//        // lhs is now in perf_dest
//        lhs = perf_dest;
//    }
    if (Val == lhs.type) {
        make_instr(li, perf_dest, lhs, none_oper);
        lhs = perf_dest;
    }
    // Resolve instruction.
    // branch on immediate or register
    InstrType instr_type;
    // Branch on immediate (there are instr. that do not support immediate)
    if (Reg == rhs.type) {
        instr_type = reg_instr[op];
    } else {
        instr_type = val_instr[op];

        // if there's no immediate instruction
        if (err_instr == instr_type) {
            make_instr(li, acc2, rhs, none_oper);
            // rhs is now in acc2
            rhs = acc2;
            instr_type = reg_instr[op];
        }
    }

    // ex: first = first * 3; first = first (this one is discarded =see "avoid..."=)
    if (same_oper(&lhs, &perf_dest) && '=' == op) {
        lhs = rhs;
        rhs = none_oper;

        // avoid move $x, $x
        if (same_oper(&lhs, &perf_dest)) {
            return;
        }
    }

    if ('/' == op) {
        make_instr(instr_type, lhs, rhs, none_oper);
        return;
    } else {
        // add second to first
        make_instr(instr_type, perf_dest, lhs, rhs);
    }
    // update return
    yield.type = OP;
    yield.oper = perf_dest;
}

void compile(ASTNode *root) {
    Yield ret = generate(root, &acc);
    if (None_yield != ret.type) {
        // discard
    }
}

// check if two operands are the same
bool same_oper(Operand *one, Operand *two) {
    if (NULL == one || NULL == two) {
        err("Null input. <%s>", __FUNCTION__);
        exit(1);
    }
    if (one->type != two->type || one->reg != two->reg) { // last check includes val != val
        return false;
    }
    return true;
}

// check yield.type
void yield_identity(YieldType type) {
    if (type != yield.type) {
        err("Mismatched yield: expected ` %d `; got ` %d `", type, yield.type);
        semantic_error = true;
    }
}

void make_instr(InstrType type, Operand op1, Operand op2, Operand op3) {
    instr.type = type;
    instr.op1 = op1;
    instr.op2 = op2;
    instr.op3 = op3;
    emit_instruction(&instr);
}

Operand extract_operand() {
    yield_identity(OP);
    return yield.oper;
}

Operand extract_operand_none_checked(const char* errmsg) {
    none_check(errmsg);
    return extract_operand();
}

LOCAL Yield yield_val(int value) {
    yield.type = OP;
    yield.oper.type = Val;
    yield.oper.val = value;
    return yield;
}

LOCAL Yield yield_reg(REGISTER reg) {
    yield.type = OP;
    yield.oper.type = Reg;
    yield.oper.reg = reg;
    return yield;
}

LOCAL Yield yield_none() {
    yield.type = None_yield;
    return yield;
}

LOCAL Yield yield_op(Operand op) {
    yield.type = OP;
    yield.oper = op;
    return yield;
}

void err(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf(" Line: %d\n", yylineno);
    semantic_error = true;
}

bool error() {
    return semantic_error;
}

void none_check(const char *err_msg) {
    if (None_yield == yield.type) {
        err(err_msg);
        exit(2);
    }
}