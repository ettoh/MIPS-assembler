#include <iostream>
#include <regex>
#include <fstream>
#include <iomanip>
#include <map>

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

uint32_t binInstruction(const std::string& str_instruction){
    // TODO ensure that str has the correct format
    std::string mnemonic = str_instruction.substr(0, str_instruction.find_first_of(" ", 0));

    // find codes and layout for instruction
    const auto result = instr_codes.find(mnemonic);
    if (result == instr_codes.end())
    {
        // Instruction is not supported!
        // TODO error handling
        return 0u;
    }
    InstructionCodes instruction_codes = result->second;

    // 1. op-code - first (left) 6 bits
    uint32_t binary_instr = 0x00000000;
    binary_instr |= instruction_codes.op_code << 26;

    // 2. TODO rest ....
    switch (instruction_codes.format)
    {
    case INSTR_TYPE_R:
        break;

    case INSTR_TYPE_I:
        break;

    case INSTR_TYPE_J:
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
    std::string test_instruction = "lw $t4, 4($t5)";
    uint32_t binary_instruction = binInstruction(test_instruction);
    printf("0x%08X \n", binary_instruction);

    fileReader.close();
    outputFile.close();
    return 0;
}
