# cpu-simulation
This is a simulated version of a CPU, assembler, and LRU cache for the E20 assembly language written in C++.

E20 is an assembly language used as a pedagogical tool in computer architecture by Professor Epstein at NYU Tandon School of Engineering. 
This was a semester-long project done in Spring 2021. It consists of 3 cpp files and one header file. 

The first program to be run is asm.cpp and it takes in a .s file containing a program written in E20. It assembles this into machine code and outputs a .bin file containing
the program. An example of this can be found [here](sample-asm.txt). The next program to be run is sim.cpp, which takes in a .bin file containing E20 program instructions 
and executes it. It prints out the final state of all 7 registers, as well as the program counter and a portion of all the memory in hex. An example of this can be found 
[here](sample-sim.txt). The third program is simcache.cpp which includes LRUCache.h and also takes a .bin file as its input, as well as a cache size, associativity, and 
blocksize in bytes. It can simulate the effects of one or two caches. It simulates the performance effect of an LRU (Least Recently Used) cache on a given E20 program, by 
cataloging and printing out the cache hits and misses for each attempted memory access by the program. An example of that can be found [here](sample-simcache.txt).
