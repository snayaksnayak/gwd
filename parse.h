char* get_kind_string(int kind);
void consume_cur_token();
int check_cur_token(int kind);
int check_upcoming_token(int kind);
void make_mandatory(int kind);
int cur_token_is_relop();
void print_cur_token_text();
void print_upcoming_token_text();
void parse_init();
void print_all_token();
int convert_text_to_num(char* text);
int convert_char_to_num(char* text);

int symbols_contains(char* symname);
void symbols_add(struct_token t, int type_index);
int get_symbol_index(char* symname);

int types_contains(char* typename);
void types_add(char* typename); //adds only type name
int get_type_index(char* typename);

int members_contains(int rec_ti, char* memname);
void members_add(int rec_ti, char* memname, int mem_ti);
int get_member_index(int rec_ti, char* memname);



//global funcs
int funcs_contains(char* funcname);
void funcs_add(char* funcname);
int get_func_index(char* funcname);

//global func params
int params_contains(int func_index, char* paramname);
void params_add(int func_index, char* paramname, int param_ti);
int get_param_index(int func_index, char* paramname);

//global func locals
int locals_contains(int func_index, char* localname);
void locals_add(int func_index, char* localname, int local_ti);
int get_local_index(int func_index, char* localname);



//global musts
int musts_contains(char* funcname);
void musts_add(char* funcname);
int get_must_index(char* funcname);

//global must params
int mparams_contains(int mfunc_index, char* paramname);
void mparams_add(int mfunc_index, char* paramname, int param_ti);
int get_mparam_index(int mfunc_index, char* paramname);



int get_addr_of_symbol(struct_token t);
void print_symbols();
void print_types();
void print_funcs();
void print_musts();

void program();
void statement(int fi);
void comparison(int fi);
int eq(int fi);
int neq(int fi);
int gt(int fi);
int ge(int fi);
int lt(int fi);
int le(int fi);
int expression(int fi);
int add(int fi);
int sub(int fi);
int term(int fi);
int mul_term(int fi);
int div_term(int fi);
int unary(int fi);
int plus(int fi);
int minus(int fi);
int primary(int fi);
void logical_expression(int fi);
void logical_or(int fi);
void logical_term(int fi);
void logical_and(int fi);
void logical_unary(int fi);
void logical_not(int fi);
void logical_primary(int fi);
void nl();
int accessor_lval(int fi);
int member(int fi, int pti); //pti=parent type index
int ptr_accessor_lval(int fi);

void declaration();
void procedure();
void primvardecl(int fi);
void fcall(int fi);
void mccall(int fi, int pti); //pti=parent type index
int must_accessor_lval(int fi);
int rec_accessor_lval(int fi);

void type_declaration();
void global_var_declaration();
void global_ref_declaration();
void function_declaration();
void global_must_declaration();

void pointer_type_declaration(int type_index);
void array_type_declaration(int type_index);
void record_type_declaration(int type_index);
void mustvar_type_declaration(int type_index);

void rec_ref_var_declaration(int type_index);
void rec_must_declaration(int type_index);
void rec_must_procedure(int type_index);
void rec_can_declaration(int type_index);
void rec_can_procedure(int type_index);
void rec_member_var_declaration(int type_index);

