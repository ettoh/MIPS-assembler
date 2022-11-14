#ifndef MIPS_DEFINITIONS_H
#define MIPS_DEFINITIONS_H

#include <map>

enum
{
    INSTR_TYPE_R,
    INSTR_TYPE_I,
    INSTR_TYPE_J,
    INSTR_TYPE_NULL
};

struct InstructionCodes
{
    uint32_t op_code = 0x00;
    uint32_t format = 0x00;
    uint32_t function = 0x00;
};

std::map<std::string, InstructionCodes> instr_codes = {
    {"add", {0x00, INSTR_TYPE_R, 0x20}},  // R, 0x20
    {"sub", {0x00, INSTR_TYPE_R, 0x22}},  // R, 0x22
    {"and", {0x00, INSTR_TYPE_R, 0x24}},  // R, 0x24
    {"or", {0x00, INSTR_TYPE_R, 0x25}},   // R, 0x25
    {"nor", {0x00, INSTR_TYPE_R, 0x27}},  // R, 0x27
    {"slt", {0x00, INSTR_TYPE_R, 0x2A}},  // R, 0x2A
    {"lw", {0x23, INSTR_TYPE_I, 0x00}},   // I, NA
    {"sw", {0x2B, INSTR_TYPE_I, 0x00}},   // I, NA
    {"beq", {0x04, INSTR_TYPE_I, 0x00}},  // I, NA
    {"addi", {0x08, INSTR_TYPE_I, 0x00}}, // I, NA
    {"sll", {0x00, INSTR_TYPE_R, 0x00}},  // R, 0x00
    {"j", {0x02, INSTR_TYPE_J, 0x00}},    // J, NA
    {"jr", {0x00, INSTR_TYPE_R, 0x08}},   // R, 0x08
    {"nop", {0x00, INSTR_TYPE_NULL, 0x00}}};

#endif
