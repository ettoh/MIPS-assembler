#ifndef MIPS_DEFINITIONS_H
#define MIPS_DEFINITIONS_H

#include <map>

enum {
    INSTR_TYPE_R,
    INSTR_TYPE_R_SHIFT,
    INSTR_TYPE_I,
    INSTR_TYPE_J,
    INSTR_TYPE_NULL
};

struct InstructionCodes {
    uint32_t op_code = 0x00;
    uint32_t format = 0x00;
    uint32_t function = 0x00;
};

const std::map<std::string, InstructionCodes> INSTR_CODES = {
    {"add", {0x00, INSTR_TYPE_R, 0x20}},        // R, 0x20
    {"sub", {0x00, INSTR_TYPE_R, 0x22}},        // R, 0x22
    {"and", {0x00, INSTR_TYPE_R, 0x24}},        // R, 0x24
    {"or", {0x00, INSTR_TYPE_R, 0x25}},         // R, 0x25
    {"nor", {0x00, INSTR_TYPE_R, 0x27}},        // R, 0x27
    {"slt", {0x00, INSTR_TYPE_R, 0x2A}},        // R, 0x2A
    {"lw", {0x23, INSTR_TYPE_I, 0x00}},         // I, NA
    {"sw", {0x2B, INSTR_TYPE_I, 0x00}},         // I, NA
    {"beq", {0x04, INSTR_TYPE_I, 0x00}},        // I, NA
    {"addi", {0x08, INSTR_TYPE_I, 0x00}},       // I, NA
    {"sll", {0x00, INSTR_TYPE_R_SHIFT, 0x00}},  // R, 0x00
    {"j", {0x02, INSTR_TYPE_J, 0x00}},          // J, NA
    {"jr", {0x00, INSTR_TYPE_R, 0x08}},         // R, 0x08
    {"nop", {0x00, INSTR_TYPE_NULL, 0x00}}};

// register abbreviations
const std::map<std::string, uint32_t> REGISTER_ABRV = {
    {"$zero", 0}, {"$at", 1},  {"$v0", 2},  {"$v1", 3},  {"$a0", 4},
    {"$a1", 5},   {"$a2", 6},  {"$a3", 7},  {"$t0", 8},  {"$t1", 9},
    {"$t2", 10},  {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14},
    {"$t7", 15},  {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19},
    {"$s4", 20},  {"$s5", 21}, {"$s6", 22}, {"$s7", 23}, {"$t8", 24},
    {"$t9", 25},  {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$sp", 29},
    {"$fp", 30},  {"$ra", 31}};

#endif
