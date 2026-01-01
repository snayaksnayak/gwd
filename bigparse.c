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

extern struct_token cur_token;
extern struct_token upcoming_token;

//"datatype" dec/def
extern struct_type types[MAX_TYPES];
extern int num_types; //=2

//"variable" dec/def
extern struct_symbol symbols[MAX_SYMBOLS];
// All variables we have declared so far.
// and strings that we have used so far.
extern int num_symbols; //=0

//"prototype" dec and
//"function" def both
//for general funcs
extern struct_func funcs[MAX_FUNCS];
extern int num_funcs; //=0

//"prototype" dec only
//for must funcs
    //"function" def
    //for must funcs
    //are inside classes
extern struct_func musts[MAX_MUSTS];
extern int num_musts; //=0

extern struct_fix fixfaddr[MAX_FIXES];
extern int num_fixes; //=0

/*
procedure ::= "fdef" funcname "(" {(primtype | typename) varname ","} ")" nl
              "{" nl {primvardecl} {statement} "return" expression nl "}" nl

*/
void procedure()
{
    if (check_cur_token(FDEF))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes FDEF

        ir_emit_code_line(cur_token.text);

        //collect the address of this function
        int func_addr = get_text_addr_here();
        int locals_sizebyte = 0; //compute later

        //function entry
        ir_emit_code_line("=ENT size");
        put_opcode(ENT);
        int fix_addr1 = get_text_addr_here();
        put_operand(locals_sizebyte); //fix later

        //check function name declaration
        if(!funcs_contains(cur_token.text))
        {
            printf("%d: function declaration mandatory: %s\n", line_number, cur_token.text);
            exit(1);
        }

        int func_index = get_func_index(cur_token.text);

        //save/update the address of this function
        funcs[func_index].addr = func_addr;

        make_mandatory(IDENT); // consumes IDENT

        ir_emit_code_line(cur_token.text);
        make_mandatory(LPAREN); // consumes LPAREN

        int i=0; //for param type, name, count match
        while(!check_cur_token(RPAREN))
        {
            ir_emit_code_line(cur_token.text);

            //match param type
            if(0 != strcmp(cur_token.text,
                    funcs[func_index].params[i].type) )
            {
                printf("%d: param type mismatch: %s\n", line_number, cur_token.text);
                exit(1);
            }

            consume_cur_token(); // consumes type

            ir_emit_code_line(cur_token.text);

            //match param name
            if(0 != strcmp( cur_token.text,
                    funcs[func_index].params[i].text) )
            {
                printf("%d: param name mismatch: %s\n", line_number, cur_token.text);
                exit(1);
            }

            consume_cur_token(); // consumes name

            //comma
            //four cases possible
            //1. func f(int a) //comma optional
            //2. func f(int a,) //consume comma
            //3. func f(int a, int b) //consume comma
            //4. func f(int a int b) //error
            if(check_cur_token(COMMA)
            || check_cur_token(RPAREN))
            {
                if(check_cur_token(COMMA)) //case 2 & 3
                {
                    ir_emit_code_line(cur_token.text);
                    consume_cur_token(); //consumes comma
                }
                //else fine, case 1
            }
            else //case 4
            {
                printf("%d: comma expected: %s\n", line_number, cur_token.text);
                exit(1);
            }

            i++;
        }

        ir_emit_code_line(cur_token.text);

        //now match param count
        if(funcs[func_index].func_param_cnt != i)
        {
            printf("%d: param count mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        make_mandatory(RPAREN); // consumes RPAREN

        nl(); // consumes NEWLINE

        //function body
        ir_emit_code_line(cur_token.text);
        make_mandatory(LBRACE); // consumes LBRACE

        nl(); // consumes NEWLINE

        //Parse all the declarations and statements
        //in the function body.
        while (!check_cur_token(RETURN))
        {
            if(check_cur_token(INT)
            || check_cur_token(CHAR))
            {
                primvardecl(func_index); // consumes own tokens
            }
            else
            {
                statement(func_index); // consumes own tokens
            }
        }

        ir_emit_code_line(cur_token.text);
        make_mandatory(RETURN); // consumes RETURN

        int ti = expression(func_index); //consumes own tokens
        if(ti != 0 && ti != 1) //if not (int or char)
        {
            printf("%d: func return can only be int or char: %s\n", line_number, cur_token.text);
            exit(1);
        }

        nl(); // consumes NEWLINE

        //calculate locals_sizebyte and fix ENT
        locals_sizebyte = funcs[func_index].func_local_cnt * 4;
        fix_operand(fix_addr1, locals_sizebyte);

        ir_emit_code_line(cur_token.text);
        make_mandatory(RBRACE); // consumes RBRACE

        ir_emit_code_line("=LEV");
        put_opcode(LEV);

        nl(); // consumes NEWLINE
    }
}

// fcall ::= funcname "(" {varname ","} ")"
void fcall(int fi)
{
    int callee_fi; //func index
    int ti; //type index

    ir_emit_code_line(cur_token.text);

    callee_fi = get_func_index(cur_token.text);
    int func_addr = funcs[callee_fi].addr;
    int params_sizebyte = funcs[callee_fi].func_param_cnt * 4;

    consume_cur_token(); // consumes func name

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    int i=0; //for param type, count match
    while(!check_cur_token(RPAREN))
    {
        //we don't support expression as arg
        //because we support pass by reference
        ti = accessor_lval(fi); //consumes own tokens

        //match arg type with param type
        if(0 != strcmp(types[ti].text,
                funcs[callee_fi].params[i].type) )
        {
            printf("%d: param type mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //push collected address of the arg
        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }

        i++;
    }

    ir_emit_code_line(cur_token.text);

    //now match arg and param count
    if(funcs[callee_fi].func_param_cnt != i)
    {
        printf("%d: arg/param count mismatch: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(RPAREN); // consumes RPAREN

    //call the function
    ir_emit_code_line("=CALL addr");
    put_opcode(CALL);
    //keep text section addr to be fixed for mutual recursion
    int fix_addr = get_text_addr_here();
    put_operand(func_addr); //fix later if func_addr is invalid, else no worries

    //if function is not defined yet, in cases of
    //mutual recursion, A calling B, B calling A
    if(func_addr == INVALID_ADDR)
    {
        num_fixes++;
        if(num_fixes > MAX_FIXES)
        {
            printf("%d: faddr fix table overflow: %s\n", line_number, cur_token.text);
            exit(1);
        }
        fixfaddr[num_fixes-1].fix_addr = fix_addr;
        fixfaddr[num_fixes-1].func_index = callee_fi;
    }

    //clean pushed args from stack
    ir_emit_code_line("=ADJ size");
    put_opcode(ADJ);
    put_operand(params_sizebyte);
}

// mccall ::= funcname "(" {varname ","} ")"
void mccall(int fi, int pti)
{
    int callee_fi; //func index
    int ti; //type index

    ir_emit_code_line(cur_token.text);

    //recreate anonymous func name
    //from the given must/can func name
    struct_token mangled_mcfunc;
    int recnamelen = strlen(types[pti].text);
    int mfuncnamelen = strlen(cur_token.text);
    if(recnamelen + mfuncnamelen > MAX_TOK_LEN_USABLE)
    {
        printf("%d: Identifier too big!\n", line_number);
        exit(1);
    }
    strcpy(mangled_mcfunc.text, types[pti].text);
    strcat(mangled_mcfunc.text, " ");
    strcat(mangled_mcfunc.text, cur_token.text);
    //in anonymous names, we used a space char
    //this is because, user can't create such name
    //and there will be no chance of accidental match
    //and we don't need to check for match as well.

    callee_fi = get_func_index(mangled_mcfunc.text);
    int func_addr = funcs[callee_fi].addr;
    int params_sizebyte = funcs[callee_fi].func_param_cnt * 4;

    consume_cur_token(); // consumes func name

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    //push address of the object, which is self
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    int i=1; //for param type, count match
             //i=0 is for addr of object, as self
    while(!check_cur_token(RPAREN))
    {
        //we don't support expression as arg
        //because we support pass by reference
        ti = accessor_lval(fi); //consumes own tokens

        //match arg type with param type
        if(0 != strcmp(types[ti].text,
                funcs[callee_fi].params[i].type) )
        {
            printf("%d: param type mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //push collected address of the arg
        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }

        i++;
    }

    ir_emit_code_line(cur_token.text);

    //now match arg and param count
    if(funcs[callee_fi].func_param_cnt != i)
    {
        printf("%d: arg/param count mismatch: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(RPAREN); // consumes RPAREN

    //call the function
    ir_emit_code_line("=CALL addr");
    put_opcode(CALL);
    //keep text section addr to be fixed for mutual recursion
    int fix_addr = get_text_addr_here();
    put_operand(func_addr); //fix later if func_addr is invalid, else no worries

    //if function is not defined yet, in cases of
    //mutual recursion, A calling B, B calling A
    if(func_addr == INVALID_ADDR)
    {
        num_fixes++;
        if(num_fixes > MAX_FIXES)
        {
            printf("%d: faddr fix table overflow: %s\n", line_number, cur_token.text);
            exit(1);
        }
        fixfaddr[num_fixes-1].fix_addr = fix_addr;
        fixfaddr[num_fixes-1].func_index = callee_fi;
    }

    //clean pushed args from stack
    ir_emit_code_line("=ADJ size");
    put_opcode(ADJ);
    put_operand(params_sizebyte);
}

// pcall ::= funcname "(" {varname ","} ")"
void pcall(int fi, int pti)
{
    int callee_fi; //func index
    int ti; //type index

    ir_emit_code_line(cur_token.text);

    callee_fi = get_must_index(cur_token.text);
    int params_sizebyte = musts[callee_fi].func_param_cnt * 4;
    //func_addr will be collected from
    //must var address using PUSH/SWAP instruction trick

    consume_cur_token(); // consumes func name

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    //at this moment ax holds addr of must var.

    //push address of the must var,
    //which is needed later to get func addr
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    //get address of the object, which is self
    ir_emit_code_line("=LI");
    put_opcode(LI);

    //push address of the object, which is self, the first param
    ir_emit_code_line("=PUSH");
    put_opcode(PUSH);

    //get address of the must var on top,
    //which is needed later to get func addr
    ir_emit_code_line("=SWAP");
    put_opcode(SWAP);

    int i=1; //for param type, count match
             //i=0 is for addr of object, as self
    while(!check_cur_token(RPAREN))
    {
        //we don't support expression as arg
        //because we support pass by reference
        ti = accessor_lval(fi); //consumes own tokens

        //match arg type with param type
        if(0 != strcmp(types[ti].text,
                musts[callee_fi].params[i].type) )
        {
            printf("%d: param type mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //push collected address of the arg
        ir_emit_code_line("=PUSH");
        put_opcode(PUSH);

        //get address of the must var on top,
        //which is needed later to get func addr
        ir_emit_code_line("=SWAP");
        put_opcode(SWAP);

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }

        i++;
    }

    ir_emit_code_line(cur_token.text);

    //now match arg and param count
    if(musts[callee_fi].func_param_cnt != i)
    {
        printf("%d: arg/param count mismatch: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(RPAREN); // consumes RPAREN

    //now we have to get func addr
    //recall that variable addr
    //is already pushed in stack
    //so add 4 to it and do LI
    //to get func addr. then do ICALL

    //func addr is stored in next int
    //so load an offset value of 4
    int offset_value = 4;
    ir_emit_code_line("=IMM num");
    put_opcode(IMM);
    put_operand(offset_value);

    //now add the offset to base address of var
    ir_emit_code_line("=ADD");
    put_opcode(ADD);
    //ADD pops and base+4 is in ax

    //func addr is in base+4 addr
    //so get func addr in ax
    ir_emit_code_line("=LI");
    put_opcode(LI);

    //call the function
    ir_emit_code_line("=ICALL");
    put_opcode(ICALL);

    //clean pushed args from stack
    ir_emit_code_line("=ADJ size");
    put_opcode(ADJ);
    put_operand(params_sizebyte);
}

/*
declaration ::= "type" typename "==" (ptrtype | comptype | codetype) nl
              | (primtype | typename) varname nl
              | "ref" varname "points" (primtype | comp_typename) nl     #typename!=ptrtype, we have no pointer dereferencing
              | "func" funcname "(" {(primtype | typename) varname ","} ")" nl
              | "must" funcname "(" {(primtype | typename) varname ","} ")" nl
*/
void declaration()
{
    if (check_cur_token(TYPE))
    {
        type_declaration();
    }
    else if ( check_cur_token(INT) || check_cur_token(CHAR)
           || (check_cur_token(IDENT) && types_contains(cur_token.text)) )
    {
        global_var_declaration();
    }
    else if (check_cur_token(REF))
    {
        global_ref_declaration();
    }
    else if (check_cur_token(FUNC))
    {
        function_declaration();
    }
    else if (check_cur_token(MUST))
    {
        global_must_declaration();
    }
}

void type_declaration()
{
    // "type" typename "==" (ptrtype | comptype | codetype) nl

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes TYPE

    ir_emit_code_line(cur_token.text);

    //check type name validity
    if(symbols_contains(cur_token.text))
    {
        printf("%d: type name cant be same as variable name: %s\n", line_number, cur_token.text);
        exit(1);
    }
    if(funcs_contains(cur_token.text))
    {
        printf("%d: type name can't be same as function name: %s\n", line_number, cur_token.text);
        exit(1);
    }
    if(musts_contains(cur_token.text))
    {
        printf("%d: type name can't be same as must function name: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //check if datatype exists
    if (types_contains(cur_token.text))
    {
        printf("%d: Multiple type declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //lets add the new type name
    types_add(cur_token.text);
    int type_index = get_type_index(cur_token.text); //this is type index of just added typename

    make_mandatory(IDENT); // consumes IDENT

    ir_emit_code_line(cur_token.text);
    make_mandatory(EQEQ); // consumes EQEQ

    if(check_cur_token(POINTS))
    {
        pointer_type_declaration(type_index);
    }
    else if(check_cur_token(ARRAY))
    {
        array_type_declaration(type_index);
    }
    else if(check_cur_token(RECORD))
    {
        record_type_declaration(type_index);
    }
    else if(check_cur_token(MUST))
    {
        mustvar_type_declaration(type_index);
    }
    else
    {
        printf("%d: Expected ptr, arr, rec or func type: %s\n", line_number, cur_token.text);
        exit(1);
    }

    nl(); // consumes NEWLINE
}

//type_index = index of the type being created
void pointer_type_declaration(int type_index)
{
    //ptrtype ::= "points" (primtype | comp_typename)     #typename!=ptrtype, we have no pointer dereferencing

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes POINTS

    ir_emit_code_line(cur_token.text);

    types[type_index].catg = PTR_CATG;
    types[type_index].sizebyte = 4;

    //don't check if value datatype exists.
    //directly use it in ptr_val_type.
    //this is to support self/cross reference
    //without adding any forward declaration.
    //but whenever we use a ptr var, we have to
    //make sure that, its value type is exiting,
    //and not of a pointer type again.
    //avoiding above ptr to ptr is to support
    //direct dereferncing of ptr like references,
    //without the need of any "value at" operator.

    strcpy(types[type_index].ptr_val_type, cur_token.text);

    //consume ptr val type
    if(check_cur_token(INT)
    || check_cur_token(CHAR)
    || check_cur_token(IDENT))
    {
        consume_cur_token(); // consumes ptr val type
    }
    else
    {
        printf("%d: Expected datatype, got: %s\n", line_number, cur_token.text);
        exit(1);
    }
}

//type_index = index of the type being created
void array_type_declaration(int type_index)
{
    //len ::= intlit
    //arrtype ::= "array" (primtype | typename) len

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes ARRAY

    ir_emit_code_line(cur_token.text);

    types[type_index].catg = ARR_CATG;

    //check if member datatype exists
    if (!types_contains(cur_token.text))
    {
        printf("%d: Unknown member type: %s\n", line_number, cur_token.text);
        exit(1);
    }

    int ti = get_type_index(cur_token.text); //this is type index of members

    //check if member datatype is same as
    //current datatype being defined
    if (type_index == ti)
    {
        printf("%d: Member type can't be same as parent type: %s\n", line_number, cur_token.text);
        exit(1);
    }

    strcpy(types[type_index].arr_mem_type, types[ti].text);

    //consume member type
    if(check_cur_token(INT)
    || check_cur_token(CHAR)
    || check_cur_token(IDENT))
    {
        consume_cur_token(); // consumes member type
    }
    else
    {
        printf("%d: Expected type name, got: %s\n", line_number, cur_token.text);
        exit(1);
    }

    ir_emit_code_line(cur_token.text);

    if(!check_cur_token(INTLIT))
    {
        printf("%d: Expected integer literal: %s\n", line_number, cur_token.text);
        exit(1);
    }

    int num = convert_text_to_num(cur_token.text);
    if(num == 0)
    {
        printf("%d: Expected non zero length: %s\n", line_number, cur_token.text);
        exit(1);
    }

    types[type_index].arr_mem_cnt = num;
    types[type_index].sizebyte = num * types[ti].sizebyte;

    consume_cur_token(); // consumes array length
}

/*
rectype ::= "record" nl rec_member {rec_member} "endrec"
rec_member ::= (primtype | typename) varname nl
               | "ref" varname "points" (primtype | comp_typename) nl     #typename!=ptrtype, we have no pointer dereferencing
               | "can" funcname "(" {(primtype | typename) varname ","} ")" nl
               | "must" funcname "(" {(primtype | typename) varname ","} ")" nl
               | can_procedure
               | must_procedure

can_procedure ::= "cdef" funcname "(" {(primtype | typename) varname ","} ")" nl
                  "{" nl {primvardecl} {statement} "return" expression nl "}" nl
must_procedure ::= "mdef" funcname "(" {(primtype | typename) varname ","} ")" nl
                   "{" nl {primvardecl} {statement} "return" expression nl "}" nl
*/

//type_index = index of the type being created
void record_type_declaration(int type_index)
{
    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes RECORD

    types[type_index].catg = REC_CATG;

    nl(); // consumes NEWLINE

    while(!check_cur_token(ENDREC))
    {
        if(check_cur_token(REF))
        {
            rec_ref_var_declaration(type_index);
        }
        else if (check_cur_token(MUST))
        {
            rec_must_declaration(type_index);
        }
        else if (check_cur_token(MDEF))
        {
            rec_must_procedure(type_index);
        }
        else if (check_cur_token(CAN))
        {
            rec_can_declaration(type_index);
        }
        else if (check_cur_token(CDEF))
        {
            rec_can_procedure(type_index);
        }
        else //member declaration
        {
            rec_member_var_declaration(type_index);
        }
    }

    ir_emit_code_line(cur_token.text);

    if(types[type_index].rec_mem_cnt == 0)
    {
            printf("%d: Expected members in record: %s\n", line_number, cur_token.text);
            exit(1);
    }

    int last_mem_index = types[type_index].rec_mem_cnt - 1;
    int last_mem_addr = types[type_index].rec_mem_list[last_mem_index].addr;
    int last_mem_ti = get_type_index(types[type_index].rec_mem_list[last_mem_index].type);
    int last_mem_size = types[last_mem_ti].sizebyte;
    int last_mem_size_aligned = get_bytes_req(last_mem_size); //get aligned

    types[type_index].sizebyte = last_mem_addr + last_mem_size_aligned;

    make_mandatory(ENDREC); // consumes ENDREC
}

//type_index = index of the type being created
void rec_ref_var_declaration(int type_index)
{
    //record ref variables
    //"ref" varname "points" (primtype | comp_typename) nl     #typename!=ptrtype, we have no pointer dereferencing

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes REF

    ir_emit_code_line(cur_token.text);

    //check variable name validity
    if(types_contains(cur_token.text))
    {
        printf("%d: Member variable name can't be same as datatype: %s\n", line_number, cur_token.text);
        exit(1);
    }

    // If member variable exists, error.
    if (members_contains(type_index, cur_token.text))
    {
        printf("%d: Multiple member variable declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //save varname for later use
    struct_token var_token;
    var_token = cur_token;

    make_mandatory(IDENT); // consumes varname

    ir_emit_code_line(cur_token.text);
    make_mandatory(POINTS); // consumes POINTS

    ir_emit_code_line(cur_token.text);

    //create an anonymous ptr type
    //from the given val type
    struct_token typ_token;
    typ_token = cur_token;
    strcat(typ_token.text, " ptr"); //we have 8 char space, we are safe.
    //in anonymous type name, we used a space char
    //this is because, user can't create such type name
    //and there will be no chance of accidental match
    //and we don't need to check for match as well.

    //lets add the new type name
    types_add(typ_token.text);
    int ti = get_type_index(typ_token.text); //this is type index of just added typename
    types[ti].catg = PTR_CATG;
    types[ti].sizebyte = 4;
    //add ptr val type now
    strcpy(types[ti].ptr_val_type, cur_token.text);

    //consume ptr val type
    if(check_cur_token(INT)
    || check_cur_token(CHAR)
    || check_cur_token(IDENT))
    {
        consume_cur_token(); // consumes ptr val type
    }
    else
    {
        printf("%d: Expected datatype, got: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //now add the saved symbol
    members_add(type_index, var_token.text, ti);

    nl(); // consumes NEWLINE
}

//type_index = index of the type being created
void rec_must_declaration(int type_index)
{
    // inside record
    // "must" funcname "(" {(primtype | typename) varname ","} ")"

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes MUST

    ir_emit_code_line(cur_token.text);

    //check if must function name declared globally
    if(!musts_contains(cur_token.text))
    {
        printf("%d: must function not declared globally: %s\n", line_number, cur_token.text);
        exit(1);
    }
    int mfunc_index = get_must_index(cur_token.text);

    //create anonymous func name
    //from the given must func name
    struct_token mangled_mfunc;
    int recnamelen = strlen(types[type_index].text);
    int mfuncnamelen = strlen(cur_token.text);
    if(recnamelen + mfuncnamelen > MAX_TOK_LEN_USABLE)
    {
        printf("%d: Identifier too big!\n", line_number);
        exit(1);
    }
    strcpy(mangled_mfunc.text, types[type_index].text);
    strcat(mangled_mfunc.text, " ");
    strcat(mangled_mfunc.text, cur_token.text);
    //in anonymous names, we used a space char
    //this is because, user can't create such name
    //and there will be no chance of accidental match
    //and we don't need to check for match as well.

    //check if multiple must function declaration
    if(funcs_contains(mangled_mfunc.text))
    {
        printf("%d: multiple must function declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //lets add the new function name
    funcs_add(mangled_mfunc.text);
    int func_index = get_func_index(mangled_mfunc.text);
    //this is func index of just added function

    make_mandatory(IDENT); // consumes IDENT

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    //add "self"
    params_add(func_index, "self", type_index); //self is actually of current type_index type

    int i=1; //for param type, name, count match
             //i=0 is for "self"
    while(!check_cur_token(RPAREN))
    {
        ir_emit_code_line(cur_token.text);

        //match param type
        if(0 != strcmp(cur_token.text,
                musts[mfunc_index].params[i].type) )
        {
            printf("%d: param type mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //get param type index
        int param_ti = get_type_index(cur_token.text);
        //this is type index of params

        consume_cur_token(); // consumes type

        ir_emit_code_line(cur_token.text);

        //match param name
        if(0 != strcmp( cur_token.text,
                musts[mfunc_index].params[i].text) )
        {
            printf("%d: param name mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        params_add(func_index, cur_token.text, param_ti);

        consume_cur_token(); // consumes name

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }

        i++;
    }

    ir_emit_code_line(cur_token.text);

    //now match param count
    if(musts[mfunc_index].func_param_cnt != i)
    {
        printf("%d: param count mismatch: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(RPAREN); // consumes RPAREN

    //now calculate all params addr
    if(funcs[func_index].func_param_cnt != 0)
    {
        int last_param_index = funcs[func_index].func_param_cnt - 1;
        for(int i=last_param_index; i>=0; i--)
        {
            if(i == last_param_index)
                funcs[func_index].params[i].addr = +8;
                //offset to bp
            else
            {
                int prev_allocated_param_addr = funcs[func_index].params[i+1].addr;
                funcs[func_index].params[i].addr = prev_allocated_param_addr + 4;
            }
        }
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

    nl(); // consumes NEWLINE
}

//type_index = index of the type being created
void rec_must_procedure(int type_index)
{
    //must_procedure ::= "mdef" funcname "(" {(primtype | typename) varname ","} ")" nl
    //                   "{" nl {primvardecl} {statement} "return" expression nl "}" nl

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes MDEF

    ir_emit_code_line(cur_token.text);

    //collect the address of this function
    int func_addr = get_text_addr_here();
    int locals_sizebyte = 0; //compute later

    //function entry
    ir_emit_code_line("=ENT size");
    put_opcode(ENT);
    int fix_addr1 = get_text_addr_here();
    put_operand(locals_sizebyte); //fix later

    //create anonymous func name
    //from the given must func name
    struct_token mangled_mfunc;
    int recnamelen = strlen(types[type_index].text);
    int mfuncnamelen = strlen(cur_token.text);
    if(recnamelen + mfuncnamelen > MAX_TOK_LEN_USABLE)
    {
        printf("%d: Identifier too big!\n", line_number);
        exit(1);
    }
    strcpy(mangled_mfunc.text, types[type_index].text);
    strcat(mangled_mfunc.text, " ");
    strcat(mangled_mfunc.text, cur_token.text);
    //in anonymous names, we used a space char
    //this is because, user can't create such name
    //and there will be no chance of accidental match
    //and we don't need to check for match as well.

    //check must function name declaration
    if(!funcs_contains(mangled_mfunc.text))
    {
        printf("%d: must function declaration inside record is mandatory: %s\n", line_number, cur_token.text);
        exit(1);
    }
    int func_index = get_func_index(mangled_mfunc.text);

    //save/update the address of this function
    funcs[func_index].addr = func_addr;

    make_mandatory(IDENT); // consumes IDENT

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    int i=1; //for param type, name, count match
             //i=0 is for "self"
    while(!check_cur_token(RPAREN))
    {
        ir_emit_code_line(cur_token.text);

        //match param type
        if(0 != strcmp(cur_token.text,
                funcs[func_index].params[i].type) )
        {
            printf("%d: param type mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        consume_cur_token(); // consumes type

        ir_emit_code_line(cur_token.text);

        //match param name
        if(0 != strcmp( cur_token.text,
                funcs[func_index].params[i].text) )
        {
            printf("%d: param name mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        consume_cur_token(); // consumes name

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }

        i++;
    }

    ir_emit_code_line(cur_token.text);

    //now match param count
    if(funcs[func_index].func_param_cnt != i)
    {
        printf("%d: param count mismatch: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(RPAREN); // consumes RPAREN

    nl(); // consumes NEWLINE

    //function body
    ir_emit_code_line(cur_token.text);
    make_mandatory(LBRACE); // consumes LBRACE

    nl(); // consumes NEWLINE

    //Parse all the declarations and statements
    //in the function body.
    while (!check_cur_token(RETURN))
    {
        if(check_cur_token(INT)
        || check_cur_token(CHAR))
        {
            primvardecl(func_index); // consumes own tokens
        }
        else
        {
            statement(func_index); // consumes own tokens
        }
    }

    ir_emit_code_line(cur_token.text);
    make_mandatory(RETURN); // consumes RETURN

    int ti = expression(func_index); //consumes own tokens
    if(ti != 0 && ti != 1) //if not (int or char)
    {
        printf("%d: func return can only be int or char: %s\n", line_number, cur_token.text);
        exit(1);
    }

    nl(); // consumes NEWLINE

    //calculate locals_sizebyte and fix ENT
    locals_sizebyte = funcs[func_index].func_local_cnt * 4;
    fix_operand(fix_addr1, locals_sizebyte);

    ir_emit_code_line(cur_token.text);
    make_mandatory(RBRACE); // consumes RBRACE

    ir_emit_code_line("=LEV");
    put_opcode(LEV);

    nl(); // consumes NEWLINE
}

//type_index = index of the type being created
void rec_can_declaration(int type_index)
{
    // inside record
    // "can" funcname "(" {(primtype | typename) varname ","} ")" nl

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes CAN

    ir_emit_code_line(cur_token.text);

    //check function name validity
    if(types_contains(cur_token.text))
    {
        printf("%d: can function name cant be same as type name: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //create anonymous func name
    //from the given must func name
    struct_token mangled_cfunc;
    int recnamelen = strlen(types[type_index].text);
    int mfuncnamelen = strlen(cur_token.text);
    if(recnamelen + mfuncnamelen > MAX_TOK_LEN_USABLE)
    {
        printf("%d: Identifier too big!\n", line_number);
        exit(1);
    }
    strcpy(mangled_cfunc.text, types[type_index].text);
    strcat(mangled_cfunc.text, " ");
    strcat(mangled_cfunc.text, cur_token.text);
    //in anonymous names, we used a space char
    //this is because, user can't create such name
    //and there will be no chance of accidental match
    //and we don't need to check for match as well.

    //check if multiple must function declaration
    if(funcs_contains(mangled_cfunc.text))
    {
        printf("%d: multiple must function declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //lets add the new function name
    funcs_add(mangled_cfunc.text);
    int func_index = get_func_index(mangled_cfunc.text);
    //this is func index of just added function

    make_mandatory(IDENT); // consumes IDENT

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    //add "self"
    params_add(func_index, "self", type_index); //self is actually of current type_index type

    int i=1; //for param type, name, count match
             //i=0 is for "self"
    while(!check_cur_token(RPAREN))
    {
        ir_emit_code_line(cur_token.text);

        //check datatype
        if (!types_contains(cur_token.text))
        {
            printf("%d: Unknown type: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //get param type index
        int param_ti = get_type_index(cur_token.text);
        //this is type index of params

        //consume datatype
        if(check_cur_token(INT)
        || check_cur_token(CHAR)
        || check_cur_token(IDENT))
        {
            consume_cur_token(); // consumes type
        }
        else
        {
            printf("%d: Expected datatype, got: %s\n", line_number, cur_token.text);
            exit(1);
        }

        ir_emit_code_line(cur_token.text);

        //check param name validity
        if(types_contains(cur_token.text))
        {
            printf("%d: param name can't be same as datatype: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //params can be same as globals
        //params and locals hide globals.

        // If param doesn't exist, declare it.
        if (!params_contains(func_index, cur_token.text))
        {
            params_add(func_index, cur_token.text, param_ti);
        }
        else
        {
            printf("%d: Multiple param declaration: %s\n", line_number, cur_token.text);
            exit(1);
        }

        make_mandatory(IDENT); // consumes IDENT

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }

        i++;
    }

    ir_emit_code_line(cur_token.text);
    make_mandatory(RPAREN); // consumes RPAREN

    //now calculate all params addr
    if(funcs[func_index].func_param_cnt != 0)
    {
        int last_param_index = funcs[func_index].func_param_cnt - 1;
        for(int i=last_param_index; i>=0; i--)
        {
            if(i == last_param_index)
                funcs[func_index].params[i].addr = +8;
                //offset to bp
            else
            {
                int prev_allocated_param_addr = funcs[func_index].params[i+1].addr;
                funcs[func_index].params[i].addr = prev_allocated_param_addr + 4;
            }
        }
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

    nl(); // consumes NEWLINE
}

//type_index = index of the type being created
void rec_can_procedure(int type_index)
{
    //can_procedure ::= "cdef" funcname "(" {(primtype | typename) varname ","} ")" nl
    //                  "{" nl {primvardecl} {statement} "return" expression nl "}" nl

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes CDEF

    ir_emit_code_line(cur_token.text);

    //collect the address of this function
    int func_addr = get_text_addr_here();
    int locals_sizebyte = 0; //compute later

    //function entry
    ir_emit_code_line("=ENT size");
    put_opcode(ENT);
    int fix_addr1 = get_text_addr_here();
    put_operand(locals_sizebyte); //fix later

    //create anonymous func name
    //from the given must func name
    struct_token mangled_cfunc;
    int recnamelen = strlen(types[type_index].text);
    int mfuncnamelen = strlen(cur_token.text);
    if(recnamelen + mfuncnamelen > MAX_TOK_LEN_USABLE)
    {
        printf("%d: Identifier too big!\n", line_number);
        exit(1);
    }
    strcpy(mangled_cfunc.text, types[type_index].text);
    strcat(mangled_cfunc.text, " ");
    strcat(mangled_cfunc.text, cur_token.text);
    //in anonymous names, we used a space char
    //this is because, user can't create such name
    //and there will be no chance of accidental match
    //and we don't need to check for match as well.

    //check must function name declaration
    if(!funcs_contains(mangled_cfunc.text))
    {
        printf("%d: can function declaration inside record is mandatory: %s\n", line_number, cur_token.text);
        exit(1);
    }
    int func_index = get_func_index(mangled_cfunc.text);

    //save/update the address of this function
    funcs[func_index].addr = func_addr;

    make_mandatory(IDENT); // consumes IDENT

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    int i=1; //for param type, name, count match
             //i=0 is for "self"
    while(!check_cur_token(RPAREN))
    {
        ir_emit_code_line(cur_token.text);

        //match param type
        if(0 != strcmp(cur_token.text,
                funcs[func_index].params[i].type) )
        {
            printf("%d: param type mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        consume_cur_token(); // consumes type

        ir_emit_code_line(cur_token.text);

        //match param name
        if(0 != strcmp( cur_token.text,
                funcs[func_index].params[i].text) )
        {
            printf("%d: param name mismatch: %s\n", line_number, cur_token.text);
            exit(1);
        }

        consume_cur_token(); // consumes name

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }

        i++;
    }

    ir_emit_code_line(cur_token.text);

    //now match param count
    if(funcs[func_index].func_param_cnt != i)
    {
        printf("%d: param count mismatch: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(RPAREN); // consumes RPAREN

    nl(); // consumes NEWLINE

    //function body
    ir_emit_code_line(cur_token.text);
    make_mandatory(LBRACE); // consumes LBRACE

    nl(); // consumes NEWLINE

    //Parse all the declarations and statements
    //in the function body.
    while (!check_cur_token(RETURN))
    {
        if(check_cur_token(INT)
        || check_cur_token(CHAR))
        {
            primvardecl(func_index); // consumes own tokens
        }
        else
        {
            statement(func_index); // consumes own tokens
        }
    }

    ir_emit_code_line(cur_token.text);
    make_mandatory(RETURN); // consumes RETURN

    int ti = expression(func_index); //consumes own tokens
    if(ti != 0 && ti != 1) //if not (int or char)
    {
        printf("%d: func return can only be int or char: %s\n", line_number, cur_token.text);
        exit(1);
    }

    nl(); // consumes NEWLINE

    //calculate locals_sizebyte and fix ENT
    locals_sizebyte = funcs[func_index].func_local_cnt * 4;
    fix_operand(fix_addr1, locals_sizebyte);

    ir_emit_code_line(cur_token.text);
    make_mandatory(RBRACE); // consumes RBRACE

    ir_emit_code_line("=LEV");
    put_opcode(LEV);

    nl(); // consumes NEWLINE
}

//type_index = index of the type being created
void rec_member_var_declaration(int type_index)
{
    //record variables
    //(primtype | typename) varname nl

    ir_emit_code_line(cur_token.text);

    //check datatype
    if (!types_contains(cur_token.text))
    {
        printf("%d: Unknown type: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //get type index
    int ti = get_type_index(cur_token.text); //this is type index of members

    //check if member datatype is same as
    //current datatype being defined
    if (type_index == ti)
    {
        printf("%d: Member type can't be same as parent type: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //consume datatype
    if(check_cur_token(INT)
    || check_cur_token(CHAR)
    || check_cur_token(IDENT))
    {
        consume_cur_token(); // consumes type
    }
    else
    {
        printf("%d: Expected datatype, got: %s\n", line_number, cur_token.text);
        exit(1);
    }

    ir_emit_code_line(cur_token.text);

    //check variable name validity
    if(types_contains(cur_token.text))
    {
        printf("%d: Member variable name can't be same as datatype: %s\n", line_number, cur_token.text);
        exit(1);
    }

    // If member variable doesn't exist, declare it.
    if (!members_contains(type_index, cur_token.text))
    {
        members_add(type_index, cur_token.text, ti);
    }
    else
    {
        printf("%d: Multiple member variable declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(IDENT); // consumes IDENT

    nl(); // consumes NEWLINE
}

//type_index = index of the type being created
void mustvar_type_declaration(int type_index)
{
    //codetype ::= "must" funcname

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes MUST

    ir_emit_code_line(cur_token.text);

    types[type_index].catg = FUNC_CATG;
    types[type_index].sizebyte = 4+4;

    //check if must func exists
    if (!musts_contains(cur_token.text))
    {
        printf("%d: Unknown must func type: %s\n", line_number, cur_token.text);
        exit(1);
    }

    strcpy(types[type_index].func_type, cur_token.text);

    //consume func type
    if(check_cur_token(INT)
    || check_cur_token(CHAR)
    || check_cur_token(IDENT))
    {
        consume_cur_token(); // consumes func type
    }
    else
    {
        printf("%d: Expected must func name, got: %s\n", line_number, cur_token.text);
        exit(1);
    }
}

void global_var_declaration()
{
    // global variables
    // (primtype | typename) varname nl

    ir_emit_code_line(cur_token.text);

    int type_index;

    //get type index
    type_index = get_type_index(cur_token.text);

    consume_cur_token(); // consumes type

    ir_emit_code_line(cur_token.text);

    //check variable name validity
    if(types_contains(cur_token.text))
    {
        printf("%d: Variable name can't be same as datatype: %s\n", line_number, cur_token.text);
        exit(1);
    }

    // If variable doesn't exist, declare it.
    if (!symbols_contains(cur_token.text))
    {
        symbols_add(cur_token, type_index);
    }
    else
    {
        printf("%d: Multiple variable declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    make_mandatory(IDENT); // consumes IDENT

    nl(); // consumes NEWLINE
}

void global_ref_declaration()
{
    // global ref variable
    // "ref" varname "points" (primtype | typename) nl

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes REF

    ir_emit_code_line(cur_token.text);

    //check variable name validity
    if(types_contains(cur_token.text))
    {
        printf("%d: Variable name can't be same as datatype: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //check if variable already exists
    if (symbols_contains(cur_token.text))
    {
        printf("%d: Multiple variable declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //save varname for later use
    struct_token var_token;
    var_token = cur_token;

    make_mandatory(IDENT); // consumes varname

    ir_emit_code_line(cur_token.text);
    make_mandatory(POINTS); // consumes POINTS

    ir_emit_code_line(cur_token.text);

    //create an anonymous ptr type
    //from the given val type
    struct_token typ_token;
    typ_token = cur_token;
    strcat(typ_token.text, " ptr"); //we have 8 char space, we are safe.
    //in anonymous type name, we used a space char
    //this is because, user can't create such type name
    //and there will be no chance of accidental match
    //and we don't need to check for match as well.

    //lets add the new type name
    types_add(typ_token.text);
    int type_index = get_type_index(typ_token.text); //this is type index of just added typename
    types[type_index].catg = PTR_CATG;
    types[type_index].sizebyte = 4;
    //add ptr val type now
    strcpy(types[type_index].ptr_val_type, cur_token.text);

    //consume ptr val type
    if(check_cur_token(INT)
    || check_cur_token(CHAR)
    || check_cur_token(IDENT))
    {
        consume_cur_token(); // consumes ptr val type
    }
    else
    {
        printf("%d: Expected datatype, got: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //now add the saved symbol
    symbols_add(var_token, type_index);

    nl(); // consumes NEWLINE
}

void function_declaration()
{
    // "func" funcname "(" {(primtype | typename) varname ","} ")" nl

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes FUNC

    ir_emit_code_line(cur_token.text);

    //check function name validity
    if(types_contains(cur_token.text))
    {
        printf("%d: function name cant be same as type name: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //check function name multiple declaration
    if(funcs_contains(cur_token.text))
    {
        printf("%d: Multiple function declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //lets add the new function name
    funcs_add(cur_token.text);
    int func_index = get_func_index(cur_token.text);
    //this is func index of just added function

    make_mandatory(IDENT); // consumes IDENT

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    while(!check_cur_token(RPAREN))
    {
        ir_emit_code_line(cur_token.text);

        //check datatype
        if (!types_contains(cur_token.text))
        {
            printf("%d: Unknown type: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //get param type index
        int param_ti = get_type_index(cur_token.text);
        //this is type index of params

        //consume datatype
        if(check_cur_token(INT)
        || check_cur_token(CHAR)
        || check_cur_token(IDENT))
        {
            consume_cur_token(); // consumes type
        }
        else
        {
            printf("%d: Expected datatype, got: %s\n", line_number, cur_token.text);
            exit(1);
        }

        ir_emit_code_line(cur_token.text);

        //check param name validity
        if(types_contains(cur_token.text))
        {
            printf("%d: param name can't be same as datatype: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //params can be same as globals
        //params and locals hide globals.

        // If param doesn't exist, declare it.
        if (!params_contains(func_index, cur_token.text))
        {
            params_add(func_index, cur_token.text, param_ti);
        }
        else
        {
            printf("%d: Multiple param declaration: %s\n", line_number, cur_token.text);
            exit(1);
        }

        make_mandatory(IDENT); // consumes IDENT

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }
    }

    ir_emit_code_line(cur_token.text);
    make_mandatory(RPAREN); // consumes RPAREN

    //now calculate all params addr
    if(funcs[func_index].func_param_cnt != 0)
    {
        int last_param_index = funcs[func_index].func_param_cnt - 1;
        for(int i=last_param_index; i>=0; i--)
        {
            if(i == last_param_index)
                funcs[func_index].params[i].addr = +8;
                //offset to bp
            else
            {
                int prev_allocated_param_addr = funcs[func_index].params[i+1].addr;
                funcs[func_index].params[i].addr = prev_allocated_param_addr + 4;
            }
        }
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

    nl(); // consumes NEWLINE
}

void global_must_declaration()
{
    // global
    // "must" funcname "(" {(primtype | typename) varname ","} ")" nl

    ir_emit_code_line(cur_token.text);
    consume_cur_token(); // consumes MUST

    ir_emit_code_line(cur_token.text);

    //check function name validity
    if(types_contains(cur_token.text))
    {
        printf("%d: must function name cant be same as type name: %s\n", line_number, cur_token.text);
        exit(1);
    }

    //check must function name multiple declaration
    if(musts_contains(cur_token.text))
    {
        printf("%d: Multiple must function declaration: %s\n", line_number, cur_token.text);
        exit(1);
    }
    //lets add the new must function name
    musts_add(cur_token.text);
    int mfunc_index = get_must_index(cur_token.text);
    //this is func index of just added must function

    make_mandatory(IDENT); // consumes IDENT

    ir_emit_code_line(cur_token.text);
    make_mandatory(LPAREN); // consumes LPAREN

    //add "self"
    mparams_add(mfunc_index, "self", 0); //0=int type
    //this is not the actual type of self
    //actual type is assigned inside class
    //here 0 is just assigned as a default

    while(!check_cur_token(RPAREN))
    {
        ir_emit_code_line(cur_token.text);

        //check datatype
        if (!types_contains(cur_token.text))
        {
            printf("%d: Unknown type: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //get param type index
        int param_ti = get_type_index(cur_token.text);
        //this is type index of params

        //consume datatype
        if(check_cur_token(INT)
        || check_cur_token(CHAR)
        || check_cur_token(IDENT))
        {
            consume_cur_token(); // consumes type
        }
        else
        {
            printf("%d: Expected datatype, got: %s\n", line_number, cur_token.text);
            exit(1);
        }

        ir_emit_code_line(cur_token.text);

        //check param name validity
        if(types_contains(cur_token.text))
        {
            printf("%d: param name can't be same as datatype: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //params can be same as globals
        //params and locals hide globals.

        // If param doesn't exist, declare it.
        if (!mparams_contains(mfunc_index, cur_token.text))
        {
            mparams_add(mfunc_index, cur_token.text, param_ti);
        }
        else
        {
            printf("%d: Multiple param declaration: %s\n", line_number, cur_token.text);
            exit(1);
        }

        make_mandatory(IDENT); // consumes IDENT

        //comma
        //four cases possible
        //1. func f(int a) //comma optional
        //2. func f(int a,) //consume comma
        //3. func f(int a, int b) //consume comma
        //4. func f(int a int b) //error
        if(check_cur_token(COMMA)
        || check_cur_token(RPAREN))
        {
            if(check_cur_token(COMMA)) //case 2 & 3
            {
                ir_emit_code_line(cur_token.text);
                consume_cur_token(); //consumes comma
            }
            //else fine, case 1
        }
        else //case 4
        {
            printf("%d: comma expected: %s\n", line_number, cur_token.text);
            exit(1);
        }
    }

    ir_emit_code_line(cur_token.text);
    make_mandatory(RPAREN); // consumes RPAREN

    //no need of calculating all params addr
    //because global must declartion is just for
    //matching function name, parameter name and type

    nl(); // consumes NEWLINE
}

//primary ::= intlit | charlit | accessor
//          | accessor_lval ".." mccall
//          | must_accessor_lval "::" pcall
//          | fcall | "(" expression ")"
int primary(int fi)
{
    int ti;

    if (check_cur_token(INTLIT))
    {
        ir_emit_code_line(cur_token.text);
        ir_emit_code_line("=IMM num");
        int num = convert_text_to_num(cur_token.text);
        put_opcode(IMM);
        put_operand(num);

        ti = 0; //int

        consume_cur_token(); // consumes INTLIT
    }
    else if (check_cur_token(CHARLIT))
    {
        ir_emit_code_line(cur_token.text);
        ir_emit_code_line("=IMM num");
        int num = convert_char_to_num(cur_token.text);
        put_opcode(IMM);
        put_operand(num);

        ti = 1; //char

        consume_cur_token(); // consumes CHARLIT
    }
    else if ( check_cur_token(IDENT)
              && (symbols_contains(cur_token.text)
                 || params_contains(fi, cur_token.text)
                 || locals_contains(fi, cur_token.text))
              && !check_upcoming_token(LPAREN) )
    {
        ti = accessor_lval(fi); //consumes own tokens

        if(check_cur_token(DOTDOT))
        {
            ir_emit_code_line(cur_token.text);
            consume_cur_token(); // consumes DOTDOT

            if(types[ti].catg != REC_CATG)
            {
                printf("%d: Expected a record before dotdot operator: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //create anonymous func name
            //from the given must/can func name
            struct_token mangled_mcfunc;
            int recnamelen = strlen(types[ti].text);
            int mfuncnamelen = strlen(cur_token.text);
            if(recnamelen + mfuncnamelen > MAX_TOK_LEN_USABLE)
            {
                printf("%d: Identifier too big!\n", line_number);
                exit(1);
            }
            strcpy(mangled_mcfunc.text, types[ti].text);
            strcat(mangled_mcfunc.text, " ");
            strcat(mangled_mcfunc.text, cur_token.text);
            //in anonymous names, we used a space char
            //this is because, user can't create such name
            //and there will be no chance of accidental match
            //and we don't need to check for match as well.

            //check if must/can function exists
            if(funcs_contains(mangled_mcfunc.text))
            {
                mccall(fi, ti); //consumes own tokens
                ti = 0; //0=int, to be returned
            }
            else
            {
                printf("%d: Undeclared record function: %s\n", line_number, cur_token.text);
                exit(1);
            }
        }
        else if(check_cur_token(TWOCOLON))
        {
            ir_emit_code_line(cur_token.text);
            consume_cur_token(); // consumes TWOCOLON

            if(types[ti].catg != FUNC_CATG)
            {
                printf("%d: Expected func type var before two colon operator: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //check if compatible function is called
            if(0 == strcmp(cur_token.text, types[ti].func_type) )
            {
                pcall(fi, ti); //consumes own tokens
                ti = 0; //0=int, to be returned
            }
            else
            {
                printf("%d: this must func type var can't call this function : %s\n", line_number, cur_token.text);
                exit(1);
            }
        }
        else
        {
            if(ti == 0) //int
            {
                ir_emit_code_line("=LI");
                put_opcode(LI);
            }
            else if(ti == 1) //char
            {
                ir_emit_code_line("=LC");
                put_opcode(LC);
            }
            else
            {
                printf("%d: Expected int or char value: %s\n", line_number, cur_token.text);
                exit(1);
            }
        }
    }
    else if (check_cur_token(IDENT)
          && funcs_contains(cur_token.text)
          && check_upcoming_token(LPAREN))
    {
        fcall(fi); //consumes own tokens
        ti = 0; //int
    }
    else if (check_cur_token(LPAREN))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes LPAREN

        ti = expression(fi); // consumes own tokens

        ir_emit_code_line(cur_token.text);
        make_mandatory(RPAREN); // consumes RPAREN
    }
    else
    {
        // Error!
        //abort("Unexpected token at " + cur_token.text);
        printf("%d: Unexpected token at ", line_number);
        printf("%s\n", cur_token.text);
        exit(1);
    }

    return ti;
}

int must_accessor_lval(int fi)
{
    int ti = accessor_lval(fi); //consumes own tokens

    if(types[ti].catg != FUNC_CATG)
    {
        printf("%d: Expected func type var: %s\n", line_number, cur_token.text);
        exit(1);
    }

    return ti;
}

int rec_accessor_lval(int fi)
{
    int ti = accessor_lval(fi); //consumes own tokens

    if(types[ti].catg != REC_CATG)
    {
        printf("%d: Expected rec type var: %s\n", line_number, cur_token.text);
        exit(1);
    }

    return ti;
}

// accessor_lval ::= varname {member}

//this is mainly for lhs.
//this returns variable type
//and loads variable address.

//this is also called from primary used by expression
//primary loads value of variable from its loaded address.
int accessor_lval(int fi) //function index
{
    int ti; //type index
    int si; //symbol index
    int pi; //param index
    int li; //local index
    int sym_addr;

    ir_emit_code_line(cur_token.text);

    //we need to check only the beginning identifier
    //if it is from globals or params or locals.
    //after dot or bracket, it is members only.

    // params and locals hide globals.
    if (params_contains(fi, cur_token.text))
    {
        pi = get_param_index(fi, cur_token.text);
        ti = get_type_index(funcs[fi].params[pi].type);

        sym_addr = funcs[fi].params[pi].addr;
        ir_emit_code_line("=LEA offset");
        put_opcode(LEA);
        put_operand(sym_addr);

        //load address of the param
        ir_emit_code_line("=LI");
        put_opcode(LI);

        if(types[ti].catg == PTR_CATG)
        {
            //check if value datatype exists
            if (!types_contains(types[ti].ptr_val_type))
            {
                printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            int vti; //value type index
            vti = get_type_index(types[ti].ptr_val_type);

            //check if value type is a pointer type
            if(types[vti].catg == PTR_CATG) //IMP
            {
                printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //modify ti
            ti = vti;

            //load address of the object
            ir_emit_code_line("=LI");
            put_opcode(LI);
        }
    }
    else if (locals_contains(fi, cur_token.text))
    {
        li = get_local_index(fi, cur_token.text);
        ti = get_type_index(funcs[fi].locals[li].type);

        sym_addr = funcs[fi].locals[li].addr;
        ir_emit_code_line("=LEA offset");
        put_opcode(LEA);
        put_operand(sym_addr);

        //locals can't be ptr type
        //so no ptr val type checking
        //is needed like above cases
    }
    else if (symbols_contains(cur_token.text))
    {
        si = get_symbol_index(cur_token.text);
        ti = get_type_index(symbols[si].type);

        sym_addr = get_addr_of_symbol(cur_token);
        ir_emit_code_line("=IMM addr");
        put_opcode(IMM);
        put_operand(sym_addr);

        if(types[ti].catg == PTR_CATG)
        {
            //check if value datatype exists
            if (!types_contains(types[ti].ptr_val_type))
            {
                printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            int vti; //value type index
            vti = get_type_index(types[ti].ptr_val_type);

            //check if value type is a pointer type
            if(types[vti].catg == PTR_CATG) //IMP
            {
                printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //modify ti
            ti = vti;

            //load address of the object
            ir_emit_code_line("=LI");
            put_opcode(LI);
        }
    }
    else
    {
        printf("%d: Undeclared variable: %s\n", line_number, cur_token.text);
        exit(1);
    }

    consume_cur_token(); // consumes IDENT

    while(check_cur_token(DOT) || check_cur_token(LBRKT))
    {
        if(check_cur_token(DOT) && types[ti].catg != REC_CATG)
        {
            printf("%d: Expected a record before dot operator: %s\n", line_number, cur_token.text);
            exit(1);
        }

        if(check_cur_token(LBRKT) && types[ti].catg != ARR_CATG)
        {
            printf("%d: Expected an array before bracket operator: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //member returns member data type
        //which is returned from this function
        int mti = member(fi, ti);

        if(types[mti].catg == PTR_CATG)
        {
            //check if value datatype exists
            if (!types_contains(types[mti].ptr_val_type))
            {
                printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            int vti; //value type index
            vti = get_type_index(types[mti].ptr_val_type);

            //check if value type is a pointer type
            if(types[vti].catg == PTR_CATG) //IMP
            {
                printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //modify ti
            mti = vti;

            //load address of the object
            ir_emit_code_line("=LI");
            put_opcode(LI);
        }

        //modify ti
        ti = mti;
    }

    return ti;
}

// member ::= "." varname
//          | "[" (intlit | int_accessor_lval) "]"
int member(int fi, int pti) //pti=parent type index
{
    int mti; //mti=member type index

    if(check_cur_token(DOT))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes DOT

        if(members_contains(pti, cur_token.text))
        {
            ir_emit_code_line(cur_token.text);

            //push base address of parent
            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            //get member data type
            int mi = get_member_index(pti, cur_token.text);
            mti = get_type_index(types[pti].rec_mem_list[mi].type); //to be returned

            //load offset value
            int offset_value = types[pti].rec_mem_list[mi].addr;
            ir_emit_code_line("=IMM num");
            put_opcode(IMM);
            put_operand(offset_value);

            //now add the offset to base address
            ir_emit_code_line("=ADD");
            put_opcode(ADD);

            make_mandatory(IDENT); // consumes IDENT
        }
        else
        {
            printf("%d: Undeclared member: %s\n", line_number, cur_token.text);
            exit(1);
        }
    }
    else if(check_cur_token(LBRKT))
    {
        ir_emit_code_line(cur_token.text);
        consume_cur_token(); // consumes LBRKT

        //get member data type and size
        mti = get_type_index(types[pti].arr_mem_type); //to be returned
        int msize = types[mti].sizebyte;

        if(check_cur_token(INTLIT))
        {
            ir_emit_code_line(cur_token.text);

            //push base address of parent
            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            //load index value
            int index_value = convert_text_to_num(cur_token.text);
            ir_emit_code_line("=IMM num");
            put_opcode(IMM);
            put_operand(index_value);

            //push index value
            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            //load member datatypes's size
            ir_emit_code_line("=IMM num");
            put_opcode(IMM);
            put_operand(msize);

            //multiply index and member size
            ir_emit_code_line("=MUL");
            put_opcode(MUL);

            //now add the multiplication result to base address
            ir_emit_code_line("=ADD");
            put_opcode(ADD);

            consume_cur_token(); // consumes INTLIT
        }
        else if(check_cur_token(IDENT))
        {
            //push base address of parent
            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            int iti; //index variable's type index
            iti = accessor_lval(fi); //consumes own tokens

            if(iti == 0) //int
            {
                ir_emit_code_line("=LI");
                put_opcode(LI);
            }
            else
            {
                printf("%d: Expected int variable: %s\n", line_number, cur_token.text);
                exit(1);
            }

            //push index value
            ir_emit_code_line("=PUSH");
            put_opcode(PUSH);

            //load member datatypes's size
            ir_emit_code_line("=IMM num");
            put_opcode(IMM);
            put_operand(msize);

            //multiply index and member size
            ir_emit_code_line("=MUL");
            put_opcode(MUL);

            //now add the multiplication result to base address
            ir_emit_code_line("=ADD");
            put_opcode(ADD);
        }
        else
        {
            printf("%d: Expected integer literal or int (primary) variable: %s\n", line_number, cur_token.text);
            exit(1);
        }

        ir_emit_code_line(cur_token.text);
        make_mandatory(RBRKT); // consumes RBRKT
    }

    return mti;
}

// accessor_lval ::= varname {member}

//this takes a pointer variable.
//this doesn't return its type,
//it returns its type's ptr-value-type!

//again it doesn't load addr of pointed object,
//it loads the addr of the pointer itself!

//this ensures accessor is a ptr
int ptr_accessor_lval(int fi)
{
    int ti; //type index
    int si; //symbol index
    int pi; //param index
    int sym_addr;

    ir_emit_code_line(cur_token.text);

    //we need to check only the beginning identifier
    //if it is from globals or params or locals.
    //after dot or bracket, it is members only.

    // params and locals hide globals.
    if (params_contains(fi, cur_token.text))
    {
        pi = get_param_index(fi, cur_token.text);
        ti = get_type_index(funcs[fi].params[pi].type);

        sym_addr = funcs[fi].params[pi].addr;
        ir_emit_code_line("=LEA offset");
        put_opcode(LEA);
        put_operand(sym_addr);

        //load address of the param
        ir_emit_code_line("=LI");
        put_opcode(LI);

        consume_cur_token(); // consumes IDENT

        //if it is accessed as an array or record,
        //it could be a "pointer" pointing to array or record
        //or it could be a "normal" array or record
        if(check_cur_token(DOT) || check_cur_token(LBRKT))
        {
            //the identifier is a "pointer"
            //so it must be a "pointer" pointing to array or record
            if(types[ti].catg == PTR_CATG)
            {
                //check if value datatype exists
                if (!types_contains(types[ti].ptr_val_type))
                {
                    printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                int vti; //value type index
                vti = get_type_index(types[ti].ptr_val_type);

                //check if value type is a pointer type
                if(types[vti].catg == PTR_CATG) //IMP
                {
                    printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                //modify ti
                ti = vti;

                //load address of the object
                ir_emit_code_line("=LI");
                put_opcode(LI);
            }
            //the identifier is not a pointer
            //so it must be a "normal" array or record
            else
            {
                //no need to anything
                //this is a record/array var
                //member() will do needful
            }
        }
        //it is not accessed as an array or record
        //it could be a "pointer" pointing to atomic var
        //or it could be a "normal" var
        else
        {
            //the identifier is a pointer
            //so it could be a "pointer"
            //pointing to atomic var
            if(types[ti].catg == PTR_CATG)
            {
                //check if value datatype exists
                if (!types_contains(types[ti].ptr_val_type))
                {
                    printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                int vti; //value type index
                vti = get_type_index(types[ti].ptr_val_type);

                //check if value type is a pointer type
                if(types[vti].catg == PTR_CATG) //IMP
                {
                    printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                //modify ti
                ti = vti;

                //don't load address of the object
                //because we are going to assign addr to this pointer

                //ir_emit_code_line("=LI");
                //put_opcode(LI);
            }
            //the identifier is not a pointer
            //it must be a "normal" var
            else
            {
                printf("%d: ptr accessor lval must be a ptr: %s\n", line_number, cur_token.text);
                exit(1);
            }
        }
    }
    //else if (locals_contains(fi, cur_token.text))
    //{
        //locals can't be ptr type
        //so this case doen't exist
        //for ptr_accessor_lval()
    //}
    else if (symbols_contains(cur_token.text))
    {
        si = get_symbol_index(cur_token.text);
        ti = get_type_index(symbols[si].type);

        sym_addr = get_addr_of_symbol(cur_token);
        ir_emit_code_line("=IMM addr");
        put_opcode(IMM);
        put_operand(sym_addr);

        consume_cur_token(); // consumes IDENT

        //if it is accessed as an array or record,
        //it could be a "pointer" pointing to array or record
        //or it could be a "normal" array or record
        if(check_cur_token(DOT) || check_cur_token(LBRKT))
        {
            //the identifier is a "pointer"
            //so it must be a "pointer" pointing to array or record
            if(types[ti].catg == PTR_CATG)
            {
                //check if value datatype exists
                if (!types_contains(types[ti].ptr_val_type))
                {
                    printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                int vti; //value type index
                vti = get_type_index(types[ti].ptr_val_type);

                //check if value type is a pointer type
                if(types[vti].catg == PTR_CATG) //IMP
                {
                    printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                //modify ti
                ti = vti;

                //load address of the object
                ir_emit_code_line("=LI");
                put_opcode(LI);
            }
            //the identifier is not a pointer
            //so it must be a "normal" array or record
            else
            {
                //no need to anything
                //this is a record/array var
                //member() will do needful
            }
        }
        //it is not accessed as an array or record
        //it could be a "pointer" pointing to atomic var
        //or it could be a "normal" var
        else
        {
            //the identifier is a pointer
            //so it could be a "pointer"
            //pointing to atomic var
            if(types[ti].catg == PTR_CATG)
            {
                //check if value datatype exists
                if (!types_contains(types[ti].ptr_val_type))
                {
                    printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                int vti; //value type index
                vti = get_type_index(types[ti].ptr_val_type);

                //check if value type is a pointer type
                if(types[vti].catg == PTR_CATG) //IMP
                {
                    printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                //modify ti
                ti = vti;

                //don't load address of the object
                //because we are going to assign addr to this pointer

                //ir_emit_code_line("=LI");
                //put_opcode(LI);
            }
            //the identifier is not a pointer
            //it must be a "normal" var
            else
            {
                printf("%d: ptr accessor lval must be a ptr: %s\n", line_number, cur_token.text);
                exit(1);
            }
        }
    }
    else
    {
        printf("%d: Undeclared variable: %s\n", line_number, cur_token.text);
        exit(1);
    }

    while(check_cur_token(DOT) || check_cur_token(LBRKT))
    {
        if(check_cur_token(DOT) && types[ti].catg != REC_CATG)
        {
            printf("%d: Expected a record before dot operator: %s\n", line_number, cur_token.text);
            exit(1);
        }

        if(check_cur_token(LBRKT) && types[ti].catg != ARR_CATG)
        {
            printf("%d: Expected an array before bracket operator: %s\n", line_number, cur_token.text);
            exit(1);
        }

        //member returns member data type
        //which is returned from this function
        int mti = member(fi, ti);

        //if it is accessed as an array or record,
        //it could be a "pointer" pointing to array or record
        //or it could be a "normal" array or record
        if(check_cur_token(DOT) || check_cur_token(LBRKT))
        {
            //the identifier is a "pointer"
            //so it must be a "pointer" pointing to array or record
            if(types[mti].catg == PTR_CATG)
            {
                //check if value datatype exists
                if (!types_contains(types[mti].ptr_val_type))
                {
                    printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                int vti; //value type index
                vti = get_type_index(types[mti].ptr_val_type);

                //check if value type is a pointer type
                if(types[vti].catg == PTR_CATG) //IMP
                {
                    printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                //modify mti
                mti = vti;

                //load address of the object
                ir_emit_code_line("=LI");
                put_opcode(LI);
            }
            //the identifier is not a pointer
            //so it must be a "normal" array or record
            else
            {
                //no need to anything
                //this is a record/array var
                //member() will do needful
            }
        }
        //it is not accessed as an array or record
        //it could be a "pointer" pointing to atomic var
        //or it could be a "normal" var
        else
        {
            //the identifier is a pointer
            //so it could be a "pointer"
            //pointing to atomic var
            if(types[mti].catg == PTR_CATG)
            {
                //check if value datatype exists
                if (!types_contains(types[mti].ptr_val_type))
                {
                    printf("%d: Unknown pointer value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                int vti; //value type index
                vti = get_type_index(types[mti].ptr_val_type);

                //check if value type is a pointer type
                if(types[vti].catg == PTR_CATG) //IMP
                {
                    printf("%d: Expected non pointer type as ptr value type: %s\n", line_number, cur_token.text);
                    exit(1);
                }

                //modify mti
                mti = vti;

                //don't load address of the object
                //because we are going to assign addr to this pointer

                //ir_emit_code_line("=LI");
                //put_opcode(LI);
            }
            //the identifier is not a pointer
            //it must be a "normal" var
            else
            {
                printf("%d: ptr accessor lval must be a ptr: %s\n", line_number, cur_token.text);
                exit(1);
            }
        }

        //modify ti
        ti = mti;
    }

    return ti;
}
