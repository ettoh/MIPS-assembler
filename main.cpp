#include <iostream>
#include <regex>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <vector>

#include "definitions.hpp"

void firstPass(std::ifstream& fileReader, std::ofstream& outputFile){
    std::string currentLine;
    unsigned int addrPointer = 0;
    std::smatch match;
    std::map<std::string, unsigned int> labelAddrMap;

    if (outputFile.is_open()) outputFile << "Symbols\n";
    while(getline(fileReader,currentLine)) {
        bool labelSingle = false;
        //Looking for lines with codes
        if (std::regex_search(currentLine, std::regex(R"(^\s*[^\s#]+\s*#*)"))){
            //Looking for code without comments (we keep everything before a #)
            if (std::regex_search(currentLine, match, std::regex("[^#]*"))) {
                std::string lineWithoutComments = match.str(0);
                //Looking for a label name
                if (std::regex_search(lineWithoutComments, match, std::regex("\\S*(?=:)"))) {
                    labelAddrMap[match.str(0)] = addrPointer;
                    if (outputFile.is_open()){
                        outputFile << match.str(0) << "   0x" << std::setfill('0') << std::setw(8) << std::hex << addrPointer << "    ";
                    }
                    //Looking if there is no code after the label
                    if (!std::regex_search(lineWithoutComments,std::regex(":[^#]*[^#\\s]#*"))) {
                        labelSingle = true;
                    }
                }
                if (!labelSingle) addrPointer += 4;
            }
        }
    }
}

// --------------------------------------------------------

uint32_t regCode(const std::string& s){
    if(std::regex_match(s, std::regex("[$](zero|\d{2}|[a-z]{1}\d{1}|[a-z]{2})"))){
        // TODO error handling
        // register string invalid
        return 0u;
    }

    uint32_t register_number = 0;
    if(s[1] >= 48 && s[1] <= 57) { // there is a number after $ -> no abbreviations
        register_number = stoi(s.substr(1, s.length()-1));
        if(register_number < 0 || register_number > 31){
            // TODO error handling
            // register out of range
            return 0u;
        }
        return register_number;
    }

    // register abbreviations
    const auto result = REGISTER_ABRV.find(s);
    if (result == REGISTER_ABRV.end())
    {
        // register abbreviation is not supported!
        // TODO error handling
        return 0u;
    }

    return result->second;
}

// --------------------------------------------------------

uint32_t binInstruction(const std::vector<std::string>& instruction_parts){
    size_t argument_cnt = instruction_parts.size();
    if(argument_cnt == 0){
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
        if(argument_cnt != 4){
            // TODO error handling
            // invalid instruction format
            return 0u;
        }
    
        binary_instr |= regCode(instruction_parts[2]) << 21; // rs        
        binary_instr |= regCode(instruction_parts[3]) << 16; // rt        
        binary_instr |= regCode(instruction_parts[1]) << 11; // rd        
        binary_instr |= instruction_codes.function; // func. code
        break;

    case INSTR_TYPE_R_SHIFT:
        if(argument_cnt != 4){
            // TODO error handling
            // invalid instruction format
            return 0u;
        }

        binary_instr |= regCode(instruction_parts[2]) << 16; // rt        
        binary_instr |= regCode(instruction_parts[1]) << 11; // rd        
        binary_instr |= stoi(instruction_parts[3]) << 6; // sh        
        binary_instr |= instruction_codes.function; // func. code
        break;


    case INSTR_TYPE_I:
        if(argument_cnt != 3 && argument_cnt != 4){
            // TODO error handling
            // invalid instruction format
            return 0u;
        }

        // format: {instr, rt, rs, imm} or {instr, rt, rs, offset}
        binary_instr |= regCode(instruction_parts[2]) << 21; // rs        
        binary_instr |= regCode(instruction_parts[1]) << 16; // rt 
        binary_instr |= stoi(instruction_parts[3]); // imm or offset
        break;

    case INSTR_TYPE_J:
        if(argument_cnt != 2){
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

int main(int argc, char* argv[]){
    std::ifstream fileReader(argv[1]);
    // TODO create folder if neccesarry?
    std::ofstream outputFile("output/output1.txt");

    firstPass(fileReader, outputFile);

    // TODO just for debug
    std::vector<std::string> test_instr = {"lw", "$t4", "$t5", "4"};  
    uint32_t binary_instruction = binInstruction(test_instr);
    printf("0x%08X \n", binary_instruction);

    fileReader.close();
    outputFile.close();
    return 0;
}
