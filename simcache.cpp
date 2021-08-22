/*
CS-UY 2214
Jeff Epstein
Starter code for E20 cache assembler
simcache.cpp
*/

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <iomanip>
#include <regex>
#include "LRUCache.h"

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;


/*
    Prints out a correctly-formatted log entry.

    @param cache_name The name of the cache where the event
        occurred. "L1" or "L2"

    @param status The kind of cache event. "SW", "HIT", or
        "MISS"

    @param pc The program counter of the memory
        access instruction

    @param addr The memory address being accessed.

    @param line The cache line or set number where the data
        is stored.
*/
void print_log_entry(const string &cache_name, const string &status, int pc, int addr, int line) {
    cout << left << setw(8) << cache_name + " " + status <<  right <<
         " pc:" << setw(5) << pc <<
         "\taddr:" << setw(5) << addr <<
         "\tline:" << setw(4) << line << endl;
}

/**
 * alu(instruction, pc, registers)
 *
 * This function reads the last 4 bits of the instruction and performs the required operation,
 * one of add, subtract, bitwise and, bitwise or, set less than, or jumping to a register. It decodes
 * the middle 9 bits into their corresponding registers, and it also updates the pc. When updating
 * the pc, it checks for overflow and wraps around if overflow is found.
 *
 * @param instruction - The numerical value of the current instruction
 * @param pc - The current pc
 * @param registers - Our register array
 * @return Returns the next pc
 */
uint16_t alu(size_t instruction, uint16_t pc, uint16_t registers []){
    //get bits 10 through 12 for regSrcA
    size_t regSrcA = ((instruction & (0b111 << 10)) >> 10);

    //get bits 7 through 9 for regSrcB
    size_t regSrcB = ((instruction & (0b111 << 7)) >> 7);

    //get bits 4 through 6 for regDst
    size_t regDst = ((instruction & (0b111 << 4)) >> 4);

    //keep the 4 least significant bits for the operation chooser
    size_t operation = (instruction & 0xf);

    switch (operation) {
        case 0:
            //add
            registers[regDst] = registers[regSrcA] + registers[regSrcB];
            pc++;
            break;
        case 1:
            //sub
            registers[regDst] = registers[regSrcA] - registers[regSrcB];
            pc++;
            break;
        case 2:
            //and - bitwise
            registers[regDst] = registers[regSrcA] & registers[regSrcB];
            pc++;
            break;
        case 3:
            //or - bitwise
            registers[regDst] = registers[regSrcA] | registers[regSrcB];
            pc++;
            break;
        case 4:
            //slt
            registers[regDst] = (registers[regSrcA] < registers[regSrcB]) ? 1 : 0;
            pc++;
            break;
        case 8:
            //jr
            pc = registers[regSrcA];
            break;
        default:
            break;
    }

    /*ensure our pc does not overflow by only keeping the 13 least significant bits
    if (pc >= MEM_SIZE) {
        pc &= 0x1fff;
    }*/

    return pc;
}

/**
 * jump(instruction)
 *
 * This function executes the jump instruction by putting the 13 least significant bits
 * of the instruction into the pc.
 *
 * @param instruction - The numerical value of the instruction
 * @return Returns the new pc, which is the last 13 bits of the instruction
 */
uint16_t jump(size_t instruction) {
    //jump to the address given by the last 13 bits
    uint16_t pc = (instruction & 0x1fff);
    return pc;
}

/**
 * slti(instruction, registers)
 *
 * This function executes the slti instruction. It decodes bits 7 through 12 into our source
 * and destination registers, and sets the destination register to 1 if the source register's
 * value is less than the sign-extended immediate data, and 0 otherwise. The comparison is
 * unsigned - this means both numbers are treated as unsigned.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 */
void slti(size_t instruction, uint16_t registers[]) {
    //get bits 10 through 12 for operand1 register
    size_t operand1 = ((instruction & (0b111 << 10)) >> 10);

    //the second operand is the unsigned representation of the last 7 bits of the instruction
    uint16_t immData = (instruction & 0x7f);

    //sign extend from 7 to 16 bits
    immData |= (immData & (1 << 6)) ? (0x1ff << 7) : 0;

    //get bits 7 through 9 for regDst
    size_t regDst = ((instruction & (0b111 << 7)) >> 7);

    registers[regDst] = (registers[operand1] < immData) ? 1 : 0;
}
/**
 * jal(instruction, registers, pc)
 *
 * This function executes the jal instruction. It stores the next pc value into $7 after
 * checking for overflow, and then calls jump to execute the jump part of the instruction.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param pc - The current pc value
 * @return Returns the new pc value, which is the 13 bit immediate field of the instruction
 */
uint16_t jal(size_t instruction, uint16_t registers[], uint16_t pc) {

    pc++;

    /*ensure our pc does not overflow by only keeping the 13 least significant bits
    if (pc >= MEM_SIZE) {
        pc &= 0x1fff;
    }*/

    //placing the next instruction address into $7
    registers[7] = pc;

    //execute the jump
    pc = jump(instruction);
    return pc;
}

/**
 * lw(instruction, registers, memory, L1)
 *
 * This function executes the lw instruction. It decodes bits 7 through 12 into the
 * destination register and the address register, and sign extends the 7 bit immediate
 * field to get the offset. Then it takes the value in memory found at the sum of the
 * signed value of offset plus the value in the address register, and stores it into
 * the destination register. Additionally, it accesses the cache and prints out whether
 * it was a cache hit or miss.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param memory - Our memory array
 * @param L1 - Reference to our cache
 */
void lw(uint16_t pc, size_t instruction, uint16_t registers[], const size_t memory [], vector<LRUCache> &L1) {

    //the offset is the signed number represented by the least significant 7 bits of the instruction
    //first take the least significant 7 bits
    int16_t offset = (instruction & 0x7f);

    //then sign extend from 7 bits to 16 bits
    offset |= (offset & (1 << 6)) ? (0x1ff << 7) : 0;

    //get bits 10 through 12 for regAddr
    size_t regAddr = ((instruction & (0b111 << 10)) >> 10);

    //get bits 7 through 9 for regDst
    size_t regDst = ((instruction & (0b111 << 7)) >> 7);

    //desired memory location is the least significant 13 bits of (value in $regAddr + offset)
    int memLocation = ((offset + registers[regAddr]) & 0x1fff);

    //get the cache line number this memory location will be stored on and check if it's already in the cache
    int L1lineNumber = L1[0].lineNumber(memLocation);
    bool isInCache = L1[L1lineNumber].accessCache(memLocation);

    if(isInCache) {
        print_log_entry("L1", "HIT", pc, memLocation, L1lineNumber);
    }else {
        print_log_entry("L1", "MISS", pc, memLocation, L1lineNumber);
    }

    //take the value found in the desired memory location and load it into $regDst
    registers[regDst] = memory[memLocation];
}

/**
 * lw(instruction, registers, memory, L1, L2)
 *
 * This function executes the lw instruction. It decodes bits 7 through 12 into the
 * destination register and the address register, and sign extends the 7 bit immediate
 * field to get the offset. Then it takes the value in memory found at the sum of the
 * signed value of offset plus the value in the address register, and stores it into
 * the destination register. Additionally, it accesses the first cache and prints out whether
 * it was a cache hit or miss. If it was a miss, it accesses the second cache and does the
 * same thing.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param memory - Our memory array
 * @param L1 - Reference to our first cache
 * @param L2 - Reference to our second cache
 */
void lw(uint16_t pc, size_t instruction, uint16_t registers[], const size_t memory [], vector<LRUCache> &L1, vector<LRUCache> &L2) {

    //the offset is the signed number represented by the least significant 7 bits of the instruction
    //first take the least significant 7 bits
    int16_t offset = (instruction & 0x7f);

    //then sign extend from 7 bits to 16 bits
    offset |= (offset & (1 << 6)) ? (0x1ff << 7) : 0;

    //get bits 10 through 12 for regAddr
    size_t regAddr = ((instruction & (0b111 << 10)) >> 10);

    //get bits 7 through 9 for regDst
    size_t regDst = ((instruction & (0b111 << 7)) >> 7);

    //desired memory location is the least significant 13 bits of (value in $regAddr + offset)
    size_t memLocation = ((offset + registers[regAddr]) & 0x1fff);

    //get the cache line number this memory location will be stored on and check if it's already in the cache
    int L1lineNumber = L1[0].lineNumber(memLocation);
    bool isInCache = L1[L1lineNumber].accessCache(memLocation);

    if(isInCache) {
        print_log_entry("L1", "HIT", pc, memLocation, L1lineNumber);
    }else{ //if we had a miss on L1, check L2 and print
        print_log_entry("L1", "MISS", pc, memLocation, L1lineNumber);

        int L2lineNumber = L2[0].lineNumber(memLocation);
        isInCache = L2[L2lineNumber].accessCache(memLocation);
        if(isInCache) {
            print_log_entry("L2", "HIT", pc, memLocation, L2lineNumber);
        } else{
            print_log_entry("L2", "MISS", pc, memLocation, L2lineNumber);
        }
    }

    //take the value found in the desired memory location and load it into $regDst
    registers[regDst] = memory[memLocation];
}

/**
 * sw(instruction, registers, memory, L1)
 *
 * This function executes the sw instruction. It decodes bits 7 through 12 into the
 * source register and the address register, and sign extends the 7 bit immediate
 * field to get the offset. Then it takes the value found in the source register and
 * stores it into memory at the address found by taking the sum of the signed value
 * of offset plus the value in the address register. Additionally, it adds the new
 * memory value to our cache and prints out the address, cache line, and pc of the
 * cache access.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param memory - Our memory array
 * @param L1 - Reference to our cache
 */
void sw(uint16_t pc, size_t instruction, const uint16_t registers[], size_t memory[], vector<LRUCache> &L1) {

    //the offset is the signed number represented by the least significant 7 bits of the instruction
    //first take the least significant 7 bits
    int8_t offset = (instruction & 0x7f);

    //then sign extend from 7 bits to 16 bits
    offset |= (offset & (1 << 6)) ? (0x1ff << 7) : 0;

    //get bits 10 through 12 for regAddr
    size_t regAddr = ((instruction & (0b111 << 10)) >> 10);

    //get bits 7 through 9 for regSrc
    size_t regSrc = ((instruction & (0b111 << 7)) >> 7);

    //desired memory location is the least significant 13 bits of (value in $regAddr + offset)
    size_t memLocation = ((offset + registers[regAddr]) & 0x1fff);

    //get the cache line that this memory address will be stored on, write to cache and print
    int L1lineNumber = L1[0].lineNumber(memLocation);
    L1[L1lineNumber].accessCache(memLocation);
    print_log_entry("L1", "SW", pc, memLocation, L1lineNumber);

    //take the value found in $regSrc and store it in the desired memory location
    memory[memLocation] = registers[regSrc];
}

/**
 * sw(instruction, registers, memory, L1, L2)
 *
 * This function executes the sw instruction. It decodes bits 7 through 12 into the
 * source register and the address register, and sign extends the 7 bit immediate
 * field to get the offset. Then it takes the value found in the source register and
 * stores it into memory at the address found by taking the sum of the signed value
 * of offset plus the value in the address register. Additionally, it adds the new
 * memory value to both caches and prints out the address, cache line(s), and pc of
 * the cache accesses.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param memory - Our memory array
 * @param L1 - Reference to our first cache
 * @param L2 - Reference to our second cache
 */
void sw(uint16_t pc, size_t instruction, const uint16_t registers[], size_t memory[], vector<LRUCache> &L1, vector<LRUCache> &L2) {

    //the offset is the signed number represented by the least significant 7 bits of the instruction
    //first take the least significant 7 bits
    int8_t offset = (instruction & 0x7f);

    //then sign extend from 7 bits to 16 bits
    offset |= (offset & (1 << 6)) ? (0x1ff << 7) : 0;

    //get bits 10 through 12 for regAddr
    size_t regAddr = ((instruction & (0b111 << 10)) >> 10);

    //get bits 7 through 9 for regSrc
    size_t regSrc = ((instruction & (0b111 << 7)) >> 7);

    //desired memory location is the least significant 13 bits of (value in $regAddr + offset)
    size_t memLocation = ((offset + registers[regAddr]) & 0x1fff);

    //get the L1 cache line that this memory address will be stored on, write to cache and print
    int L1lineNumber = L1[0].lineNumber(memLocation);
    L1[L1lineNumber].accessCache(memLocation);
    print_log_entry("L1", "SW", pc, memLocation, L1lineNumber);

    //get the L2 cache line that this memory address will be stored on, write to cache and print
    int L2lineNumber = L2[0].lineNumber(memLocation);
    L2[L2lineNumber].accessCache(memLocation);
    print_log_entry("L2", "SW", pc, memLocation, L2lineNumber);

    //take the value found in $regSrc and store it in the desired memory location
    memory[memLocation] = registers[regSrc];
}

/**
 * jeq(instruction, registers, pc)
 *
 * This function executes the jeq instruction. It decodes bits 7 through 12 into the 2
 * operand registers, and performs an equality check on the values in those registers.
 * If they're equal it returns the sum of pc + 1 + rel_imm. Rel_imm is considered a
 * signed 7 bit number and is the sign-extended 7 bit immediate field. If the register
 * values are not equal it returns the incremented pc.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param pc - The current pc value
 * @return Returns the new pc value
 */
uint16_t jeq(size_t instruction, const uint16_t registers[], uint16_t pc) {
    //get bits 10 through 12 for regSrcA
    size_t regSrcA = ((instruction & (0b111 << 10)) >> 10);

    //get bits 7 through 9 for regSrcB
    size_t regSrcB = ((instruction & (0b111 << 7)) >> 7);

    bool isEqual = (registers[regSrcA] == registers[regSrcB]);

    pc++;

    if(isEqual) {
        //the rel_imm is the signed number represented by the least significant 7 bits of the instruction
        //first take the least significant 7 bits
        int16_t rel_imm = (instruction & 0x7f);

        //then sign extend from 7 bits to 16 bits
        rel_imm |= (rel_imm & (1 << 6)) ? (0x1ff << 7) : 0;

        pc += rel_imm;
    }

    /*ensure our pc does not overflow by only keeping the 13 least significant bits
    if (pc >= MEM_SIZE) {
        pc &= 0x1fff;
    }*/

    return pc;
}

/**
 * addi(instruction, registers)
 *
 * This function executes the addi instruction. It decodes bits 7 through 12 of the instruction
 * into our source and destination registers, and sign extends the 7 bit immediate field data.
 * It adds the signed immData to the value in the source register and places the sum into the
 * destination register.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 */
void addi(size_t instruction, uint16_t registers[]) {

    //the immData is the signed number represented by the least significant 7 bits of the instruction
    //first take the least significant 7 bits
    int16_t immData = (instruction & 0x7f);

    //then sign extend from 7 bits to 16 bits
    immData |= (immData & (1 << 6)) ? (0x1ff << 7) : 0;

    //get bits 10 through 12 for regSrc
    size_t regSrc = ((instruction & (0b111 << 10)) >> 10);

    //get bits 7 through 9 for regDst
    size_t regDst = ((instruction & (0b111 << 7)) >> 7);

    registers[regDst] = immData + registers[regSrc];

}

/**
 * simulation(memory, registers, L1)
 *
 * This function acts as a high level controller for our simulator. It takes in the memory array, which
 * contains our instructions, our register array, and one cache. It checks the 3 most significant bits of the
 * instruction and calls the corresponding function to execute the instruction, and updates the pc. It prevents
 * the pc from exceeding its max value of 8191 by only looking at its first 13 bits, and also enforces $0's
 * immutability by changing its value to 0 at the end of every instruction execution. It will run infinitely,
 * going through each element of the memory array unless/until it encounters a halt instruction, and then
 * returns the final value of the pc, which is where the halt is.
 *
 * @param memory - Our memory array, contains all of our instructions
 * @param registers - Our register array
 * @param L1 - Reference to our cache
 * @return Returns the final pc
 */
uint16_t simulation(size_t memory [], uint16_t registers [], vector<LRUCache> &L1){

    uint16_t pc = 0;
    uint16_t lastPc;
    bool isHalt = false;

    size_t opCode;
    size_t instruction;

    while (!isHalt) {
        lastPc = pc;

        //Only use the least significant 13 bits of the pc for the instruction
        instruction = memory[(pc & 0x1fff)];

        //Only takes bits 13 through 15
        opCode = ((instruction & 0xffff) >> 13);

        switch (opCode) {
            case 0: //add, sub, and, or, slt, jr
                pc = alu(instruction, pc, registers);
                break;
            case 1: //slti
                slti(instruction, registers);
                pc++;
                break;
            case 2: //j - check if the instruction was a halt and set isHalt appropriately
                pc = jump(instruction);
                if(pc == lastPc)
                    isHalt = true;
                break;
            case 3: //jal
                pc = jal(instruction, registers, pc);
                break;
            case 4: //lw
                lw(pc, instruction, registers, memory, L1);
                pc++;
                break;
            case 5: //sw
                sw(pc, instruction, registers, memory, L1);
                pc++;
                break;
            case 6: //jeq
                pc = jeq(instruction, registers, pc);
                break;
            case 7: //addi
                addi(instruction, registers);
                pc++;
                break;
            default:
                break;
        }

        //register $0 is always 0
        registers[0] = 0;

    }

    return pc;
}

/**
 * simulation(memory, registers, L1, L2)
 *
 * This function acts as a high level controller for our simulator. It takes in the memory array, which
 * contains our instructions, our register array, and two caches. It checks the 3 most significant bits of the
 * instruction and calls the corresponding function to execute the instruction, and updates the pc. It prevents
 * the pc from exceeding its max value of 8191 by only looking at its first 13 bits, and also enforces $0's
 * immutability by changing its value to 0 at the end of every instruction execution. It will run infinitely,
 * going through each element of the memory array unless/until it encounters a halt instruction, and then
 * returns the final value of the pc, which is where the halt is.
 *
 * @param memory - Our memory array, contains all of our instructions
 * @param registers - Our register array
 * @param L1 - Reference to our first cache
 * @param L2 - Reference to our second cache
 * @return Returns the final pc
 */
uint16_t simulation(size_t memory [], uint16_t registers [], vector<LRUCache> &L1, vector<LRUCache> &L2){

    uint16_t pc = 0;
    uint16_t lastPc;
    bool isHalt = false;

    size_t opCode;
    size_t instruction;

    while (!isHalt) {
        lastPc = pc;
        //Only use the least significant 13 bits of the pc for the instruction
        instruction = memory[(pc & 0x1fff)];

        //Only takes bits 13 through 15
        opCode = ((instruction & 0xffff) >> 13);

        switch (opCode) {
            case 0: //add, sub, and, or, slt, jr
                pc = alu(instruction, pc, registers);
                break;
            case 1: //slti
                slti(instruction, registers);
                pc++;
                break;
            case 2: //j - check if the instruction was a halt and set isHalt appropriately
                pc = jump(instruction);
                if(pc == lastPc)
                    isHalt = true;
                break;
            case 3: //jal
                pc = jal(instruction, registers, pc);
                break;
            case 4: //lw
                lw(pc, instruction, registers, memory, L1, L2);
                pc++;
                break;
            case 5: //sw
                sw(pc, instruction, registers, memory, L1, L2);
                pc++;
                break;
            case 6: //jeq
                pc = jeq(instruction, registers, pc);
                break;
            case 7: //addi
                addi(instruction, registers);
                pc++;
                break;
            default:
                break;
        }

        //register $0 is always 0
        registers[0] = 0;

    }

    return pc;
}


/*
    Loads an E20 machine code file into the list
    provided by mem. We assume that mem is
    large enough to hold the values in the machine
    code file.

    @param f Open file to read from
    @param mem Array representing memory into which to read program
*/
void load_machine_code(ifstream &f, size_t mem[]) {
    regex machine_code_re("^ram\\[(\\d+)\\] = 16'b(\\d+);.*$");
    size_t expectedaddr = 0;
    string line;
    while (getline(f, line)) {
        smatch sm;
        if (!regex_match(line, sm, machine_code_re)) {
            cerr << "Can't parse line: " << line << endl;
            exit(1);
        }
        size_t addr = stoi(sm[1], nullptr, 10);
        size_t instr = stoi(sm[2], nullptr, 2);
        if (addr != expectedaddr) {
            cerr << "Memory addresses encountered out of sequence: " << addr << endl;
            exit(1);
        }
        if (addr >= MEM_SIZE) {
            cerr << "Program too big for memory" << endl;
            exit(1);
        }
        expectedaddr ++;
        mem[addr] = instr;
    }
}


/*
    Prints out the correctly-formatted configuration of a cache.

    @param cache_name The name of the cache. "L1" or "L2"

    @param size The total size of the cache, measured in memory cells.
        Excludes metadata

    @param assoc The associativity of the cache. One of [1,2,4,8,16]

    @param blocksize The blocksize of the cache. One of [1,2,4,8,16,32,64])
*/
void print_cache_config(const string &cache_name, int size, int assoc, int blocksize, int num_lines) {
    cout << "Cache " << cache_name << " has size " << size <<
         ", associativity " << assoc << ", blocksize " << blocksize <<
         ", lines " << num_lines << endl;
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
    string cache_config;
    for (int i=1; i<argc; i++) {
        string arg(argv[i]);
        if (arg.rfind("-",0)==0) {
            if (arg== "-h" || arg == "--help")
                do_help = true;
            else if (arg=="--cache") {
                i++;
                if (i>=argc)
                    arg_error = true;
                else
                    cache_config = argv[i];
            }
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
        cerr << "usage " << argv[0] << " [-h] [--cache CACHE] filename" << endl << endl;
        cerr << "Simulate E20 cache" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
        cerr << "  --cache CACHE  Cache configuration: size,associativity,blocksize (for one"<<endl;
        cerr << "                 cache) or"<<endl;
        cerr << "                 size,associativity,blocksize,size,associativity,blocksize"<<endl;
        cerr << "                 (for two caches)"<<endl;
        return 1;
    }

    ifstream f(filename);
    if (!f.is_open()) {
        cerr << "Can't open file "<<filename<<endl;
        return 1;
    }

    //Load f and parse using load_machine_code
    size_t memory [MEM_SIZE];
    load_machine_code(f, memory);

    //Create our array of registers
    uint16_t registers [NUM_REGS] = {0};

    //uint16_t pc;

    /* parse cache config */
    if (cache_config.size() > 0) {
        vector<int> parts;
        size_t pos;
        size_t lastpos = 0;
        while ((pos = cache_config.find(",", lastpos)) != string::npos) {
            parts.push_back(stoi(cache_config.substr(lastpos,pos)));
            lastpos = pos + 1;
        }
        parts.push_back(stoi(cache_config.substr(lastpos)));
        if (parts.size() == 3) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            
            //Create our one cache and initialize its properties
            //The entire vector is the cache. It is a vector of fully associative caches, 
            //and each line of the vector represents a line of the cache
            vector<LRUCache> L1;
            LRUCache temp(L1size, L1assoc, L1blocksize);
            int L1numLines = temp.numLines();
            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1numLines);
            //The vector should only be L1numLines in size
            L1.reserve(L1numLines);
            for(int i = 0; i < L1numLines; i++){
                L1.push_back(temp);
            }

            //execute E20 program and simulate one cache here
            simulation(memory, registers, L1);
            
        } else if (parts.size() == 6) {
            int L1size = parts[0];
            int L1assoc = parts[1];
            int L1blocksize = parts[2];
            int L2size = parts[3];
            int L2assoc = parts[4];
            int L2blocksize = parts[5];

            //Create our two caches and initialize their properties
            //The entire vector is the cache. It is a vector of fully associative caches, 
            //and each line of the vector represents a line of the cache
            vector<LRUCache> L1;
            LRUCache temp1(L1size, L1assoc, L1blocksize);
            int L1numLines = temp1.numLines();
            //The vector should only be L1numLines in size
            L1.reserve(L1numLines);
            for(int i = 0; i < L1numLines; i++){
                L1.push_back(temp1);
            }

            vector<LRUCache> L2;
            LRUCache temp2(L2size, L2assoc, L2blocksize);
            int L2numLines = temp2.numLines();
            //The vector should only be L2\numLines in size
            L2.reserve(L2numLines);
            for(int i = 0; i < L2numLines; i++){
                L2.push_back(temp2);
            }

            print_cache_config("L1", L1size, L1assoc, L1blocksize, L1numLines);
            print_cache_config("L2", L2size, L2assoc, L2blocksize, L2numLines);

            //execute E20 program and simulate two caches here
            simulation(memory, registers, L1, L2);
        } else {
            cerr << "Invalid cache config"  << endl;
            return 1;
        }
    }

    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9