# cpu-simulation
This is a simulated version of a CPU, assembler, and LRU cache for the E20 assembly language written in C++.

E20 is an assembly language used as a pedagogical tool in computer architecture by Professor Epstein at NYU Tandon School of Engineering. 
This was a semester-long project done in Spring 2021. It consists of 3 cpp files and one header file. 

The first program to be run is asm.cpp and it takes in a .s file containing a program written in E20. It assembles this into machine code and outputs a .bin file containing
the program. The next program to be run is sim.cpp, which takes in a .bin file containing E20 program instructions and executes it. The third program is simcache.cpp which
includes LRUCache.h and also takes a .bin file as its input. It simulates the performance effect of an LRU (Least Recently Used) cache on a given E20 program, by cataloging 
the cache hits and misses for each attempted memory access by the program.
