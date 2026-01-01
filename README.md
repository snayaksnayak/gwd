
# GWD Programming Language

A new programming language, **GWD** /good/,  
and its compiler/interpreter with its VM,  
dedicated to **Gries, Wirth and Dijkstra**.

David Gries, Niklaus Wirth and Edsger Dijkstra  
are the pioneers of computer science,  
programming languages and compilers.

GWD /good/ supports both procedural and  
object oriented programming paradigms.

---

## To Build and Test

**DEBUG** and **INTERPRET** macros are in `types.h`  
enable and disable as you need.

```sh
$ gcc -g lex.c parse.c bigparse.c emit.c vm.c gwd.c
$ ./a.out ./examples/average.gwd
$ cat out.ir.txt
````

look at `build.sh` for more info,
and how to include memory manager.

---

## Done

* IR output
* vm ISA: 32bit opcode, optional 32bit operand
* disassembler
* interpreter
* debugger
* int datatype 32bit
* char datatype 8bit
* relational operator
* arithmetic operator
* unary arithmetic operator
* assignment
* branching
* looping
* int array
* char array
* record
* pointer datatype
* ref var for easiness
* function
* print string literal
* print char array
* must funcs in record
* can funcs in record
* must var to hold objects
* polymorphism
* new and free
* boolean operator

---

## Plan

* ptr ownership transfer


