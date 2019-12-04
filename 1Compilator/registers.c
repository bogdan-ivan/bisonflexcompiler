#include "registers.h"
#include "common.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>


static inline void err(const char *fmt, ...);

char const* const register_name[] = {
        "$r0", // reserved by assembler
        "$at", // reserved by assembler
        "$v0", // reserved: code for syscall (4 - print, 10 - exit)
        "$v1", // reserved: accumulator
        "$a0", // reserved: pointer to str for syscall 4 (print)
        "$a1", // reserved: accumulator
        "$a2",
        "$a3",
        "$t0",
        "$t1",
        "$t2",
        "$t3",
        "$t4",
        "$t5",
        "$t6",
        "$t7",
        "$s0",
        "$s1",
        "$s2",
        "$s3",
        "$s4",
        "$s5",
        "$s6",
        "$s7",
        "$t8",
        "$t9",
        "$k0",
        "$k1",
        "$gp",
        "$sp",
        "$s8",
        "$ra"
};

const char *special_registers[] = {
        "$HI",
        "$LO"
};

const char* regstr(REGISTER reg) {
    switch(reg) {
        case hi:
            return special_registers[hi_str];
        case lo:
            return special_registers[lo_str];
        default:
            if (!inrange(reg)) {
                err("Register ` %d ` is not defined", reg);
                exit(1);
            }
            return register_name[reg];
    }
}

uint8_t used_register[NELEMS(register_name)] = {1, 1, 1, 1, 1, 1}; // 1 for each reserved registers

int registers_count() {
    return NELEMS(register_name);
}

// reserved registers
int syscall_reg() {
    return v0;
}

int arg0_reg() {
    return a0;
}

// pick out next register
int next_available_register() {
    int i;
    for (i = 0; inrange(i); ++i) {
        if (!used_register[i]) {
            if (is_reserved(i)) {
                err("%d is reserved", i);
            }
            used_register[i] = true;
            return i;
        }
    }
    err("Unavailable memory, after %d iter.\n", i);
    exit(69); // unavailable memory
}

// "put back" register
void free_register(int index) {
    if (is_reserved(index)) {
        err("Cannot free reserved register.");
        exit(69); // unavailable
    }
    used_register[index] = false;
}

//
bool is_reserved(int index) {
    switch (index) {
        case r0:
        case at:
        case v0:
        case v1:
        case a0:
        case a1:
            return true;
    }
    return false;
}

// registers range
bool inrange(int index) {
    return index >= 0 && index < NELEMS(register_name);
}

// err printing
void err(const char *fmt, ...) {
    printf("Error: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
