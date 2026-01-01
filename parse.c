#include <stdio.h> //for fopen, printf
#include <stdlib.h> //for malloc, exit
#include <memory.h> //for memcp
#include <string.h> //for strcmp

#include "types.h"
#include "lex.h"
#include "parse.h"
#include "emit.h"
#include "vm.h"

extern struct_token token; //in lex.c
extern char *source; //in lex.c
extern int line_number; //in lex.c

struct_token cur_token;
struct_token upcoming_token;

//let's not distinguish between
//declaration and definition.
//say declaration(dec) = definition(def).
//in programming we have two things.
//data and code.
//data is our variables.
//code is our functions.
//data has type and instance.
//  we know them as "datatype" dec/def and "variable" dec/def.
//function also has type and instance.
//  we know them as "prototype" dec and "function" def.
//difference is that,
//we have "many" "variables" for "one" "datatype".
//  that is why we separated struct_type and struct_symbol
//  struct_type is for "datatype" dec/def
//  struct_symbol is for "variable" dec/def
//but we have "one" "function" for "one" "prototype".
//  that is why we have only struct_func
//  struct_func is for "prototype" dec
//  and "function" def both

//"datatype" dec/def
struct_type types[MAX_TYPES];
int num_types=2;

//"variable" dec/def
struct_symbol symbols[MAX_SYMBOLS];
// All variables we have declared so far.
// and strings that we have used so far.
int num_symbols=0;

//"prototype" dec and
//"function" def both
//for general funcs
//and mangled must funcs
//and mangled can funcs
struct_func funcs[MAX_FUNCS];
int num_funcs=0;

//"prototype" dec only
//for must funcs
//just for matching purpose
    //"function" def
    //for must funcs
    //are inside funcs
struct_func musts[MAX_MUSTS];
int num_musts=0;

struct_fix fixfaddr[MAX_FIXES];
int num_fixes=0;

int convert_text_to_num(char* text)
{
    int len = strlen(text);
    if(len > 9)
    {
        printf("%d: Warning: Numbers more than 9 digits are error codes.\n", line_number);
        //exit(1);
    }
    int num=0;
    for (int i=0; i<len; i++)
    {
        num = num*10 + (text[i]-48); //ascii of char 0 is 48
    }
    if(num >= 2000000000)
    {
        printf("%d: Error: Number >= 2000000000. Too big number.\n", line_number);
        exit(1);
    }
    return num;
}

int convert_char_to_num(char* text)
{
    return text[0]; //ascii of char literal
}

int members_contains(int rec_ti, char* memname)
{
    for(int i=0; i<types[rec_ti].rec_mem_cnt; i++)
    {
        if(0==strcmp(types[rec_ti].rec_mem_list[i].text, memname))
        {
            return 1;
        }
    }
    return 0;
}

void members_add(int rec_ti, char* memname, int mem_ti)
{
    int member_index = types[rec_ti].rec_mem_cnt; //index for new member
    types[rec_ti].rec_mem_cnt += 1; //increase total count
    if(types[rec_ti].rec_mem_cnt > MAX_MEMBERS)
    {
        printf("%d: member table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(types[rec_ti].rec_mem_list[member_index].text, memname);
        strcpy(types[rec_ti].rec_mem_list[member_index].type, types[mem_ti].text);
        if (member_index == 0)
            types[rec_ti].rec_mem_list[member_index].addr=0;
        else
        {
            int prev_mem_addr = types[rec_ti].rec_mem_list[member_index-1].addr;
            int prev_mem_ti = get_type_index(types[rec_ti].rec_mem_list[member_index-1].type);
            int prev_mem_size = types[prev_mem_ti].sizebyte;
            int prev_mem_size_aligned = get_bytes_req(prev_mem_size); //get aligned

            types[rec_ti].rec_mem_list[member_index].addr=prev_mem_addr + prev_mem_size_aligned;
        }
    }
}

int get_member_index(int rec_ti, char* memname)
{
    for(int i=0; i<types[rec_ti].rec_mem_cnt; i++)
    {
        if(0==strcmp(types[rec_ti].rec_mem_list[i].text, memname))
        {
            return i;
        }
    }
    return -1;
}

int params_contains(int func_index, char* paramname)
{
    for(int i=0; i<funcs[func_index].func_param_cnt; i++)
    {
        if(0==strcmp(funcs[func_index].params[i].text, paramname))
        {
            return 1;
        }
    }
    return 0;
}

void params_add(int func_index, char* paramname, int param_ti)
{
    int param_index = funcs[func_index].func_param_cnt;
    //index for new param

    funcs[func_index].func_param_cnt += 1;
    //increase total count

    if(funcs[func_index].func_param_cnt > MAX_PARAMS)
    {
        printf("%d: param table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(funcs[func_index].params[param_index].text, paramname);
        strcpy(funcs[func_index].params[param_index].type, types[param_ti].text);
        //addr of params to be taken care after all params added

        //high address
        //arg_1:        new_bp+16
        //arg_2:        new_bp+12
        //arg_3:        new_bp+8
        //ret addr:     new_bp+4
        //old_bp:       new_bp
        //local_1:      new_bp-4
        //local_2:      new_bp-8
        //local_3:      new_bp-12
        //low address
    }
}

int get_param_index(int func_index, char* paramname)
{
    for(int i=0; i<funcs[func_index].func_param_cnt; i++)
    {
        if(0==strcmp(funcs[func_index].params[i].text, paramname))
        {
            return i;
        }
    }
    return -1;
}

int locals_contains(int func_index, char* localname)
{
    for(int i=0; i<funcs[func_index].func_local_cnt; i++)
    {
        if(0==strcmp(funcs[func_index].locals[i].text, localname))
        {
            return 1;
        }
    }
    return 0;
}

void locals_add(int func_index, char* localname, int local_ti)
{
    int local_index = funcs[func_index].func_local_cnt;
    //index for new local

    funcs[func_index].func_local_cnt += 1;
    //increase total count

    if(funcs[func_index].func_local_cnt > MAX_PARAMS)
    {
        printf("%d: local table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(funcs[func_index].locals[local_index].text, localname);
        strcpy(funcs[func_index].locals[local_index].type, types[local_ti].text);
        //addr of locals to be taken care now

        if(local_index == 0)
        {
            funcs[func_index].locals[local_index].addr = -4;
            //offset to bp
        }
        else
        {
            int prev_local_addr = funcs[func_index].locals[local_index-1].addr;
            funcs[func_index].locals[local_index].addr = prev_local_addr - 4;
        }

        //high address
        //arg_1:        new_bp+16
        //arg_2:        new_bp+12
        //arg_3:        new_bp+8
        //ret addr:     new_bp+4
        //old_bp:       new_bp
        //local_1:      new_bp-4
        //local_2:      new_bp-8
        //local_3:      new_bp-12
        //low address
    }
}

int get_local_index(int func_index, char* localname)
{
    for(int i=0; i<funcs[func_index].func_local_cnt; i++)
    {
        if(0==strcmp(funcs[func_index].locals[i].text, localname))
        {
            return i;
        }
    }
    return -1;
}

int mparams_contains(int mfunc_index, char* paramname)
{
    for(int i=0; i<musts[mfunc_index].func_param_cnt; i++)
    {
        if(0==strcmp(musts[mfunc_index].params[i].text, paramname))
        {
            return 1;
        }
    }
    return 0;
}

void mparams_add(int mfunc_index, char* paramname, int param_ti)
{
    int param_index = musts[mfunc_index].func_param_cnt;
    //index for new param

    musts[mfunc_index].func_param_cnt += 1;
    //increase total count

    if(musts[mfunc_index].func_param_cnt > MAX_PARAMS)
    {
        printf("%d: param table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(musts[mfunc_index].params[param_index].text, paramname);
        strcpy(musts[mfunc_index].params[param_index].type, types[param_ti].text);
        //addr of params to be taken care after all params added

        //high address
        //arg_1:        new_bp+16
        //arg_2:        new_bp+12
        //arg_3:        new_bp+8
        //ret addr:     new_bp+4
        //old_bp:       new_bp
        //local_1:      new_bp-4
        //local_2:      new_bp-8
        //local_3:      new_bp-12
        //low address
    }
}

int get_mparam_index(int mfunc_index, char* paramname)
{
    for(int i=0; i<musts[mfunc_index].func_param_cnt; i++)
    {
        if(0==strcmp(musts[mfunc_index].params[i].text, paramname))
        {
            return i;
        }
    }
    return -1;
}

int symbols_contains(char* symname)
{
    for(int i=0; i<num_symbols; i++)
    {
        if(0==strcmp(symbols[i].text, symname))
        {
            return 1;
        }
    }
    return 0;
}

void symbols_add(struct_token t, int type_index)
{
    int symbol_index = num_symbols; //index for new symbol
    num_symbols += 1; //increase total count
    if(num_symbols > MAX_SYMBOLS)
    {
        printf("%d: symbol table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(symbols[symbol_index].text, t.text);

        if (t.kind == STRLIT) //if string literal
        {
            strcpy(symbols[symbol_index].type, "string literal");
            symbols[symbol_index].addr=alloc_strlit(symbol_index);
        }
        else
        {
            strcpy(symbols[symbol_index].type, types[type_index].text);
            symbols[symbol_index].addr=alloc_ident(symbol_index);
        }
    }
}

int get_symbol_index(char* symname)
{
    for(int i=0; i<num_symbols; i++)
    {
        if(0==strcmp(symbols[i].text, symname))
        {
            return i;
        }
    }
    return -1;
}

int types_contains(char* typename)
{
    for(int i=0; i<num_types; i++)
    {
        if(0==strcmp(types[i].text, typename))
        {
            return 1;
        }
    }
    return 0;
}

int get_type_index(char* typename)
{
    for(int i=0; i<num_types; i++)
    {
        if(0==strcmp(types[i].text, typename))
        {
            return i;
        }
    }
    return -1;
}

//adds only type name
void types_add(char* typename)
{
    int type_index = num_types; //index for new type
    num_types += 1; //increase total count
    if(num_types > MAX_TYPES)
    {
        printf("%d: type table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(types[type_index].text, typename);
    }
}

int funcs_contains(char* funcname)
{
    for(int i=0; i<num_funcs; i++)
    {
        if(0==strcmp(funcs[i].text, funcname))
        {
            return 1;
        }
    }
    return 0;
}

int get_func_index(char* funcname)
{
    for(int i=0; i<num_funcs; i++)
    {
        if(0==strcmp(funcs[i].text, funcname))
        {
            return i;
        }
    }
    return -1;
}

void funcs_add(char* funcname)
{
    int func_index = num_funcs; //index for new function
    num_funcs += 1; //increase total count
    if(num_funcs > MAX_FUNCS)
    {
        printf("%d: function table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(funcs[func_index].text, funcname);
    }

    funcs[func_index].addr = INVALID_ADDR;
    funcs[func_index].func_param_cnt = 0;
    funcs[func_index].func_local_cnt = 0;
}

int musts_contains(char* funcname)
{
    for(int i=0; i<num_musts; i++)
    {
        if(0==strcmp(musts[i].text, funcname))
        {
            return 1;
        }
    }
    return 0;
}

int get_must_index(char* funcname)
{
    for(int i=0; i<num_musts; i++)
    {
        if(0==strcmp(musts[i].text, funcname))
        {
            return i;
        }
    }
    return -1;
}

void musts_add(char* funcname)
{
    int mfunc_index = num_musts; //index for new function
    num_musts += 1; //increase total count
    if(num_musts > MAX_MUSTS)
    {
        printf("%d: must function table overflow!\n", line_number);
        exit(1);
    }
    else
    {
        strcpy(musts[mfunc_index].text, funcname);
    }

    musts[mfunc_index].addr = INVALID_ADDR;
    musts[mfunc_index].func_param_cnt = 0;
    musts[mfunc_index].func_local_cnt = 0;
}

int get_addr_of_symbol(struct_token t)
{
    int addr = INVALID_ADDR;
    for(int i=0; i<num_symbols; i++)
    {
        if(0==strcmp(symbols[i].text, t.text))
        {
            return symbols[i].addr;
        }
    }
    return addr;
}

void print_symbols()
{
    for(int i=0; i<num_symbols; i++)
    {
        printf("\n");
        printf("symbol text: %s\n", symbols[i].text);
        printf("symbol type: %s\n", symbols[i].type);
        printf("symbol addr: %d\n", symbols[i].addr);
    }
}

void print_types()
{
    for(int i=0; i<num_types; i++)
    {
        printf("\n");
        printf("type text: %s\n", types[i].text);
        printf("type catg: %d    (", types[i].catg);
        printf("0=int, 1=char, 2=ptr, 3=arr, 4=rec, 5=func)\n");
        printf("type sizebyte: %d\n", types[i].sizebyte);

        printf("type ptr_val_type: %s\n", types[i].ptr_val_type);

        printf("type arr_mem_type: %s\n", types[i].arr_mem_type);
        printf("type arr_mem_cnt: %d\n", types[i].arr_mem_cnt);

        printf("type rec_mem_cnt: %d\n", types[i].rec_mem_cnt);
        for(int j=0; j<types[i].rec_mem_cnt; j++)
        {
            if (j!=0) printf("    ----\n");
            printf("    member text: %s\n", types[i].rec_mem_list[j].text);
            printf("    member type: %s\n", types[i].rec_mem_list[j].type);
            printf("    member addr: %d\n", types[i].rec_mem_list[j].addr);
        }
    }
}

void print_funcs()
{
    for(int i=0; i<num_funcs; i++)
    {
        printf("\n");
        printf("func text: %s\n", funcs[i].text);
        printf("func addr: %d\n", funcs[i].addr);

        printf("func func_param_cnt: %d\n", funcs[i].func_param_cnt);
        for(int j=0; j<funcs[i].func_param_cnt; j++)
        {
            if (j!=0) printf("    ----\n");
            printf("    param text: %s\n", funcs[i].params[j].text);
            printf("    param type: %s\n", funcs[i].params[j].type);
            printf("    param addr: %d\n", funcs[i].params[j].addr);
        }

        printf("func func_local_cnt: %d\n", funcs[i].func_local_cnt);
        for(int j=0; j<funcs[i].func_local_cnt; j++)
        {
            if (j!=0) printf("    ----\n");
            printf("    local text: %s\n", funcs[i].locals[j].text);
            printf("    local type: %s\n", funcs[i].locals[j].type);
            printf("    local addr: %d\n", funcs[i].locals[j].addr);
        }
    }
}

void print_musts()
{
    for(int i=0; i<num_musts; i++)
    {
        printf("\n");
        printf("must text: %s\n", musts[i].text);
        printf("must addr: %d\n", musts[i].addr);

        printf("must func_param_cnt: %d\n", musts[i].func_param_cnt);
        for(int j=0; j<musts[i].func_param_cnt; j++)
        {
            if (j!=0) printf("    ----\n");
            printf("    param text: %s\n", musts[i].params[j].text);
            printf("    param type: %s\n", musts[i].params[j].type);
            printf("    param addr: %d\n", musts[i].params[j].addr);
        }

        printf("must func_local_cnt: %d\n", musts[i].func_local_cnt);
        for(int j=0; j<musts[i].func_local_cnt; j++)
        {
            if (j!=0) printf("    ----\n");
            printf("    local text: %s\n", musts[i].locals[j].text);
            printf("    local type: %s\n", musts[i].locals[j].type);
            printf("    local addr: %d\n", musts[i].locals[j].addr);
        }
    }
}

char* get_kind_string(int kind)
{
    switch(kind)
    {
        case TBD: return "TBD";
        case ENDOFFILE: return "ENDOFFILE";
        case NEWLINE: return "NEWLINE";
        case INTLIT: return "INTLIT";
        case CHARLIT: return "CHARLIT";
        case IDENT: return "IDENT";
        case STRLIT: return "STRLIT";
        case LPAREN: return "LPAREN";
        case RPAREN: return "RPAREN";
        case DOT: return "DOT";
        case LBRKT: return "LBRKT";
        case RBRKT: return "RBRKT";
        case COMMA: return "COMMA";
        case LBRACE: return "LBRACE";
        case RBRACE: return "RBRACE";
        case DOTDOT: return "DOTDOT";
        case TWOCOLON: return "TWOCOLON";

        case TYPE: return "TYPE";
        case PRINT: return "PRINT";
        case INPUT: return "INPUT";
        case IF: return "IF";
        case THEN: return "THEN";
        case ELSE: return "ELSE";
        case ENDIF: return "ENDIF";
        case WHILE: return "WHILE";
        case REPEAT: return "REPEAT";
        case ENDWHILE: return "ENDWHILE";
        case INT: return "INT";
        case CHAR: return "CHAR";
        case POINTS: return "POINTS";
        case ARRAY: return "ARRAY";
        case RECORD: return "RECORD";
        case ENDREC: return "ENDREC";
        case BIND: return "BIND";
        case NEW: return "NEW";
        case MOVE: return "MOVE";
        case FREE: return "FREE";
        case REF: return "REF";
        case FUNC: return "FUNC";
        case FDEF: return "FDEF";
        case RETURN: return "RETURN";
        case MUST: return "MUST";
        case MDEF: return "MDEF";
        case CAN: return "CAN";
        case CDEF: return "CDEF";
        case POLY: return "POLY";

        case EQUAL: return "EQUAL";
        case PLUS: return "PLUS";
        case MINUS: return "MINUS";
        case ASTERISK: return "ASTERISK";
        case SLASH: return "SLASH";
        case EQEQ: return "EQEQ";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LTEQ: return "LTEQ";
        case GRET: return "GRET";
        case GTEQ: return "GTEQ";
        case PIPE: return "PIPE";
        case AMPERSAND: return "AMPERSAND";
        case TILDE: return "TILDE";

        default: return "????";
    }
}

void print_cur_token_text()
{
    if(cur_token.text[0] == '\n')
        printf("  cur: newline");
    else if (cur_token.text[0] == '\0')
        printf("  cur: nullchar");
    else
        printf("  cur: %s", cur_token.text);

    printf("\n");
}

void print_upcoming_token_text()
{
    if(upcoming_token.text[0] == '\n')
        printf("  upc: newline");
    else if (upcoming_token.text[0] == '\0')
        printf("  upc: nullchar");
    else
        printf("  upc: %s", upcoming_token.text);

    printf("\n");
}

void make_mandatory(int kind)
{
    if(check_cur_token(kind))
        consume_cur_token();
    else
    {
        printf("%d: Error! Expected %s, got %s\n", line_number,
            get_kind_string(kind), get_kind_string(cur_token.kind));
        exit(1);
    }
}

int cur_token_is_relop()
{
    return check_cur_token(GRET) || check_cur_token(GTEQ)
    || check_cur_token(LESS) || check_cur_token(LTEQ)
    || check_cur_token(EQEQ) || check_cur_token(NOTEQ);
}

void consume_cur_token()
{
    cur_token = upcoming_token;
    get_token();
    upcoming_token = token;
    //No need to worry about passing the ENDOFFILE, lexer handles that.
    if(DEBUG)
    {
        print_cur_token_text();
        print_upcoming_token_text();
    }
}

void parse_init()
{
    //to init cur_token and upcoming_token
    get_token();

    cur_token = upcoming_token;
    upcoming_token = token;

    get_token();

    cur_token = upcoming_token;
    upcoming_token = token;

    if(DEBUG)
    {
        print_cur_token_text();
        print_upcoming_token_text();
    }

    //initialize types table
    strcpy(types[0].text, "int");
    types[0].catg = INT_CATG;
    types[0].sizebyte = 4;

    strcpy(types[1].text, "char");
    types[1].catg = CHAR_CATG;
    types[1].sizebyte = 1;
}

int check_cur_token(int kind)
{
    if(kind == cur_token.kind)
        return 1;
    else
        return 0;
}

int check_upcoming_token(int kind)
{
    if(kind == upcoming_token.kind)
        return 1;
    else
        return 0;
}

void print_all_token()
{
    // Parse all the statements in the program.
    while (!check_cur_token(ENDOFFILE))
    {
        print_cur_token_text();
        consume_cur_token();
    }
}

// Production rules.
// each production-function consumes all tokens of that production

//program ::= {declaration} {procedure}
void program()
{
    //Set up things.
    ir_emit_code_line("=CALL addr");
    put_opcode(CALL); //this opcode goes to addr=0
    //keep text section addr to be fixed
    //with actual address of main function
    int fix_addr = get_text_addr_here();
    put_operand(12); //this operand goes to addr=4
                     //it tells to call main() at addr=12
    ir_emit_code_line("=HLT");
    put_opcode(HLT); //this opcode goes to addr=8
    //main() starts at addr=12
    //CALL deposits 8 as return addr where HLT is there
    //execution starts at pc=0

    // Since some newlines are required in our grammar,
    // need to skip the excess.
    while (check_cur_token(NEWLINE)) {
        consume_cur_token(); // consumes NEWLINE
    }

    //Parse all the declarations and functions
    //in the program.
    while (!check_cur_token(ENDOFFILE))
    {
        if(check_cur_token(TYPE)
        || check_cur_token(REF)
        || check_cur_token(FUNC)
        || check_cur_token(MUST)
        || check_cur_token(INT)
        || check_cur_token(CHAR)
        || (check_cur_token(IDENT) && types_contains(cur_token.text)) )
        {
            declaration(); // consumes own tokens
        }
        else if(check_cur_token(FDEF))
        {
            procedure(); // consumes own tokens
        }
        else
        {
            printf("%d: a program has declations and procedures, nothing else: %s\n", line_number, cur_token.text);
            exit(1);
        }
    }

    //fix main function address
    if(!funcs_contains("main"))
    {
        printf("%d: main function mandatory: %s\n", line_number, cur_token.text);
        exit(1);
    }
    int func_index = get_func_index("main");
    fix_operand(fix_addr, funcs[func_index].addr);

    //fix addresses for mutual recusion
    for(int i=0; i<num_fixes; i++)
    {
        int fix_addr = fixfaddr[i].fix_addr;
        int fi = fixfaddr[i].func_index;
        int func_addr = funcs[fi].addr;

        if(func_addr == INVALID_ADDR)
        {
            printf("%d: calling undefined function: %s\n", line_number, cur_token.text);
            exit(1);
        }
        else
        {
            fix_operand(fix_addr, func_addr);
        }
    }

    printf("compilation completed!\n");
}

//primtype ::= "int" | "char"
//primvardecl ::= primtype varname nl
void primvardecl(int fi)
{
    if(check_cur_token(INT)
    || check_cur_token(CHAR))
    {
        ir_emit_code_line(cur_token.text);

        int local_ti;

        //get type index
        local_ti = get_type_index(cur_token.text);

        consume_cur_token(); // consumes type

        ir_emit_code_line(cur_token.text);

        //check variable name validity
        if(types_contains(cur_token.text))
        {
            printf("%d: Variable name can't be same as datatype: %s\n", line_number, cur_token.text);
            exit(1);
        }
        //locals can be same as globals
        //params and locals hide globals.
        if(params_contains(fi, cur_token.text))
        {
            printf("%d: Variable name can't be same as params: %s\n", line_number, cur_token.text);
            exit(1);
        }

        // If variable doesn't exist, declare it.
        if (!locals_contains(fi, cur_token.text))
        {
            locals_add(fi, cur_token.text, local_ti);
        }
        else
        {
            printf("%d: Multiple variable declaration: %s\n", line_number, cur_token.text);
            exit(1);
        }

        make_mandatory(IDENT); // consumes IDENT

        nl(); // consumes NEWLINE
    }
}

/*
statement ::= "print" (chararr_accessor_lval | accessor | strlit ) nl
            | "input" accessor_lval nl
            | accessor_lval "=" expression nl
            | "poly" must_accessor_lval "=" rec_accessor_lval nl
            | "bind" ptr_accessor_lval "=" ("new" typename | accessor_lval) nl
            | "move" ptr_accessor_lval "=" ptr_accessor_lval nl
            | "free" ptr_accessor_lval nl
            | "if" (comparison) "then" nl
                {statement}
              "else" nl
                {statement}
              "endif" nl
            | "while" (comparison) "repeat" nl
                {statement}
              "endwhile" nl
*/
void statement(int fi)
{
    // Check the first token to see what kind of statement this is.

    // "print" (chararr_accessor_lval | accessor | strlit ) nl
    if (check_cur_token(PRINT))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes PRINT

        if (check_cur_token(STRLIT))
        {
            ir_emit_code_line(cur_token.text);

            // If string doesn't already exist,
            //add it to symbols, like a declaration.
            if (!symbols_contains(cur_token.text))
            {
                symbols_add(cur_token, -1); //for string literals type_index is not used
            }
            else
            {
                //no worries, multiple appearance of string
                //is not a multiple declation case.
            }

            // Simple string, so print it.
            ir_emit_code_line("=IMM addr");
            ir_emit_code_line("=OUTSTR");
            int sym_addr = get_addr_of_symbol(cur_token);
            put_opcode(IMM);
            put_operand(sym_addr);
            put_opcode(OUTSTR);

            consume_cur_token(); // consumes STRLIT
        }
        else
        {
            int ti = accessor_lval(fi); // consumes own tokens

            if(ti == 0) //int
            {
                //get the int value
                ir_emit_code_line("=LI");
                put_opcode(LI);

                //now print
                ir_emit_code_line("=OUTINT");
                put_opcode(OUTINT);
            }
            else if(ti == 1) //char
            {
                //get the char value
                ir_emit_code_line("=LC");
                put_opcode(LC);

                //now print
                ir_emit_code_line("=OUTCHAR");
                put_opcode(OUTCHAR);
            }
            else if(types[ti].catg == ARR_CATG
                 && (get_type_index(types[ti].arr_mem_type)==1) ) //char array
            {
                ir_emit_code_line("=OUTSTR");
                put_opcode(OUTSTR);
            }
            else
            {
                printf("%d: Only strlit, int var, char var and char array allowed: %s\n", line_number, cur_token.text);
                exit(1);
            }
        }

        nl(); // consumes NEWLINE
    }
    // "input" accessor_lval nl
    else if (check_cur_token(INPUT))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes INPUT

        int ti;

        if (check_cur_token(IDENT))
        {
            ti = accessor_lval(fi); // consumes own tokens

            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            if(ti == 0) //int
            {
                ir_emit_code_line("=ININT");
                ir_emit_code_line("=SI");
                put_opcode(ININT);
                put_opcode(SI);
            }
            else if (ti == 1) //char
            {
                ir_emit_code_line("=INCHAR");
                ir_emit_code_line("=SC");
                put_opcode(INCHAR);
                put_opcode(SC);
            }
            else
            {
                printf("%d: Expected int or char var: %s\n", line_number, cur_token.text);
                exit(1);
            }
        }
        else
        {
            printf("%d: Expected a variable: %s\n", line_number, cur_token.text);
            exit(1);
        }

        nl(); // consumes NEWLINE
    }
    // accessor_lval "=" expression nl
    else if (check_cur_token(IDENT))
    {
        int ti_1, ti_2;

        ti_1 = accessor_lval(fi); //consumes own tokens

        if( !(ti_1 == 0 || ti_1 == 1) ) //neither int nor char
        {
            printf("%d: Expected int or char variable on LHS: %s\n", line_number, cur_token.text);
            exit(1);
        }

        ir_emit_code_line(cur_token.text);
        make_mandatory(EQUAL); // consumes EQUAL

        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        ti_2 = expression(fi); // consumes own tokens

        if( !(ti_2 == 0 || ti_2 == 1) ) //neither int nor char
        {
            printf("%d: Expected int or char expression on RHS: %s\n", line_number, cur_token.text);
            exit(1);
        }

        if(ti_1==0) //int
        {
            ir_emit_code_line("=SI");
            put_opcode(SI);
        }
        if(ti_1==1) //char
        {
            ir_emit_code_line("=SC");
            put_opcode(SC);
        }

        nl(); // consumes NEWLINE
    }
    // "poly" must_accessor_lval "=" rec_accessor_lval nl
    else if (check_cur_token(POLY))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes POLY

        int ti_1, ti_2;

        ti_1 = must_accessor_lval(fi); //consumes own tokens

        ir_emit_code_line(cur_token.text);
        make_mandatory(EQUAL); // consumes EQUAL

        //to store func addr
        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);
        //used in second go

        //to store obj addr
        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);
        //used in first go

        //here is a trick.
        //we want to store two things
        //in must codetype variable.
        //since it has size of two ints,
        //in first int we store obj's addr
        //in second int we store
        //must func addr of the obj's class
        //for that reason we pushed the addr twice

        ti_2 = rec_accessor_lval(fi); // consumes own tokens

        //create anonymous func name
        //from the given must func name
        struct_token mangled_mfunc;
        int recnamelen = strlen(types[ti_2].text);
        int mfuncnamelen = strlen(types[ti_1].func_type);
        if(recnamelen + mfuncnamelen > MAX_TOK_LEN_USABLE)
        {
            printf("%d: Identifier too big!\n", line_number);
            exit(1);
        }
        strcpy(mangled_mfunc.text, types[ti_2].text);
        strcat(mangled_mfunc.text, " ");
        strcat(mangled_mfunc.text, types[ti_1].func_type);
        //in anonymous names, we used a space char
        //this is because, user can't create such name
        //and there will be no chance of accidental match
        //and we don't need to check for match as well.

        //check if record is compatible
        if(!funcs_contains(mangled_mfunc.text))
        {
            printf("%d: record has no such must func: %s\n", line_number, cur_token.text);
            exit(1);
        }
        int func_index = get_func_index(mangled_mfunc.text);

        //store obj addr loaded by rec_accessor_lval
        ir_emit_code_line("=SI");
        put_opcode(SI);
        //SI pops

        //now we have to store func addr
        //recall that variable addr
        //is already pushed in stack
        //so add 4 to it and push it again
        //then load func addr and store

        //func addr is stored in next int
        //so load an offset value of 4
        int offset_value = 4;
        ir_emit_code_line("=IMM num");
        put_opcode(IMM);
        put_operand(offset_value);

        //now add the offset to base address of var
        ir_emit_code_line("=ADD");
        put_opcode(ADD);
        //ADD pops

        //since ADD pops,
        //push the resultant 4 added addr
        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        //load func addr
        int func_addr = funcs[func_index].addr;
        ir_emit_code_line("=IMM num");
        put_opcode(IMM);
        put_operand(func_addr);

        //store func addr now
        ir_emit_code_line("=SI");
        put_opcode(SI);
        //SI pops

        nl(); // consumes NEWLINE
    }
    // "bind" ptr_accessor_lval "=" ("new" typename | accessor_lval) nl
    else if (check_cur_token(BIND))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes BIND

        int ti_1 = ptr_accessor_lval(fi); //consumes own tokens

        ir_emit_code_line(cur_token.text);
        make_mandatory(EQUAL); // consumes EQUAL

        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        int ti_2;

        if (check_cur_token(NEW))
        {
            ir_emit_code_line(cur_token.text);
            consume_cur_token(); // consumes NEW

            ir_emit_code_line(cur_token.text);

            // Ensure the type already exists.
            if (!types_contains(cur_token.text))
            {
                printf("%d: Unknown type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            ti_2 = get_type_index(cur_token.text);

            if(ti_1 != ti_2)
            {
                printf("%d: Expected pointer val type same as new type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //here you get address from new in AX and do SI.

            //if doesn't exist, create a temp var.
            //to keep record size and pass it to
            //heap_new function which allocates memory.
            struct_token temp_token;
            strcpy(temp_token.text, "temp var");
            temp_token.kind = IDENT;
            if (!symbols_contains(temp_token.text))
            {
                symbols_add(temp_token, 0); //0=int type
            }

            //load temp var address
            int sym_addr = get_addr_of_symbol(temp_token);
            ir_emit_code_line("=IMM addr");
            put_opcode(IMM);
            put_operand(sym_addr);

            //push address of the temp var
            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            //get record size
            int rec_size = types[ti_2].sizebyte;

            //load record size
            ir_emit_code_line("=IMM num");
            put_opcode(IMM);
            put_operand(rec_size);

            //save record size to temp var
            ir_emit_code_line("=SI");
            put_opcode(SI);

            int callee_fi; //func index
            if(!funcs_contains("heap_new"))
            {
                printf("%d: memory manager functions not available. supply memory manager .gwd file: %s\n", line_number, cur_token.text);
                exit(1);
            }
            callee_fi = get_func_index("heap_new");
            int func_addr = funcs[callee_fi].addr;
            int params_sizebyte = funcs[callee_fi].func_param_cnt * 4;

            //load temp var address
            ir_emit_code_line("=IMM addr");
            put_opcode(IMM);
            put_operand(sym_addr);

            //push collected address of the arg
            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            //call the function
            ir_emit_code_line("=CALL addr");
            put_opcode(CALL);
            put_operand(func_addr);

            //clean pushed args from stack
            ir_emit_code_line("=ADJ size");
            put_opcode(ADJ);
            put_operand(params_sizebyte);

            //save returned address
            //of newly allocated memory
            ir_emit_code_line("=SI");
            put_opcode(SI);

            consume_cur_token(); // consumes typename
        }
        else if (check_cur_token(IDENT)) //this is for testing purpose when new is not there
        {
            ti_2 = accessor_lval(fi); //consumes own tokens

            if(ti_1 != ti_2)
            {
                printf("%d: Expected pointer val type same as var type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //here we have address of given var
            //loaded into AX, so just do SI.
            ir_emit_code_line("=SI");
            put_opcode(SI);
        }
        else
        {
            printf("%d: Expecting new typename or varname: %s\n", line_number, cur_token.text);
            exit(1);
        }

        nl(); // consumes NEWLINE
    }
    // "move" ptr_accessor_lval "=" ptr_accessor_lval nl
    else if (check_cur_token(MOVE))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes MOVE

        int ti_1 = ptr_accessor_lval(fi); //consumes own tokens

        ir_emit_code_line(cur_token.text);
        make_mandatory(EQUAL); // consumes EQUAL

        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        int ti_2 = ptr_accessor_lval(fi); //consumes own tokens

        if(ti_1 != ti_2)
        {
            printf("%d: Expected pointer val type same for both: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //load object addr from second ptr var
        ir_emit_code_line("=LI");
        put_opcode(LI);

        //put object addr into first ptr var
        ir_emit_code_line("=SI");
        put_opcode(SI);

        nl(); // consumes NEWLINE
    }
    // "free" ptr_accessor_lval nl
    else if (check_cur_token(FREE))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes FREE

        int callee_fi; //func index
        if(!funcs_contains("heap_free"))
        {
            printf("%d: memory manager functions not available. supply memory manager .gwd file: %s\n", line_number, cur_token.text);
            exit(1);
        }
        callee_fi = get_func_index("heap_free");
        int func_addr = funcs[callee_fi].addr;
        int params_sizebyte = funcs[callee_fi].func_param_cnt * 4;

        int ti = ptr_accessor_lval(fi); //consumes own tokens

        //push collected address of the pointer
        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        //call the function
        ir_emit_code_line("=CALL addr");
        put_opcode(CALL);
        put_operand(func_addr);

        //clean pushed args from stack
        ir_emit_code_line("=ADJ size");
        put_opcode(ADJ);
        put_operand(params_sizebyte);

        //only in case of free statement
        //we don't save the returned value

        nl(); // consumes NEWLINE
    }
    /*
    "if" (logical_expression) "then" nl
      {statement}
    "else" nl
      {statement}
    "endif" nl
    */
    else if (check_cur_token(IF))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes IF

        logical_expression(fi); // consumes own tokens

        ir_emit_code_line(cur_token.text);
        make_mandatory(THEN); // consumes THEN
        nl(); // consumes NEWLINE

        ir_emit_code_line("=JZ addr1");
        put_opcode(JZ);
        int fix_addr1 = get_text_addr_here();
        put_operand(INVALID_ADDR); //fix later

        // Zero or more statements in the if body.
        while (!check_cur_token(ELSE)) {
            statement(fi); // consumes own tokens
        }

        ir_emit_code_line(cur_token.text);
        make_mandatory(ELSE); // consumes ELSE
        nl(); // consumes NEWLINE

        ir_emit_code_line("=JMP addr2");
        put_opcode(JMP);
        int fix_addr2 = get_text_addr_here();
        put_operand(INVALID_ADDR); //fix later

        ir_emit_code_line("=HERE addr1");
        int text_addr1 = get_text_addr_here();
        fix_operand(fix_addr1, text_addr1);

        // Zero or more statements in the else body.
        while (!check_cur_token(ENDIF)) {
            statement(fi); // consumes own tokens
        }

        ir_emit_code_line(cur_token.text);
        make_mandatory(ENDIF); // consumes ENDIF

        ir_emit_code_line("=HERE addr2");
        int text_addr2 = get_text_addr_here();
        fix_operand(fix_addr2, text_addr2);

        nl(); // consumes NEWLINE
    }
    /*
    "while" (logical_expression) "repeat" nl
      {statement}
    "endwhile" nl
    */
    else if (check_cur_token(WHILE))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes WHILE

        ir_emit_code_line("=HERE addr3");
        int text_addr3 = get_text_addr_here();

        logical_expression(fi); // consumes own tokens

        ir_emit_code_line(cur_token.text);
        make_mandatory(REPEAT); // consumes REPEAT
        nl(); // consumes NEWLINE

        ir_emit_code_line("=JZ addr4");
        put_opcode(JZ);
        int fix_addr4 = get_text_addr_here();
        put_operand(INVALID_ADDR); //fix later

        // Zero or more statements in the loop body.
        while (!check_cur_token(ENDWHILE)) {
            statement(fi); // consumes own tokens
        }

        ir_emit_code_line(cur_token.text);
        make_mandatory(ENDWHILE); // consumes ENDWHILE

        ir_emit_code_line("=JMP addr3");
        put_opcode(JMP);
        put_operand(text_addr3);

        ir_emit_code_line("=HERE addr4");
        int text_addr4 = get_text_addr_here();
        fix_operand(fix_addr4, text_addr4);

        nl(); // consumes NEWLINE
    }
    // This is not a valid statement. Error!
    else
    {
        printf("%d: Invalid statement at ", line_number);
        printf("%s", cur_token.text);
        printf(" (");
        printf("%s", get_kind_string(cur_token.kind) );
        printf(")\n");
        exit(1);
    }
}

// comparison ::= expression (eq | neq | gt | ge | lt | le)
void comparison(int fi)
{
    int ti_1, ti_2;

    ti_1 = expression(fi); // consumes own tokens

    // Now we can have 0 or more comparison operator and expressions.
    if(check_cur_token(EQEQ))
        ti_2 = eq(fi); // consumes own tokens
    else if(check_cur_token(NOTEQ))
        ti_2 = neq(fi); // consumes own tokens
    else if(check_cur_token(GRET))
        ti_2 = gt(fi); // consumes own tokens
    else if(check_cur_token(GTEQ))
        ti_2 = ge(fi); // consumes own tokens
    else if(check_cur_token(LESS))
        ti_2 = lt(fi); // consumes own tokens
    else if(check_cur_token(LTEQ))
        ti_2 = le(fi); // consumes own tokens
    else
    {
        printf("%d: Expects relational operator: %s\n", line_number, cur_token.text);
        exit(1);
    }

    if(ti_1 != ti_2)
    {
        printf("%d: Expects operands of similar types: %s\n", line_number, cur_token.text);
        exit(1);
    }
}

// eq ::= "==" expression
int eq(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(EQEQ); // consumes EQEQ

    ti = expression(fi); // consumes own tokens

    ir_emit_code_line("=EQ");
    put_opcode(EQ);

    return ti;
}

// neq ::= "!=" expression
int neq(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(NOTEQ); // consumes NOTEQ

    ti = expression(fi); // consumes own tokens

    ir_emit_code_line("=NE");
    put_opcode(NE);

    return ti;
}

// gt ::= ">" expression
int gt(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(GRET); // consumes GRET

    ti = expression(fi); // consumes own tokens

    ir_emit_code_line("=GT");
    put_opcode(GT);

    return ti;
}

// ge ::= ">=" expression
int ge(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(GTEQ); // consumes GTEQ

    ti = expression(fi); // consumes own tokens

    ir_emit_code_line("=GE");
    put_opcode(GE);

    return ti;
}

// lt ::= "<" expression
int lt(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(LESS); // consumes LESS

    ti = expression(fi); // consumes own tokens

    ir_emit_code_line("=LT");
    put_opcode(LT);

    return ti;
}

// le ::= "<=" expression
int le(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(LTEQ); // consumes LTEQ

    ti = expression(fi); // consumes own tokens

    ir_emit_code_line("=LE");
    put_opcode(LE);

    return ti;
}

// expression ::= term {add | sub}
int expression(int fi)
{
    int ti_1, ti_2;

    ti_1 = term(fi); // consumes own tokens

    // Can have 0 or more +/- and terms.
    while (check_cur_token(PLUS) || check_cur_token(MINUS))
    {
        if(check_cur_token(PLUS))
            ti_2 = add(fi); // consumes own tokens
        else if(check_cur_token(MINUS))
            ti_2 = sub(fi); // consumes own tokens

        if(ti_1 != 0 || ti_2 != 0)
        {
            printf("%d: Expects int operands: %s\n", line_number, cur_token.text);
            exit(1);
        }
    }

    return ti_1;
}

// add ::= "+" term
int add(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(PLUS); // consumes PLUS

    ti = term(fi); // consumes own tokens

    ir_emit_code_line("=ADD");
    put_opcode(ADD);

    return ti;
}

// sub ::= "-" term
int sub(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(MINUS); // consumes MINUS

    ti = term(fi); // consumes own tokens

    ir_emit_code_line("=SUB");
    put_opcode(SUB);

    return ti;
}

// term ::= unary {mul | div}
int term(int fi)
{
    int ti_1, ti_2;

    ti_1 = unary(fi); // consumes own tokens

    // Can have 0 or more *// and unarys.
    while (check_cur_token(ASTERISK) || check_cur_token(SLASH))
    {
        if(check_cur_token(ASTERISK))
            ti_2 = mul_term(fi); // consumes own tokens
        else if(check_cur_token(SLASH))
            ti_2 = div_term(fi); // consumes own tokens

        if(ti_1 != 0 || ti_2 != 0)
        {
            printf("%d: Expects int operands: %s\n", line_number, cur_token.text);
            exit(1);
        }
    }

    return ti_1;
}

// mul ::= "*" unary
int mul_term(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(ASTERISK); // consumes ASTERISK

    ti = unary(fi); // consumes own tokens

    ir_emit_code_line("=MUL");
    put_opcode(MUL);

    return ti;
}

// div ::= "/" unary
int div_term(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(SLASH); // consumes SLASH

    ti = unary(fi); // consumes own tokens

    ir_emit_code_line("=DIV");
    put_opcode(DIV);

    return ti;
}

// unary ::= plus | minus | primary
int unary(int fi)
{
    int ti;

    // Optional unary +/-
    if (check_cur_token(PLUS))
        ti = plus(fi); // consumes own tokens
    else if (check_cur_token(MINUS))
        ti = minus(fi); // consumes own tokens
    else
        ti = primary(fi); // consumes own tokens

    return ti;
}

// plus ::= "+" primary
int plus(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=IMM 0");
    ir_emit_code_line("=PUSH");
    put_opcode(IMM);
    put_operand(0);
    put_opcode(PUSH);

    make_mandatory(PLUS); // consumes PLUS

    ti = primary(fi); // consumes own tokens
    if(ti != 0)
    {
        printf("%d: Expects int operand: %s\n", line_number, cur_token.text);
        exit(1);
    }

    ir_emit_code_line("=ADD");
    put_opcode(ADD);

    return ti;
}

// minus ::= "-" primary
int minus(int fi)
{
    int ti;

    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=IMM 0");
    ir_emit_code_line("=PUSH");
    put_opcode(IMM);
    put_operand(0);
    put_opcode(PUSH);

    make_mandatory(MINUS); // consumes MINUS

    ti = primary(fi); // consumes own tokens
    if(ti != 0)
    {
        printf("%d: Expects int operand: %s\n", line_number, cur_token.text);
        exit(1);
    }

    ir_emit_code_line("=SUB");
    put_opcode(SUB);

    return ti;
}

// logical_expression ::= logical_term {or}
void logical_expression(int fi)
{
    logical_term(fi); // consumes own tokens

    // Can have 0 or more '|' and terms.
    while (check_cur_token(PIPE))
    {
		logical_or(fi); // consumes own tokens
    }
}

// or ::= "|" logical_term
void logical_or(int fi)
{
    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(PIPE); // consumes PIPE

    logical_term(fi); // consumes own tokens

    ir_emit_code_line("=OR");
    put_opcode(OR);
}

// logical_term ::= logical_unary {and}
void logical_term(int fi)
{
	logical_unary(fi); // consumes own tokens

    // Can have 0 or more '&' and unarys.
    while (check_cur_token(AMPERSAND))
    {
		logical_and(fi); // consumes own tokens
    }
}

// and ::= "&" logical_unary
void logical_and(int fi)
{
    ir_emit_code_line(cur_token.text);
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    make_mandatory(AMPERSAND); // consumes AMPERSAND

    logical_unary(fi); // consumes own tokens

    ir_emit_code_line("=AND");
    put_opcode(AND);
}

// logical_unary ::= not | logical_primary
void logical_unary(int fi)
{
    // Optional unary '~'
    if (check_cur_token(TILDE))
        logical_not(fi); // consumes own tokens
    else
        logical_primary(fi); // consumes own tokens
}

// not ::= "~" logical_primary
void logical_not(int fi)
{
    ir_emit_code_line(cur_token.text);
    make_mandatory(TILDE); // consumes TILDE

    logical_primary(fi); // consumes own tokens

    ir_emit_code_line("=PUSH");
    ir_emit_code_line("=IMM 0");
    ir_emit_code_line("=EQ");
    put_opcode(PUSH);
    put_opcode(IMM);
    put_operand(0);
    put_opcode(EQ);
}

// logical_primary ::= comparison | "[" logical_expression "]"
void logical_primary(int fi)
{
    if (!check_cur_token(LBRKT))
    {
		comparison(fi); // consumes own tokens
	}
    else if (check_cur_token(LBRKT))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes LBRKT

        logical_expression(fi); // consumes own tokens

        ir_emit_code_line(cur_token.text);
        make_mandatory(RBRKT); // consumes RBRKT
    }
}

// nl ::= "\n" {"\n"}
void nl()
{
    // Require at least one newline.
    make_mandatory(NEWLINE); // consumes NEWLINE
    // But we will allow extra newlines too, of course.
    while (check_cur_token(NEWLINE))
    {
        consume_cur_token(); // consumes NEWLINE
    }
}
