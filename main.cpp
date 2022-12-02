#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

#include "definitions.hpp"

void firstPass(std::ifstream& fileReader,
               std::ofstream& outputFile,
               std::map<std::string, unsigned int>& labelAddrMap) {
    std::string currentLine;
    unsigned int addrPointer = 0;
    std::smatch match;

    while (getline(fileReader, currentLine)) {
        bool labelSingle = false;
        // Looking for lines with codes
        if (std::regex_search(currentLine, std::regex(R"(^\s*[^\s#]+\s*#*)"))) {
            // Looking for code without comments (we keep everything before a #)
            if (std::regex_search(currentLine, match, std::regex("[^#]*"))) {
                std::string lineWithoutComments = match.str(0);
                // Looking for a label name
                if (std::regex_search(
                        lineWithoutComments, match, std::regex("\\S*(?=:)"))) {
                    labelAddrMap[match.str(0)] = addrPointer;

                    // Looking if there is no code after the label
                    if (!std::regex_search(lineWithoutComments,
                                           std::regex(":[^#]*[^#\\s]#*"))) {
                        labelSingle = true;
                    }
                }
                if (!labelSingle) addrPointer += 4;
            }
        }
    }
}

// --------------------------------------------------------

uint32_t regCode(const std::string& s) {
    if (std::regex_match(
            s, std::regex("[$](zero|\d{2}|[a-z]{1}\d{1}|[a-z]{2})"))) {
        // TODO error handling
        // register string invalid
        return 0u;
    }

    uint32_t register_number = 0;
    if (s[1] >= 48 &&
        s[1] <= 57) {  // there is a number after $ -> no abbreviations
        register_number = stoi(s.substr(1, s.length() - 1));
        if (register_number < 0 || register_number > 31) {
            // TODO error handling
            // register out of range
            return 0u;
        }
        return register_number;
    }

    // register abbreviations
    const auto result = REGISTER_ABRV.find(s);
    if (result == REGISTER_ABRV.end()) {
        // register abbreviation is not supported!
        // TODO error handling
        return 0u;
    }

    return result->second;
}

// --------------------------------------------------------

uint32_t binInstruction(const std::vector<std::string>& instruction_parts) {
    size_t argument_cnt = instruction_parts.size();
    if (argument_cnt == 0) {
        // TODO error handling
        return 0u;
    }

    // find codes and layout for instruction
    const auto result = INSTR_CODES.find(instruction_parts[0]);
    if (result == INSTR_CODES.end()) {
        // Instruction is not supported!
        // TODO error handling
        return 0u;
    }
    InstructionCodes instruction_codes = result->second;

    // 1. op-code - first (left) 6 bits
    uint32_t binary_instr = 0x00000000;
    binary_instr |= instruction_codes.op_code << 26;

    // 2. TODO rest ....
    switch (instruction_codes.format) {
        case INSTR_TYPE_R:
            if (argument_cnt == 2) {  // jr instruction
                binary_instr |= regCode(instruction_parts[1]) << 21;
                binary_instr |= instruction_codes.function;
                break;
            }

            if (argument_cnt != 4) {
                // TODO error handling
                // invalid instruction format
                return 0u;
            }

            binary_instr |= regCode(instruction_parts[2]) << 21;  // rs
            binary_instr |= regCode(instruction_parts[3]) << 16;  // rt
            binary_instr |= regCode(instruction_parts[1]) << 11;  // rd
            binary_instr |= instruction_codes.function;           // func. code
            break;

        case INSTR_TYPE_R_SHIFT:
            if (argument_cnt != 4) {
                // TODO error handling
                // invalid instruction format
                return 0u;
            }

            binary_instr |= regCode(instruction_parts[2]) << 16;  // rt
            binary_instr |= regCode(instruction_parts[1]) << 11;  // rd
            binary_instr |= stoi(instruction_parts[3]) << 6;      // sh
            binary_instr |= instruction_codes.function;           // func. code
            break;

        case INSTR_TYPE_I:
            if (argument_cnt != 3 && argument_cnt != 4) {
                // TODO error handling
                // invalid instruction format
                return 0u;
            }

            // format: {instr, rt, rs, imm} or {instr, rt, rs, offset}
            binary_instr |= regCode(instruction_parts[2]) << 21;  // rs
            binary_instr |= regCode(instruction_parts[1]) << 16;  // rt
            binary_instr |= stoi(instruction_parts[3]);  // imm or offset
            break;

        case INSTR_TYPE_J:
            if (argument_cnt != 2) {
                // TODO error handling
                // invalid instruction format
                return 0u;
            }

            binary_instr |= stoi(instruction_parts[1]);
            break;

        case INSTR_TYPE_NULL:
        default:
            binary_instr = 0u;
            break;
    }

    return binary_instr;
}

// --------------------------------------------------------

void secondPass(std::ifstream& fileReader,
                std::ofstream& outputListing,
                std::ofstream& outputInstructions,
                std::map<std::string, unsigned int>& labelAddrMap) {
    fileReader.clear();
    fileReader.seekg(0);
    std::string currentLine;
    unsigned int instruction_count = 0;
    std::smatch match;

    while (getline(fileReader, currentLine)) {
        bool labelSingle = false;
        std::vector<std::string> result = {};
        // Looking for lines with codes
        if (std::regex_search(currentLine, std::regex(R"(^\s*[^\s#]+\s*#*)"))) {
            // Looking for code without comments (we keep everything before a #)
            if (std::regex_search(currentLine, match, std::regex("[^#]*"))) {
                std::string lineWithoutComments = match.str(0);
                // Looking for a label name
                if (std::regex_search(
                        lineWithoutComments, match, std::regex(R"(\S*:)"))) {
                    // Looking if there is no code after the label
                    if (!std::regex_search(lineWithoutComments,
                                           std::regex(":[^#]*[^#\\s]#*"))) {
                        labelSingle = true;
                    }
                    lineWithoutComments = std::regex_replace(
                        lineWithoutComments, std::regex(R"(\S*:)"), "");
                }
                if (std::regex_search(lineWithoutComments,
                                      match,
                                      std::regex(R"(^\s*(\S+)\s*$)"))) {
                    result = {match.str(1)};
                } else if (std::regex_search(
                               lineWithoutComments,
                               match,
                               std::regex(R"(^\s*(\S+)\s+(\S+)\s*$)"))) {
                    if (match.str(1) == "j") {
                        result = {match.str(1),
                                  std::to_string(labelAddrMap[match.str(2)])};
                    } else {
                        result = {match.str(1), match.str(2)};
                    }
                } else if (
                    std::regex_search(
                        lineWithoutComments,
                        match,
                        std::regex(
                            R"(^\s*(\S+)\s+(\S+),\s*(\d+)\((\S+)\)\s*$)"))) {
                    result = {
                        match.str(1), match.str(2), match.str(4), match.str(3)};
                } else if (
                    std::regex_search(
                        lineWithoutComments,
                        match,
                        std::regex(
                            R"(^\s*(\S+)\s+(\S+),\s*(\S+),\s*(\S+)\s*$)"))) {
                    if (match.str(1) == "beq") {
                        result = {match.str(1),
                                  match.str(2),
                                  match.str(3),
                                  std::to_string(labelAddrMap[match.str(4)])};
                    } else {
                        result = {match.str(1),
                                  match.str(2),
                                  match.str(3),
                                  match.str(4)};
                    }
                } else {
                    std::cout
                        << "Error, no match found for : " << lineWithoutComments
                        << std::endl;
                }

                if (result.size() != 0) {
                    uint32_t binary_instruction = binInstruction(result);

                    // output listing
                    std::ios hex_format(nullptr);
                    outputListing << "0x";
                    outputListing << std::hex << std::uppercase << std::setw(8)
                                  << std::setfill('0');
                    hex_format.copyfmt(outputListing);
                    outputListing << instruction_count;
                    outputListing << "    0x";
                    outputListing.copyfmt(hex_format);
                    outputListing << binary_instruction;
                    outputListing << "    ";
                    for (const auto& s : result) {
                        outputListing << s << " ";
                    }
                    outputListing << "\n";

                    // output instructions
                    outputInstructions << "0x";
                    outputInstructions.copyfmt(hex_format);
                    outputInstructions << binary_instruction << "\n";
                }

                if (!labelSingle) instruction_count += 4;
            }
        }
    }

    // TODO order by value?
    // output listing symbols
    outputListing << "\nSymbols\n";
    for (const auto& lbl : labelAddrMap) {
        outputListing << std::left << std::setw(13) << std::setfill(' ');
        outputListing << lbl.first << " ";
        outputListing << "0x";
        outputListing << std::hex << std::uppercase << std::setw(8)
                      << std::setfill('0');
        outputListing << lbl.second;
        outputListing << "\n";
    }
}

// --------------------------------------------------------

int main(int argc, char* argv[]) {
    std::ifstream fileReader(argv[1]);
    // TODO create folder if neccesarry?
    std::ofstream outputListing("output_listing.txt");
    std::ofstream outputInstructions("out_intructions.txt");
    std::map<std::string, unsigned int> labelAddrMap;

    firstPass(fileReader, outputListing, labelAddrMap);
    secondPass(fileReader, outputListing, outputInstructions, labelAddrMap);
    fileReader.close();
    outputListing.close();
    outputInstructions.close();
    return 0;
}
