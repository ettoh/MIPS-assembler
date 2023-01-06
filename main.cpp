#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <vector>
#include <stdexcept>

#include "definitions.hpp"

/**
 * @brief First pass to find the addresses for each lable that occur.
 *
 * @param fileReader input file stream of the file that contains the raw
 * instructions
 * @param labelAddrMap reference to the map that will store the numerical
 * addresses of each label
 */
void firstPass(std::ifstream &fileReader, std::map<std::string, int> &labelAddrMap) {
    std::string currentLine;
    unsigned int addrPointer = 0;
    std::smatch match;
    std::regex regex1(R"(^\s*[^\s#]+\s*#*)");
    std::regex regex2("[^#]*");
    std::regex regex3("\\S*(?=:)");
    std::regex regex4(":[^#]*[^#\\s]#*");

    while (getline(fileReader, currentLine)) {
        bool labelSingle = false;
        // Looking for lines with codes
        if (std::regex_search(currentLine, regex1)) {
            // Looking for code without comments (we keep everything before a #)
            if (std::regex_search(currentLine, match, regex2)) {
                std::string lineWithoutComments = match.str(0);
                // Looking for a label name
                if (std::regex_search(lineWithoutComments, match, regex3)) {
                    labelAddrMap[match.str(0)] = addrPointer;

                    // Looking if there is no code after the label
                    if (!std::regex_search(lineWithoutComments, regex4)) {
                        labelSingle = true;
                    }
                }
                if (!labelSingle) addrPointer += 4;
            }
        }
    }
}

// --------------------------------------------------------

/**
 * @brief Converts a register abbreviation into the numerical index of the
 * corresponding register.
 *
 * @param s String that contains the register abbreviation. Must begin with '$'
 * followed by either the numeric index or the alphanumerical abbreviation. E.g.
 * "$14" or "$t1".
 * @param errout Reference to a file output stream where the error message
 * should be printed to.
 * @return uint32_t the numerical index of the corresponding register. 0, if an
 * error occured.
 */
uint32_t regCode(const std::string &s, std::ofstream &errout) {
    if (!std::regex_match(s, std::regex(R"([$](zero|[0-9]{2}|[a-z]\d|[a-z]{2}))"))) {
        errout << "Error: Register string invalid: " << s << ". Abort ...\n";
        errout.close();
        exit(EXIT_FAILURE);
    }

    uint32_t register_number = 0;
    if (s[1] >= 48 &&
        s[1] <= 57) {  // there is a number after $ -> no abbreviations
        register_number = stoi(s.substr(1, s.length() - 1));
        if (register_number < 0 || register_number > 31) {
            errout << "Error: Register out of range: " << register_number << ". Abort ...\n";
            errout.close();
            exit(EXIT_FAILURE);
        }
        return register_number;
    }

    // register abbreviations
    const auto result = REGISTER_ABRV.find(s);
    if (result == REGISTER_ABRV.end()) {
        errout << "Error: Register abbreviation not supported: " << s << ". Abort ...\n";
        errout.close();
        exit(EXIT_FAILURE);
    }

    return result->second;
}

// --------------------------------------------------------

/**
 * @brief Converts a MIPS instruction into its binary form.
 *
 * @param instruction_parts a valid MIPS instruction that was split into its
 * parts so it won't containt any whitespaces or other seperators. E.g. "add
 * $t2, $t1, $t1" has to be split into {"add", "$t2", "$t1", "$t1"}
 * @param errout Reference to a file output stream where the error message
 * should be printed to.
 * @return uint32_t binary MIPS instruction
 */
uint32_t binInstruction(const std::vector<std::string> &instruction_parts, std::ofstream &errout) {
    size_t argument_cnt = instruction_parts.size();
    if (argument_cnt == 0) {
        errout << "Error: Empty instruction can't be converted to binary. Abort ...\n";
        errout.close();
        exit(EXIT_FAILURE);
    }

    // find codes and layout for instruction
    const auto result = INSTR_CODES.find(instruction_parts[0]);
    if (result == INSTR_CODES.end()) {
        errout << "Error: Instruction " << instruction_parts[0] << " is not supported. Abort ...\n";
        errout.close();
        exit(EXIT_FAILURE);
    }
    InstructionCodes instruction_codes = result->second;

    // 1. op-code - first (left) 6 bits
    uint32_t binary_instr = 0x00000000;
    binary_instr |= instruction_codes.op_code << 26;

    // 2. parse arguments
    switch (instruction_codes.format) {
        case INSTR_TYPE_R:
            if (argument_cnt == 2) {  // jr instruction
                binary_instr |= regCode(instruction_parts[1], errout) << 21;
                binary_instr |= instruction_codes.function;
                break;
            }

            if (argument_cnt != 4) {
                errout << "Error: Wrong amount of arguments for instruction type R: " << argument_cnt << ".\n";
                errout.close();
                exit(EXIT_FAILURE);
            }

            binary_instr |= regCode(instruction_parts[2], errout) << 21;  // rs
            binary_instr |= regCode(instruction_parts[3], errout) << 16;  // rt
            binary_instr |= regCode(instruction_parts[1], errout) << 11;  // rd
            binary_instr |= instruction_codes.function;  // func. code
            break;

        case INSTR_TYPE_R_SHIFT:
            if (argument_cnt != 4) {
                errout << "Error: Wrong amount of arguments for instruction type R: " << argument_cnt << ".\n";
                errout.close();
                exit(EXIT_FAILURE);
            }

            binary_instr |= regCode(instruction_parts[2], errout) << 16;  // rt
            binary_instr |= regCode(instruction_parts[1], errout) << 11;  // rd
            binary_instr |= stoi(instruction_parts[3]) << 6;              // sh
            binary_instr |= instruction_codes.function;  // func. code
            break;

        case INSTR_TYPE_I:
            if (argument_cnt != 3 && argument_cnt != 4) {
                errout << "Error: Wrong amount of arguments for instruction type I: " << argument_cnt << ".\n";
                errout.close();
                exit(EXIT_FAILURE);
            }

            // format: {instr, rt, rs, imm} or {instr, rt, rs, offset}
            binary_instr |= regCode(instruction_parts[2], errout) << 21;  // rs
            binary_instr |= regCode(instruction_parts[1], errout) << 16;  // rt
            binary_instr |= stoi(instruction_parts[3]) & 0xFFFF;  // imm or offset
            break;

        case INSTR_TYPE_J:
            if (argument_cnt != 2) {
                errout << "Error: Wrong amount of arguments for instruction type J: " << argument_cnt << ".\n";
                errout.close();
                exit(EXIT_FAILURE);
            }

            binary_instr |= stoi(instruction_parts[1]) & 0x3FFFFFF;
            break;

        case INSTR_TYPE_NULL:
        default:
            binary_instr = 0u;
            break;
    }

    return binary_instr;
}

// --------------------------------------------------------

/**
 * @brief outputPrinting generates the two output files after the execution
 * of the second pass
 *
 * @param outputListing output file stream for the file containing the listing
 * @param outputInstructions output file stream for the file containing the
 * instructions
 * @param result contain the parts of the MIPS instruction to handle
 * @param comment contain the comment if there is one on the line
 * @param label contain the label which is placed at the beginning of the line
 * if there is one
 * @param labelCall in case of instruction "j" or "beq", contain the label
 * name to jump to
 * @param instruction_count contain the current instruction address
 * @param labelSingle is true if a label is the only element on the line
 * (doesn't take into account potential comment)
 */
void outputPrinting(std::ofstream &outputListing,
            std::ofstream &outputInstructions,
            std::vector<std::string> &result,
            std::string &comment,
            std::string &label,
            std::string &labelCall,
            int &instruction_count,
            bool &labelSingle) {
    if (!result.empty() && result[0] == "err") {
        outputListing << "Error: Wrong amount of arguments, operation not supported" << ".\n";
        outputListing.close();
        exit(EXIT_FAILURE);
    } else {
        if (result.size() != 0) {
            uint32_t binary_instruction = binInstruction(result, outputListing);

            // output listing
            std::ios hex_format(nullptr);
            outputListing << "0x";
            outputListing << std::hex << std::setw(8) << std::setfill('0');
            hex_format.copyfmt(outputListing);
            outputListing << instruction_count;
            outputListing << "    0x";
            outputListing.copyfmt(hex_format);
            outputListing << binary_instruction;
            if (label.empty()) {
                outputListing << "                  ";
            } else {
                outputListing << "    ";
                outputListing << std::left << std::setw(10) << std::setfill(' ') << label << std::right;
                outputListing << "    ";
            }
            if (result[0] == "sw" || result[0] == "lw") {
                outputListing << result[0] << " " << result[1] << " " << result[3] << "(" << result[2] << ") ";
            } else if (result[0] == "j" && !labelCall.empty()) {
                outputListing << result[0] << " " << labelCall << " ";
            } else if (result[0] == "beq" && !labelCall.empty()) {
                outputListing << result[0] << " " << result[2] << " " << result[1] << " " << labelCall << " ";
            } else {
                for (const auto &s: result) {
                    outputListing << s << " ";
                }
            }
            if (!comment.empty()) {
                outputListing << "    ";
                outputListing << comment;
            }
            outputListing << "\n";

            // output instructions
            outputInstructions << "0x";
            outputInstructions.copyfmt(hex_format);
            outputInstructions << binary_instruction << "\n";
            if (!labelSingle) instruction_count += 4;
        } else {
            if (!label.empty() || !comment.empty()) {
                outputListing << "                            ";
            }
            if (!label.empty()) {
                outputListing << label;
            }
            if (!comment.empty()) {
                if (!label.empty()) {
                    outputListing << "    ";
                }
                outputListing << comment;
            }
            outputListing << "\n";
        }
    }
}

// --------------------------------------------------------

/**
 * @brief symbolsOutputPrinting print the symbols at the ends of the listing
 * file when the outputPrinting function has finished
 *
 * @param outputListing output file stream for the file containing the listing
 * @param labelAddrMap reference to a map that holds the numerical addresses for
 * each label
 */
void symbolsOutputPrinting(std::ofstream &outputListing, std::map<std::string, int> &labelAddrMap) {
    outputListing << "\nSymbols\n";
    for (const auto &lbl: labelAddrMap) {
        outputListing << std::left << std::setw(13) << std::setfill(' ');
        outputListing << lbl.first << " ";
        outputListing << "0x";
        outputListing << std::hex << std::right << std::setw(8) << std::setfill('0');
        outputListing << lbl.second;
        outputListing << "\n";
    }
}

// --------------------------------------------------------

/**
 * @brief Tries to convert a given string into an integer and 
 * checks if that string can be converted into an integer at all.
 * 
 * @param s string
 * @return std::pair<bool, int> first: bool, if the string can be
 * converted to an integer; second: converted integer value, 0 if 
 * conversion failed
 */
std::pair<bool, int> strtoi_safe(const std::string& s){
    char* p;
    int x = strtol(s.c_str(), &p, 10);
    return {*p == 0, x};
}

// --------------------------------------------------------

/**
 * @brief Second pass to validate the instructions, handle comments, split the
 * instructions into their parts and eventually convert and print them.
 *
 * @param fileReader input file stream of the file that contains the raw
 * instructions
 * @param outputListing output file stream for the file containing the listing
 * @param outputInstructions output file stream for the file containing the
 * instructions
 * @param labelAddrMap reference to a map that holds the numerical addresses for
 * each label
 */
void secondPass(std::ifstream &fileReader,
                std::ofstream &outputListing,
                std::ofstream &outputInstructions,
                std::map<std::string, int> &labelAddrMap) {
    fileReader.clear();
    fileReader.seekg(0);
    std::string currentLine;
    int instruction_count = 0;
    std::smatch match;
    std::regex regex1(R"(.*(#.*))");
    std::regex regex2((R"(^\s*[^\s#]+\s*#*)"));
    std::regex regex3("[^#]*");
    std::regex regex4((R"(\S*:)"));
    std::regex regex5(":[^#]*[^#\\s]#*");
    std::regex regex6(R"(\S*:)");
    std::regex firstMatch(R"(^\s*(\S+)\s*$)");
    std::regex secondMatch(R"(^\s*(\S+)\s+(\S+)\s*$)");
    std::regex thirdMatch(R"(^\s*(\S+)\s+(\S+),\s*(\d+)\((\S+)\)\s*$)");
    std::regex fourthMatch(R"(^\s*(\S+)\s+(\S+),\s*(\S+),\s*(\S+)\s*$)");

    while (getline(fileReader, currentLine)) {
        bool labelSingle = false;
        std::string lineWithoutComments;
        std::string comment;
        std::string label;
        std::vector<std::string> result = {};
        std::string labelCall;
        if (std::regex_search(currentLine, match, regex1)) {
            comment = match.str(1);
        }
        // Looking for lines with codes
        if (std::regex_search(currentLine, regex2)) {
            // Looking for code without comments (we keep everything before a #)
            if (std::regex_search(currentLine, match, regex3)) {
                lineWithoutComments = match.str(0);
                // Looking for a label name
                if (std::regex_search(lineWithoutComments, match, regex4)) {
                    label = match.str(0);
                    // Looking if there is no code after the label
                    if (!std::regex_search(lineWithoutComments, regex5)) {
                        labelSingle = true;
                    }
                    lineWithoutComments = std::regex_replace(lineWithoutComments, regex6, "");
                }
                if (std::regex_search(lineWithoutComments, match, firstMatch)) {
                    result = {match.str(1)};
                } else if (std::regex_search(lineWithoutComments, match, secondMatch)) {
                    if (match.str(1) == "j") {
                        auto converted_string = strtoi_safe(match.str(2));
                        if(converted_string.first){ // input is already integer
                            result = {match.str(1), std::to_string(converted_string.second)};
                        }else{ // input is a label
                            auto map_result = labelAddrMap.find(match.str(2));
                            if (map_result == labelAddrMap.end()) {
                                outputListing << "Error: label '" << match.str(2)
                                            << "' does not exist!" << std::endl;
                                outputListing.close();
                                exit(EXIT_FAILURE);
                            }
                            result = {match.str(1), std::to_string(map_result->second / 4)};
                            labelCall = match.str(2);
                        }
                    } else {
                        result = {match.str(1), match.str(2)};
                    }
                } else if (std::regex_search(lineWithoutComments, match, thirdMatch)) {
                    result = {match.str(1), match.str(2), match.str(4), match.str(3)};
                } else if (std::regex_search(lineWithoutComments, match, fourthMatch)) {
                    if (match.str(1) == "beq") {
                        auto converted_string = strtoi_safe(match.str(4));
                        if(converted_string.first){ // input is already integer
                            result = {
                                match.str(1),
                                match.str(3),  // special order for beq and bne
                                match.str(2),
                                std::to_string(converted_string.second)
                            };
                        }else{ // input is a label
                            auto map_result = labelAddrMap.find(match.str(4));
                            if (map_result == labelAddrMap.end()) {
                                outputListing << "Error: label '" << match.str(4)
                                            << "' does not exist!" << std::endl;
                                outputListing.close();
                                exit(EXIT_FAILURE);
                            }
                            result = {
                                    match.str(1),
                                    match.str(3),  // special order for beq and bne
                                    match.str(2),
                                    std::to_string((map_result->second - instruction_count - 4) / 4)
                            };
                            labelCall = match.str(4);
                        }
                    } else {
                        result = {match.str(1), match.str(2), match.str(3), match.str(4)};
                    }
                } else if (label.empty()) {
                    result = {"err"};
                }
            }
        }
        outputPrinting(outputListing, outputInstructions, result, comment, label, labelCall, instruction_count, labelSingle);
    }
    symbolsOutputPrinting(outputListing, labelAddrMap);
}

// --------------------------------------------------------

int main(int argc, char* argv[]) {
    // call like "./executable inputfile output_listing output_instructions"
    if(argc != 4){
        return 1;
    }

    // open files
    std::ifstream fileReader(argv[1]);
    std::ofstream outputListing(argv[2]);
    std::ofstream outputInstructions(argv[3]);
    if(!fileReader.is_open() || !outputInstructions.is_open() || ! outputListing.is_open()){
        return 1;
    }

    std::map<std::string, int> labelAddrMap;

    firstPass(fileReader, labelAddrMap);
    secondPass(fileReader, outputListing, outputInstructions, labelAddrMap);
    fileReader.close();
    outputListing.close();
    outputInstructions.close();
    return 0;
}
