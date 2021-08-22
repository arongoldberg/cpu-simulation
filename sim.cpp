/*
CS-UY 2214
Jeff Epstein
Starter code for E20 simulator
sim.cpp
*/


#include <cstddef>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <regex>
#include <cstdlib>

using namespace std;

// Some helpful constant values that we'll be using.
size_t const static NUM_REGS = 8;
size_t const static MEM_SIZE = 1<<13;
size_t const static REG_SIZE = 1<<16;

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
 * lw(instruction, registers, memory)
 *
 * This function executes the lw instruction. It decodes bits 7 through 12 into the
 * destination register and the address register, and sign extends the 7 bit immediate
 * field to get the offset. Then it takes the value in memory found at the sum of the
 * signed value of offset plus the value in the address register, and stores it into
 * the destination register.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param memory - Our memory array
 */
void lw(size_t instruction, uint16_t registers[], const size_t memory []) {

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

    cout << memLocation<<endl;
    //take the value found in the desired memory location and load it into $regDst
    registers[regDst] = memory[memLocation];
}

/**
 * sw(instruction, registers, memory)
 *
 * This function executes the sw instruction. It decodes bits 7 through 12 into the
 * source register and the address register, and sign extends the 7 bit immediate
 * field to get the offset. Then it takes the value found in the source register and
 * stores it into memory at the address found by taking the sum of the signed value
 * of offset plus the value in the address register.
 *
 * @param instruction - The numerical value of the current instruction
 * @param registers - Our register array
 * @param memory - Our memory array
 */
void sw(size_t instruction, const uint16_t registers[], size_t memory[]) {

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
 * simulation(memory, registers)
 *
 * This function acts as a high level controller for our simulator. It takes in the memory array, which
 * contains our instructions, and our register array. It checks the 3 most significant bits and calls
 * the corresponding function to execute the instruction, and updates the pc. It performs wrap-around
 * logic to prevent the pc from exceeding its max value of 8191, and also enforces $0's immutability by
 * changing its value to 0 at the end of every instruction execution. It will run infinitely, going
 * through each element of the memory array unless/until it encounters a halt instruction, and then
 * returns the final value of the pc, which is where the halt is.
 *
 * @param memory - Our memory array, contains all of our instructions
 * @param registers - Our register array
 * @return Returns the final pc
 */
uint16_t simulation(size_t memory [], uint16_t registers []){
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
                lw(instruction, registers, memory);
                pc++;
                break;
            case 5: //sw
                sw(instruction, registers, memory);
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
    Prints the current state of the simulator, including
    the current program counter, the current register values,
    and the first memquantity elements of memory.

    @param pc The final value of the program counter
    @param regs Final value of all registers
    @param memory Final value of memory
    @param memquantity How many words of memory to dump
*/
void print_state(size_t pc, uint16_t regs[], size_t memory[], size_t memquantity) {
    cout << setfill(' ');
    cout << "Final state:" << endl;
    cout << "\tpc=" <<setw(5)<< pc << endl;

    for (size_t reg=0; reg<NUM_REGS; reg++)
        cout << "\t$" << reg << "="<<setw(5)<<regs[reg]<<endl;

    cout << setfill('0');
    bool cr = false;
    for (size_t count=0; count<memquantity; count++) {
        cout << hex << setw(4) << memory[count] << " ";
        cr = true;
        if (count % 8 == 7) {
            cout << endl;
            cr = false;
        }
    }
    if (cr)
        cout << endl;
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
        cerr << "Simulate E20 machine" << endl << endl;
        cerr << "positional arguments:" << endl;
        cerr << "  filename    The file containing machine code, typically with .bin suffix" << endl<<endl;
        cerr << "optional arguments:"<<endl;
        cerr << "  -h, --help  show this help message and exit"<<endl;
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

    //Do simulation
    uint16_t pc;
    pc = simulation(memory, registers);

    //Print the final state of the simulator before ending, using print_state
    print_state(pc, registers, memory, 128);

    return 0;
}
//ra0Eequ6ucie6Jei0koh6phishohm9
