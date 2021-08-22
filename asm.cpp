/*
CS-UY 2214
Jeff Epstein
Starter code for E20 assembler
asm.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <bitset>
#include <sstream>
#include <map>
#include <algorithm>
#include <cctype>

using namespace std;

//Create a map to keep track of the labels and their addresses
//Global because many functions need access to it
map<string, uint16_t> labels;

//Create a map to keep track of instructions and their opcodes
//Global because many functions need access to it
map<string, uint16_t> opcodes;

/**
    print_line(address, num)
    Print a line of machine code in the required format.
    Parameters:
        address = RAM address of the instructions
        num = numeric value of machine instruction 
    */
void print_machine_code(size_t address, size_t num) {
    bitset<16> instruction_in_binary(num);
    cout << "ram[" << address << "] = 16'b" << instruction_in_binary <<";"<<endl;
}


/**
 * trim(line)
 * @param line = String that we want to strip the whitespace from.
 * @return Returns the string after trimming whitespace at the front and back.
 */
string & trim(string& line){
    size_t pos = line.find_first_not_of(" \t\n\r\f\v");
    size_t endPos = line.find_last_not_of(" \t\n\r\f\v");

    if (pos != string::npos){
        line = line.substr(pos, (endPos - pos) + 1);
    }else{
        line.clear();
    }

    return line;
}

/**
 *
 * @param s = string which we want to check if it's a number
 * @return Returns a bool value of true if a string is a number(signed and unsigned numbers will return true),
 * and false if it contains any non-number characters(excluding the minus sign).
 */
bool isNumber(string& s){

    //Remove whitespace first
    s = trim(s);

    //Loop through the string, checking if each character is a digit or a minus sign
    //and returning false if any one character isn't either of those two.
    for (char i : s) {
        if (isdigit(i) == false && i != '-') {
            return false;
        }
    }
    return true;
}

/**
 *
 * @param instructionPoint = Pointer to the variable that holds the machine code encoding of the current instruction
 * @param line = The line that we are currently parsing
 * Encodes the three registers of add, sub, and, or, and slt into our machine code variable.
 */
void threeRegInstruction(uint16_t * instructionPoint, const string& line){
    size_t pos = line.find_first_of('$');
    int16_t regNum;

    //Encoding dstReg
    string reg = line.substr(pos+1,1);
    regNum = (int16_t) stoi(reg);
    (*instructionPoint) |= (regNum << 4);

    //Encoding regSrcA
    pos = line.find_first_of('$', pos+1);
    reg = line.substr(pos+1, 1);
    regNum = (int16_t) stoi(reg);
    (*instructionPoint) |= (regNum << 10);

    //Encoding regSrcB
    pos = line.find_first_of('$', pos+1);
    reg = line.substr(pos+1, 1);
    regNum = (int16_t) stoi(reg);
    (*instructionPoint) |= (regNum << 7);

}

/**
 *
 * @param instructionPoint = Pointer to the variable that holds the machine code encoding of the current instruction
 * @param line = The line that we are currently parsing
 * Encoding the registers and immData of addi, slti and movi.
 */
void twoRegAndImm(uint16_t * instructionPoint, const string& line){
    size_t pos;
    int16_t encodedNum;
    string toBeEncoded;

    //Encoding dstReg
    pos = line.find_first_of('$');
    toBeEncoded = line.substr(pos+1,1);
    encodedNum = (int16_t) stoi(toBeEncoded);
    (*instructionPoint) |= (encodedNum << 7);

    //Encoding regSrcA
    pos = line.find_first_of('$', pos+1);

    //Checks if there is a second reg, if not that means we're dealing with movi so regSrcA will be 0
    if (pos != string::npos) {
        toBeEncoded = line.substr(pos + 1, 1);
        encodedNum = (int16_t) stoi(toBeEncoded);
        (*instructionPoint) |= (encodedNum << 10);
    }

    //Encode immData
    //Get rid of the parts of the line before immData
    pos =  line.find_last_of(',');
    toBeEncoded = line.substr(pos + 1);
    toBeEncoded = trim(toBeEncoded);

    //Check if immData is a number or a label
    if (isNumber(toBeEncoded)){
        encodedNum = (int16_t) stoi(toBeEncoded);
    }else{
        encodedNum = labels.at(toBeEncoded);
    }

    //We only want the 7 least significant bits, so clear all the other bits
    encodedNum &= 0b1111111;
    (*instructionPoint) |= encodedNum;
}

/**
 *
 * @param instructionPoint = Pointer to the variable that holds the machine code encoding of the current instruction
 * @param line = The line that we are currently parsing
 * Encoding the registers and immData of sw and lw
 */
void twoRegAndAddress(uint16_t * instructionPoint, const string& line){
    size_t pos = line.find_first_of('$');
    int16_t encodedNum;

    //Encoding dstReg/srcReg
    string toBeEncoded = line.substr(pos+1,1);
    encodedNum = (int16_t) stoi(toBeEncoded);
    (*instructionPoint) |= (encodedNum << 7);

    //Encoding the address register
    pos = line.find_last_of('$');
    toBeEncoded = line.substr(pos + 1, 1);
    encodedNum = (int16_t) stoi(toBeEncoded);
    (*instructionPoint) |= (encodedNum << 10);

    //Encoding immData, first extract immData from the line
    pos = line.find_first_of(',');
    size_t endPos;
    endPos = line.find_first_of('(');
    toBeEncoded = line.substr(pos + 1, (endPos - pos - 1));
    toBeEncoded = trim(toBeEncoded);

    //Check if immData is a number or label
    if(isNumber(toBeEncoded)){
        encodedNum = stoi(toBeEncoded);
    }else{
        encodedNum = labels.at(toBeEncoded);
    }

    //We only want the 7 least significant bits, so clear all the other bits
    encodedNum &= 0b1111111;
    (*instructionPoint) |= encodedNum;
}

/**
 * noRegInstruction(instruction, line)
 * @param instructionPoint = Pointer to the variable that holds the machine code encoding of the current instruction
 * @param line = The line that we are currently parsing
 * Encoding the immData of j and jal.
 */
void noRegInstruction(uint16_t * instructionPoint, const string& line){
    int16_t immData;
    size_t pos = line.find_first_of(' ');
    string toBeEncoded = line.substr(pos + 1);
    toBeEncoded = trim(toBeEncoded);

    //Check if immData is a number or label
    if(isNumber(toBeEncoded)){
        immData = stoi(toBeEncoded);
    }else{
        immData = labels.at(toBeEncoded);
    }

    //Only use the least significant 13 bits
    immData &= ~(111 << 13);
    (*instructionPoint) |= immData;
}

/**
 * jeq(instruction, line, currentAddress)
 * @param instructionPoint = Pointer to the variable that holds the machine code encoding of the current instruction
 * @param line = The line that we are currently parsing
 * @param currentAddress = The current pc address
 * Encodes the registers and immData of jeq which requires a relative immData value to be encoded. This relative
 * immData is based on the current address, and calculated as rel_imm = imm - currentAddress - 1.
 */
void jeq(uint16_t * instructionPoint, const string& line, uint16_t currentAddress){
    size_t pos = line.find_first_of('$');
    int16_t encodedNum;

    //Encoding regA
    string toBeEncoded = line.substr(pos+1,1);
    encodedNum = (int16_t) stoi(toBeEncoded);
    (*instructionPoint) |= (encodedNum << 10);

    //Encoding regB
    pos = line.find_last_of('$');
    toBeEncoded = line.substr(pos + 1, 1);
    encodedNum = (int16_t) stoi(toBeEncoded);
    (*instructionPoint) |= (encodedNum << 7);

    //Encoding immData
    pos =  line.find_last_of(',');
    toBeEncoded = line.substr(pos + 1);
    toBeEncoded = trim(toBeEncoded);

    //Check if immData is a number or a label
    if (isNumber(toBeEncoded)){
        encodedNum = (int16_t) stoi(toBeEncoded);
    }else{
        encodedNum = labels.at(toBeEncoded);
    }

    //Encoding the relative jump distance
    encodedNum = encodedNum - currentAddress - 1;

    //We only want the 7 least significant bits, so clear all the other bits
    encodedNum &= 0b1111111;
    (*instructionPoint) |= encodedNum;
}

/**
 * jr(instruction, line)
 * @param instructionPoint = Pointer to the variable that holds the machine code encoding of the current instruction
 * @param line = The line that we are currently parsing
 * Encodes the register that we're using for our jump instruction into our machine code variable
 */
void jr(uint16_t * instructionPoint, const string& line){
    size_t pos = line.find('$');
    string reg = line.substr(pos + 1, 1);
    auto regNum = (int16_t) stoi(reg);

    (*instructionPoint) |= (regNum << 10);
}

/**
 * halt(instruction, currentAddress)
 * @param instructionPoint = Pointer to the variable that holds the machine code encoding of the current instruction
 * @param address = The current address
 * Encodes a jump to the current address into our machine code variable
 */
void halt(uint16_t * instructionPoint, uint16_t currentAddress){

    //Only use the least significant 13 bits
    currentAddress &= ~(111 << 13);
    (*instructionPoint) |= currentAddress;
}

/**
 * parseLine(line, currentAddress)
 * @param line = Line of the file that we want to parse for instructions
 * @param currentAddress = The current pc address, based on how many instructions have been processed
 * @return Returns the machine code encoding of the E20 instruction on the line.
 *
 * Checks the first word of the line against the map of E20 instructions. If it's in the map, it encodes
 * the opcode value into the instruction variable. If not, it means it's a .fill line so it put the
 * value on the line into the instruction variable. The line has already been stripped of whitespace and
 * labels so there's no need to worry about encountering any of that.
 */
uint16_t parseLine(string& line, uint16_t currentAddress){
    uint16_t instruction=0;
    string word;

    //Stores the first word on the line into word
    stringstream lineStream(line);
    getline(lineStream, word, ' ');

    //Checks the map for the presence of the first word on the line
    map<string, uint16_t>::iterator it;
    it = opcodes.find(word);

    if(it != opcodes.end()){ //if the word is an opcode

        //Switches to the numerical value of the opcode
        switch (it->second)
        {
            case 0: case 1: case 2: case 3: case 4: //add, sub, slt, or, and
                instruction |= it->second;
                threeRegInstruction(&instruction, line);
                break;
            case (1 << 15): case (5 << 13): //sw and lw
                instruction |= it->second;
                twoRegAndAddress(&instruction, line);
                break;
            case (7 << 13): case (1 << 13): //slti, addi and movi, slti will be unsigned while addi/movi is signed
                instruction |= it->second;
                twoRegAndImm(&instruction, line);
                break;
            case (3 << 14): //jeq
                instruction |= it->second;
                jeq(&instruction, line, currentAddress);
                break;
            case 8: //jr
                instruction |= it->second;
                jr(&instruction, line);
                break;
            case (1 << 14): case (3 << 13): //j and jal
                instruction |= it->second;
                noRegInstruction(&instruction, line);
                break;
            case 9: //nop
                //Translates to machine code of all zeros so just do nothing
                break;
            case 10: //halt
                instruction |= (1 << 14);
                halt(&instruction, currentAddress);
                break;
            default:
                break;
        }

    }else if(word == ".fill"){

        //Get rid of the .fill part
        string value = line.substr(6);
        value = trim(value);

        //Check if it's a label or a number
        size_t pos = value.find_first_not_of("-0987654321");

        //It's a number
        if(pos == string::npos) {
            instruction = (uint16_t) stoi(value);
        }else{ //It's a label
            instruction = labels.at(value);
        }
    }

    return instruction;
}

/**
 * parseLabel(line, address)
 * @param line = String which we want to parse labels from
 * @param currentAddress = the address to associate with the label(s) on this line
 * This takes in a string and uses recursion to put any labels found in the string
 * into the labels map, along with its corresponding address. As long as there's a
 * colon in the string it will keep calling itself with a substring from the first
 * character until the last colon, while trimming whitespace each time through.
 */
void parseLabel(string line, uint16_t currentAddress){
    size_t pos = line.find_last_of(':');

    //Base case: If there are no labels on the line, we're done.
    if(pos == string::npos)
        return;

    line = line.substr(0, pos);
    line = trim(line);

    parseLabel(line, currentAddress);

    //Isolates the label in the string and puts it into the map with its corresponding address
    pos = line.find_last_of(':');
    if (pos != string::npos){
        line = line.substr(pos + 1);
        line = trim(line);
    }

    labels[line] = currentAddress;
}

/**
 * stripLabels(line)
 * @param line = String which we want to strip the labels from.
 * @return Returns a substring of line starting from the position after the last colon i.e. after the
 * last label declaration. Returns the original string if there's no colon found.
 * Will return the empty string if a line contains only label(s).
 */
string & stripLabels(string & line){
    size_t pos = line.find_last_of(':');

    if(pos == string::npos)
        return line;

    line = line.substr(pos + 1);
    line = trim(line);
    return line;
}

/**
 * containsInstruction
 * @param line = String which we're checking for the presence of an E20 instruction.
 * @return Returns a bool value containing true if the line has an E20 instruction and false if it doesn't.
 * It performs the check by stripping any labels from the line and checking if there's a valid instruction remaining.
 */
bool containsInstruction(string line){
    line = stripLabels(line);
    line = trim(line);

    string word;
    stringstream lineStream(line);
    getline(lineStream, word, ' ');

    bool isOpcode = (opcodes.count(word));
    isOpcode |= (word == ".fill");
    return isOpcode;
}

/**
 * populateLabels(filename)
 * @param filename = name of the file we want to loop through
 * @return Returns 1 if the file couldn't be opened and 0 otherwise.
 * This loops through every line in the file and populates the label map.
 */
int populateLabels(char * filename){
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }
    string line;
    uint16_t labelAddress = 0;

    //Increment through the file one line at a time
    while (getline(f, line)) {

        //Get rid of comments
        size_t pos = line.find("#");
        if (pos != string::npos)
            line = line.substr(0, pos);

        //Trim whitespace at beginning and end
        line = trim(line);

        //Change everything in line to lowercase because labels are not case sensitive
        transform(line.begin(), line.end(), line.begin(),
                  [](unsigned char c){ return std::tolower(c); });

        //Parse labels on all non-empty lines and increment the address counter if the line has an instruction
        if (!(line.empty())) {
            parseLabel(line, labelAddress);
            if(containsInstruction(line))
                ++labelAddress;
        }
    }
    return 0;
}

/**
    Main function
    Takes command-line args as documented below
*/
int main(int argc, char *argv[]) {
    /*
        Parse the command-line arguments
    */
    char *filename = nullptr;
    bool do_help = false;
    bool arg_error = false;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else
                arg_error = true;
        } else {
            if (filename == nullptr)
                filename = argv[i];
            else
                arg_error = true;
        }
    }
    /* Display error message if appropriate */
    if (arg_error || do_help || filename == nullptr) {
        cerr << "usage " << argv[0] << " [-h] filename" << endl << endl;
        cerr << "Assemble E20 files into machine code" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing assembly language, typically with .s suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        return 1;
    }

    //Populate map of instructions and their opcodes
    opcodes["add"]=0;
    opcodes["sub"]=1;
    opcodes["and"]=2;
    opcodes["or"]=3;
    opcodes["slt"]=4;
    opcodes["jr"]=8;
    opcodes["slti"]=(1 << 13);
    opcodes["lw"]=(1 << 15);
    opcodes["sw"]=(5 << 13);
    opcodes["jeq"]=(6 << 13);
    opcodes["addi"]=(7 << 13);
    opcodes["j"]=(1 << 14);
    opcodes["jal"]=(3 << 13);
    opcodes["movi"]=(7 << 13);
    opcodes["nop"]=9;
    opcodes["halt"]=10;

    //Loop through the file and populate the map of labels
    int fileFailure = populateLabels(filename);

    if(fileFailure == 1)
        return 1;

    /* iterate through the line in the file, construct a list
       of numeric values representing machine code */
    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }

    vector<size_t> instructions;
    string line;
    uint16_t machineCode;
    uint16_t currentAddress = 0;

    //Loop through the file one line at a time and translate to machine code
    while (getline(f, line)) {

        //Cut out the comments
        size_t pos = line.find("#");
        if (pos != string::npos)
            line = line.substr(0, pos);

        //Change everything on the line to lowercase because instructions are not case sensitive
        transform(line.begin(), line.end(), line.begin(),
                  [](unsigned char c){ return std::tolower(c); });

        //Remove all labels on the line and strip the whitespace at the beginning and end.
        line = stripLabels(line);
        line = trim(line);

        //Parse every non-empty line for instructions
        if(!(line.empty())){
            machineCode = parseLine(line, currentAddress);
            instructions.push_back(machineCode);
            ++currentAddress;
        }
    }

    /* print out each instruction in the required format */
    size_t address = 0;
    for (size_t instruction : instructions) {
        print_machine_code(address, instruction);
        address ++;
    }

    return 0;
}