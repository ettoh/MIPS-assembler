#include <iostream>
#include <regex>
#include <fstream>
#include <iomanip>

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

int main(int argc, char* argv[]){
    std::ifstream fileReader(argv[1]);
    std::ofstream outputFile("output/output1.txt");

    firstPass(fileReader, outputFile);

    fileReader.close();
    outputFile.close();
    return 0;
}
