# cpu-simulation
This is a simulated version of a CPU, assembler, and LRU cache for the E20 assembly language written in C++.

E20 is an assembly language used as a pedagogical tool in computer architecture by Professor Epstein at NYU Tandon School of Engineering. 
This was a semester-long project done in Spring 2021. It consists of 3 cpp files and one header file. 

The first program to be run is asm.cpp and it takes in a .s file containing a program written in E20. It assembles this into machine code and outputs a .bin file containing
the program. The next program to be run is sim.cpp, which takes in a .bin file containing E20 program instructions and executes it. The third program is simcache.cpp which
includes LRUCache.h and also takes a .bin file as its input, as well as a cache size, associativity, and blocksize in bytes. It can simulate the effects of one or two caches. 
It simulates the performance effect of an LRU (Least Recently Used) cache on a given E20 program, by cataloging and printing out the cache hits and misses for each attempted 
memory access by the program.

Sample input to asm:
math.s

Contents of math.s:
# Some simple math stuff

# add, addi, and sub work as you expect, except
# they take three arguments. The first argument
# is the destination, the other two are the
# sources.
# There is no subi. Instead you have to use addi
# with a negative immediate.
addi $1, $0, 5      # $1 := 5
addi $2, $1, -2     # $2 := $1 + (-2)
add $3, $1, $2      # $3 := $1 + $2

# Notice that $0 is special, because it is always
# zero. So using $0 as a source lets us effectively
# do a movi, as in the first instruction below.
addi $4, $0, 55     # $4 := 55
sub $5, $4,$1      # $5 := $4 - $1

# We also have bitwise operators AND and OR.
# (but not ori and andi). As above, the first
# operand is the destination register.
or $6, $2, $5
and $7,$2, $5

halt       # end the program with an infinite loop

Sample output of asm:
ram[0] = 16'b1110000010000101;
ram[1] = 16'b1110010101111110;
ram[2] = 16'b0000010100110000;
ram[3] = 16'b1110001000110111;
ram[4] = 16'b0001000011010001;
ram[5] = 16'b0000101011100011;
ram[6] = 16'b0000101011110010;
ram[7] = 16'b0100000000000111;

Sample input to sim:
math.bin

Contents of math.bin:
ram[0] = 16'b1110000010000101;		// addi $1,$0,5
ram[1] = 16'b1110010101111110;		// addi $2,$1,-2
ram[2] = 16'b0000010100110000;		// add $3,$1,$2
ram[3] = 16'b1110001000110111;		// addi $4,$0,55
ram[4] = 16'b0001000011010001;		// sub $5,$4,$1
ram[5] = 16'b0000101011100011;		// or $6,$2,$5
ram[6] = 16'b0000101011110010;		// and $7,$2,$5
ram[7] = 16'b0100000000000111;		// halt 

Sample output of sim:
Final state:
        pc=    7
        $0=    0
        $1=    5
        $2=    3
        $3=    8
        $4=   55
        $5=   50
        $6=   51
        $7=    2
e085 e57e 0530 e237 10d1 0ae3 0af2 4007
0000 0000 0000 0000 0000 0000 0000 0000
0000 0000 0000 0000 0000 0000 0000 0000


Sample input to simcache:
stride4.bin --cache 32,2,4

First 12 lines of stride4.bin:
ram[0] = 16'b1110000010110101;		// movi $1,53
ram[1] = 16'b1000001110010001;		// lw $7,hundred($0)
ram[2] = 16'b1011110010000000;		// sw $1,0($7)
ram[3] = 16'b1011110010000100;		// sw $1,4($7)
ram[4] = 16'b1011110010001000;		// sw $1,8($7)
ram[5] = 16'b1011110010001100;		// sw $1,12($7)
ram[6] = 16'b1011110010010000;		// sw $1,16($7)
ram[7] = 16'b1110000100001010;		// movi $2,10
ram[8] = 16'b1100100000000111;		// loop: jeq $2,$0,done
ram[9] = 16'b1110100101111111;		// addi $2,$2,-1
ram[10] = 16'b1001110010000000;		// lw $1,0($7)
ram[11] = 16'b1001110010000100;		// lw $1,4($7)
ram[12] = 16'b1001110010001000;		// lw $1,8($7)

Partial sample output of simcache:
Cache L1 has size 32, associativity 2, blocksize 4, lines 4
L1 MISS  pc:    1       addr:   17      line:   0
L1 SW    pc:    2       addr:  100      line:   1
L1 SW    pc:    3       addr:  104      line:   2
L1 SW    pc:    4       addr:  108      line:   3
L1 SW    pc:    5       addr:  112      line:   0
L1 SW    pc:    6       addr:  116      line:   1
L1 HIT   pc:   10       addr:  100      line:   1
L1 HIT   pc:   11       addr:  104      line:   2
L1 HIT   pc:   12       addr:  108      line:   3
