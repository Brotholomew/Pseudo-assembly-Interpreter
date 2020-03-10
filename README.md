# Pseudo-assembly Interpreter
 Project in C, which goal is to interpret the assembly language developed at the Warsaw University of Technology for educational purposes. The user interface is all in English, however the documentation is unfortunately only in Polish.

## About pseudoassembler
Pseudoassembler is a low-level programming language which is taught at Faculty of Mathematics and Information Science at Warsaw University of Technology. 

## System requirements
It is best to use Windows 10 (tested on v. 1903 and newer) and Windows 7. 

_Why Windows?_

The project will work properly only on the Microsoft OS due to my professor's instructions to use precisely the windows.h library for aesthetic display of the virtual machine's state. 

## How to run
After compiling the code, one will be prompted to specify the path with pseudo-assembly code to be interpreted, e.g.:
```
_______ PSEUDO ASSEMBLER INTERPRETER _______
Please provide the path for the file containing the assembler instructions: ../_tests/bubble_sort.txt
```

Then one can decide whether to use the _debug mode_ or _normal mode_
```
Run in debug mode? [y/n]: y
```

## Tests
You can find tests with pseudo-assembly code in the file `_tests`

## Status
Project is: _completed_.

