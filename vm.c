#include <stdio.h> //for fopen, printf
#include <stdlib.h> //for malloc, exit
#include <string.h> //for strcmp

#include "types.h"
#include "vm.h"
#include "parse.h"

char mem[MEM_SIZE]={0};
int text_top_empty = 0; //add text at this index
int data_top_empty = TEXT_SIZE; //add data at this index

int pc=0, bp=MEM_SIZE, sp=MEM_SIZE, ax=0; // vm registers

extern struct_symbol symbols[];
extern struct_type types[];

void print_text()
{
    for(int pc=0; pc<text_top_empty;)
    {
        pc = print_instruction(pc);
    }
}

int print_instruction(int pc)
{
    int op;
    op = *((int*)&mem[pc]);

    if (op == IMM /*<val>*/)
                        {printf("%d: IMM %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}
    else if (op == LC)
                        {printf("%d: LC\n", pc); pc+=4;}
    else if (op == LI)
                        {printf("%d: LI\n", pc); pc+=4;}
    else if (op == SC)
                        {printf("%d: SC\n", pc); pc+=4;}
    else if (op == SI)
                        {printf("%d: SI\n", pc); pc+=4;}
    else if (op == PUSH)
                        {printf("%d: PUSH\n", pc); pc+=4;}
    else if (op == JMP /*<addr>*/)
                        {printf("%d: JMP %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}
    else if (op == JZ /*<addr>*/)
                        {printf("%d: JZ %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}
    else if (op == JNZ /*<addr>*/)
                        {printf("%d: JNZ %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}
    else if (op == CALL /*<addr>*/)
                        {printf("%d: CALL %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}
    else if (op == ICALL)
                        {printf("%d: ICALL\n", pc); pc+=4;}
    else if (op == SWAP)
                        {printf("%d: SWAP\n", pc); pc+=4;}
    //else if (op == RET)
                        //{printf("%d: RET\n", pc); pc+=4;}
    else if (op == ENT /*<size>*/)
                        {printf("%d: ENT %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}
    else if (op == ADJ /*<size>*/)
                        {printf("%d: ADJ %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}
    else if (op == LEV)
                        {printf("%d: LEV\n", pc); pc+=4;}
    else if (op == LEA /*<argnum>*/)
                        {printf("%d: LEA %d\n", pc, *((int*)&mem[pc+4])); pc+=8;}

    else if (op == OR)  {printf("%d: OR\n", pc); pc+=4;}
    else if (op == XOR) {printf("%d: XOR\n", pc); pc+=4;}
    else if (op == AND) {printf("%d: AND\n", pc); pc+=4;}
    else if (op == EQ)  {printf("%d: EQ\n", pc); pc+=4;}
    else if (op == NE)  {printf("%d: NE\n", pc); pc+=4;}
    else if (op == LT)  {printf("%d: LT\n", pc); pc+=4;}
    else if (op == LE)  {printf("%d: LE\n", pc); pc+=4;}
    else if (op == GT)  {printf("%d: GT\n", pc); pc+=4;}
    else if (op == GE)  {printf("%d: GE\n", pc); pc+=4;}
    else if (op == SHL) {printf("%d: SHL\n", pc); pc+=4;}
    else if (op == SHR) {printf("%d: SHR\n", pc); pc+=4;}
    else if (op == ADD) {printf("%d: ADD\n", pc); pc+=4;}
    else if (op == SUB) {printf("%d: SUB\n", pc); pc+=4;}
    else if (op == MUL) {printf("%d: MUL\n", pc); pc+=4;}
    else if (op == DIV) {printf("%d: DIV\n", pc); pc+=4;}
    else if (op == MOD) {printf("%d: MOD\n", pc); pc+=4;}

    else if (op == HLT) {printf("%d: HLT\n", pc); pc+=4;}
    else if (op == ININT) {printf("%d: ININT\n", pc); pc+=4;}
    else if (op == INCHAR) {printf("%d: INCHAR\n", pc); pc+=4;}
    else if (op == OUTSTR) {printf("%d: OUTSTR\n", pc); pc+=4;}
    else if (op == OUTINT) {printf("%d: OUTINT\n", pc); pc+=4;}
    else if (op == OUTCHAR) {printf("%d: OUTCHAR\n", pc); pc+=4;}
    else
    {
        printf("unknown instruction: %d\n", op);
        pc+=4;
    }
    return pc;
}

void print_data()
{
    for(int i=TEXT_SIZE; i<data_top_empty;)
    {
        printf("%d: ", i); printf("%c %#x\n", mem[i], mem[i]);
        printf("     : "); printf("%c %#x\n", mem[i+1], mem[i+1]);
        printf("     : "); printf("%c %#x\n", mem[i+2], mem[i+2]);
        printf("     : "); printf("%c %#x\n", mem[i+3], mem[i+3]);

        i+=4;
    }
}

void print_reg()
{
    printf("                pc=%d sp=%d bp=%d ax=%d ", pc, sp, bp, ax);
    if(sp<MEM_SIZE)
        printf("mem[sp]=%d ", *((int*)&mem[sp]));

    if(ax == *((int*)&mem[sp]))
    {
        //print once
        if(ax>=TEXT_SIZE && ax<TEXT_SIZE+DATA_SIZE)
            printf("mem[%d]=%d ", ax, *((int*)&mem[ax]));
    }
    else
    {
        if(ax>=TEXT_SIZE && ax<TEXT_SIZE+DATA_SIZE)
            printf("mem[%d]=%d ", ax, *((int*)&mem[ax]));

        if(*((int*)&mem[sp])>=TEXT_SIZE && *((int*)&mem[sp])<TEXT_SIZE+DATA_SIZE)
            printf("mem[%d]=%d ", *((int*)&mem[sp]), *((int*)&mem[*((int*)&mem[sp])]));
    }

    printf("\n");
}

void put_opcode(int op)
{
    *((int*)&mem[text_top_empty]) = op;
    text_top_empty = text_top_empty + 4;
}

void put_operand(int operand)
{
    *((int*)&mem[text_top_empty]) = operand;
    text_top_empty = text_top_empty + 4;
}

int get_text_addr_here()
{
    return text_top_empty;
}

void fix_operand(int addr, int operand)
{
    *((int*)&mem[addr]) = operand;
}

int get_bytes_req(int size)
{
    int ints_req = size/4; //assume int is 4 bytes
    if(ints_req*4 != size)
        ints_req = ints_req + 1; //to align size to int
    //otherwise it is good,
    //we are already aligned to int
    return ints_req*4;
}

int alloc_strlit(int symbol_index)
{
    int data_addr;

    int len = strlen(symbols[symbol_index].text);
    len = len + 1; //for '\0'

    int bytes_req = get_bytes_req(len); //get aligned

    //treat data[] as char and copy string to it
    char* p_data = &mem[data_top_empty];
    strcpy(p_data, symbols[symbol_index].text);

    data_addr = data_top_empty; //keep addr to return
    data_top_empty = data_top_empty + bytes_req;

    return data_addr;
}

int alloc_ident(int symbol_index)
{
    int data_addr;

    int ti = get_type_index(symbols[symbol_index].type);
    int size = types[ti].sizebyte;

    int bytes_req = get_bytes_req(size); //get aligned

    data_addr = data_top_empty; //keep addr to return
    data_top_empty = data_top_empty + bytes_req;

    return data_addr;
}

int run_vm()
{
    int op;

    if(DEBUG)
        print_reg();

    while (1)
    {
        if(DEBUG)
            print_instruction(pc);

        op = *((int*)&mem[pc]); pc+=4; // get operand or next opcode

        if (op == IMM /*<val>*/)// load immediate value to ax
                                {ax = *((int*)&mem[pc]); pc+=4;}

        else if (op == LC)  // load character to ax, address in ax
                            {ax = mem[ax];}

        else if (op == LI)  // load integer to ax, address in ax
                            {ax = *((int*)&mem[ax]);}

        else if (op == SC)  // save character to address, value in ax, address on stack
                            {int addr = *((int*)&mem[sp]); sp+=4; mem[addr] = ax;}

        else if (op == SI)  // save integer to address, value in ax, address on stack
                            {int addr = *((int*)&mem[sp]); sp+=4; *((int*)&mem[addr]) = ax;}

        else if (op == PUSH)// push the value of ax onto the stack
                            {sp-=4; *((int*)&mem[sp]) = ax;}

        else if (op == JMP /*<addr>*/)  // unconditional jump to the address
                                        {pc = *((int*)&mem[pc]);}

        else if (op == JZ /*<addr>*/)   // jump if ax is zero
                                        {if(ax==0){pc=*((int*)&mem[pc]);}else{pc+=4;}}

        else if (op == JNZ /*<addr>*/)  // jump if ax is not zero
                                        {if(ax!=0){pc=*((int*)&mem[pc]);}else{pc+=4;}}

        else if (op == CALL /*<addr>*/) // call subroutine
                                        {sp-=4; *((int*)&mem[sp])=pc+4; pc=*((int*)&mem[pc]);}

        else if (op == ICALL)   // call subroutine indirectly
                                {sp-=4; *((int*)&mem[sp])=pc; pc=ax;}

        else if (op == SWAP)   // swap top two elements
                               { int top = *((int*)&mem[sp]);        // top of stack
                                 int second = *((int*)&mem[sp + 4]); // second from top
                                 // swap them
                                 *((int*)&mem[sp]) = second;
                                 *((int*)&mem[sp + 4]) = top; }

        //else if (op == RET)   // return from subroutine;
                                //{pc = *((int*)&mem[sp]); sp+=4;}

        else if (op == ENT /*<size>*/)  // make new stack frame
                                        {sp-=4; *((int*)&mem[sp])=bp; bp=sp; sp=sp-*((int*)&mem[pc]); pc+=4;}

        else if (op == ADJ /*<size>*/)  // add esp, <size>
                                        {sp=sp+*((int*)&mem[pc]); pc+=4;}

        else if (op == LEV) // restore call frame and PC
                            {sp=bp; bp=*((int*)&mem[sp]); sp+=4; pc=*((int*)&mem[sp]); sp+=4;}

        else if (op == LEA /*<argnum>*/)    // load address for arguments.
                                            {int argnum=*((int*)&mem[pc]); ax = bp+argnum; pc+=4;}

        else if (op == OR)  {ax = *((int*)&mem[sp]) | ax; sp+=4;}
        else if (op == XOR) {ax = *((int*)&mem[sp]) ^ ax; sp+=4;}
        else if (op == AND) {ax = *((int*)&mem[sp]) & ax; sp+=4;}
        else if (op == EQ)  {ax = *((int*)&mem[sp]) == ax; sp+=4;}
        else if (op == NE)  {ax = *((int*)&mem[sp]) != ax; sp+=4;}
        else if (op == LT)  {ax = *((int*)&mem[sp]) < ax; sp+=4;}
        else if (op == LE)  {ax = *((int*)&mem[sp]) <= ax; sp+=4;}
        else if (op == GT)  {ax = *((int*)&mem[sp]) >  ax; sp+=4;}
        else if (op == GE)  {ax = *((int*)&mem[sp]) >= ax; sp+=4;}
        else if (op == SHL) {ax = *((int*)&mem[sp]) << ax; sp+=4;}
        else if (op == SHR) {ax = *((int*)&mem[sp]) >> ax; sp+=4;}
        else if (op == ADD) {ax = *((int*)&mem[sp]) + ax; sp+=4;}
        else if (op == SUB) {ax = *((int*)&mem[sp]) - ax; sp+=4;}
        else if (op == MUL) {ax = *((int*)&mem[sp]) * ax; sp+=4;}
        else if (op == DIV) {ax = *((int*)&mem[sp]) / ax; sp+=4;}
        else if (op == MOD) {ax = *((int*)&mem[sp]) % ax; sp+=4;}

        else if (op == HLT) { printf("halt(%d)\n", ax); return ax;} //halt, print content of ax
        else if (op == ININT) { if(0 == scanf("%d", &ax)) { ax = 0; scanf("%*s"); } } //get input integer to ax
        //scanf returns 1 if it reads one integer,
        //if it finds invalid input, it returns 0
        //then we assign 0 to ax as a default value
        //and discard the rest in buffer by scanf("%*s");
        else if (op == INCHAR) { char c; scanf(" %c", &c); ax=c;} //get input character to ax
        //if we write "%c", it gets one character and keeps the pressed 'enter' in buffer, which is given to next scanf
        //if we write " %c", the space before %c asks scanf to discard white spaces, so pressed 'enter' is flushed and next scanf behaves well
        else if (op == OUTSTR) { printf("%s\n", &mem[ax] );} //print string, address in ax
        else if (op == OUTINT) { printf("%d\n", ax);} //print integer present in ax
        else if (op == OUTCHAR) { char c; c=ax; printf("%c\n", c);} //print character present in ax
        else
        {
            printf("unknown instruction: %d\n", op);
            return 1;
        }

        if(DEBUG)
            print_reg();
    }
    return 0;
}

/*
main()
{
    int arg1=3;
    int arg2=4;
    int c;

    PUSH arg2
    PUSH arg1
    CALL add(arg1, arg2)
    ADJ 2 //clean space of pushed args

    c = AX
}

add()
{
    int r;

    ENT 1 //make space for local var r

    //code
    LEA -1 //get address of local var r

    ADJ 1 //clean space of local var r
    LEV
}
*/
