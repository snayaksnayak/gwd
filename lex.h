void lex_init(char* buf, int buf_size);
void consume_cur_char();
char upcoming_char();
void skip_white_space();
void skip_comment();
void init_token(int start, int end, int kind);
int comp_tok_text(char* keyword);
void decide_key_or_id_token();
void get_token();
void init_token_text(int start, int end);


