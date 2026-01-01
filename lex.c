#include <stdio.h> //for fopen, printf
#include <stdlib.h> //for malloc, exit
#include <memory.h> //for memcp
#include <string.h> //for strcmp
#include <ctype.h> //for isdigit

#include "types.h"
#include "lex.h"

char *source;
int src_size;
char cur_char;
int cur_pos;
int line_number=1;

struct_token token;

void lex_init(char* buf, int buf_size)
{
    source = buf;
    src_size = buf_size;
    cur_char = -1; //set to invalid
    cur_pos = -1; //set to invalid
    consume_cur_char(); //come to first char in source
}

void consume_cur_char()
{
    cur_pos += 1;
    if (cur_pos >= src_size)
    {
        cur_char = '\0';
        cur_pos = src_size;
    }
    else
        cur_char = source[cur_pos];
}

char upcoming_char()
{
    if (cur_pos + 1 >= src_size)
        return '\0';
    else
        return source[cur_pos+1];
}

void skip_white_space()
{
    while( cur_char == ' ' || cur_char == '\t' || cur_char == '\r')
        consume_cur_char();
}

void skip_comment()
{
    if (cur_char == '#')
        while (cur_char != '\n')
            consume_cur_char();
}

void init_token_text(int start, int end)
{
    int tok_len = end - start + 1;
    if(tok_len > MAX_TOK_LEN_USABLE)
    {
        printf("%d: Identifier too big!\n", line_number);
        exit(1);
    }
    int pos = start;
    int i=0;
    while(pos <= end && pos < src_size)
    {
        token.text[i]=source[pos];
        i += 1;
        pos += 1;
    }
    token.text[i]='\0';
}

//to decide token kind, pass TBD
void init_token(int start, int end, int kind)
{
    init_token_text(start, end);
    if(kind != TBD)
    {
        token.kind = kind;
    }
    else
    {
        decide_key_or_id_token();
    }
}

int comp_tok_text(char* keyword)
{
    if (0==strcmp(token.text, keyword))
        return 1;
    else
        return 0;
}

void decide_key_or_id_token()
{
    //keyword
    if(comp_tok_text("type"))
        token.kind = TYPE;
    else if(comp_tok_text("print"))
        token.kind = PRINT;
    else if(comp_tok_text("input"))
        token.kind = INPUT;
    else if(comp_tok_text("if"))
        token.kind = IF;
    else if(comp_tok_text("then"))
        token.kind = THEN;
    else if(comp_tok_text("else"))
        token.kind = ELSE;
    else if(comp_tok_text("endif"))
        token.kind = ENDIF;
    else if(comp_tok_text("while"))
        token.kind = WHILE;
    else if(comp_tok_text("repeat"))
        token.kind = REPEAT;
    else if(comp_tok_text("endwhile"))
        token.kind = ENDWHILE;
    else if(comp_tok_text("int"))
        token.kind = INT;
    else if(comp_tok_text("char"))
        token.kind = CHAR;
    else if(comp_tok_text("points"))
        token.kind = POINTS;
    else if(comp_tok_text("array"))
        token.kind = ARRAY;
    else if(comp_tok_text("record"))
        token.kind = RECORD;
    else if(comp_tok_text("endrec"))
        token.kind = ENDREC;
    else if(comp_tok_text("bind"))
        token.kind = BIND;
    else if(comp_tok_text("new"))
        token.kind = NEW;
    else if(comp_tok_text("move"))
        token.kind = MOVE;
    else if(comp_tok_text("free"))
        token.kind = FREE;
    else if(comp_tok_text("ref"))
        token.kind = REF;
    else if(comp_tok_text("func"))
        token.kind = FUNC;
    else if(comp_tok_text("fdef"))
        token.kind = FDEF;
    else if(comp_tok_text("return"))
        token.kind = RETURN;
    else if(comp_tok_text("must"))
        token.kind = MUST;
    else if(comp_tok_text("mdef"))
        token.kind = MDEF;
    else if(comp_tok_text("can"))
        token.kind = CAN;
    else if(comp_tok_text("cdef"))
        token.kind = CDEF;
    else if(comp_tok_text("poly"))
        token.kind = POLY;
    else //identifier
        token.kind = IDENT;
}

void get_token()
{
    skip_white_space();
    skip_comment();

    if (cur_char == '+')
    {
        init_token(cur_pos, cur_pos, PLUS);
        consume_cur_char(); //+
    }
    else if (cur_char == '-')
    {
        init_token(cur_pos, cur_pos, MINUS);
        consume_cur_char(); //-
    }
    else if (cur_char == '*')
    {
        init_token(cur_pos, cur_pos, ASTERISK);
        consume_cur_char(); //*
    }
    else if (cur_char == '/')
    {
        init_token(cur_pos, cur_pos, SLASH);
        consume_cur_char(); // /
    }
    else if (cur_char == '=')
    {
        // Check whether this token is = or ==
        if (upcoming_char() == '=')
        {
            int last_pos = cur_pos;
            consume_cur_char(); //first =
            init_token(last_pos, cur_pos, EQEQ);
            consume_cur_char(); //second =
        }
        else
        {
            init_token(cur_pos, cur_pos, EQUAL);
            consume_cur_char(); //=
        }
    }
    else if (cur_char == '>')
    {
        // Check whether this is token is > or >=
        if (upcoming_char() == '=')
        {
            int last_pos = cur_pos;
            consume_cur_char(); //>
            init_token(last_pos, cur_pos, GTEQ);
            consume_cur_char(); //=
        }
        else
        {
            init_token(cur_pos, cur_pos, GRET);
            consume_cur_char(); //>
        }
    }
    else if (cur_char == '<')
    {
        // Check whether this is token is < or <=
        if (upcoming_char() == '=')
        {
            int last_pos = cur_pos;
            consume_cur_char(); //<
            init_token(last_pos, cur_pos, LTEQ);
            consume_cur_char(); //=
        }
        else
        {
            init_token(cur_pos, cur_pos, LESS);
            consume_cur_char(); //<
        }
    }
    else if (cur_char == '!')
    {
        if (upcoming_char() == '=')
        {
            int last_pos = cur_pos;
            consume_cur_char(); // !
            init_token(last_pos, cur_pos, NOTEQ);
            consume_cur_char(); //=
        }
        else
        {
            printf("%d: Expected !=, got !\n", line_number);
            exit(1);
        }
    }
    else if (cur_char == ':')
    {
        if (upcoming_char() == ':')
        {
            int last_pos = cur_pos;
            consume_cur_char(); //first :
            init_token(last_pos, cur_pos, TWOCOLON);
            consume_cur_char(); //second :
        }
        else
        {
            printf("%d: Expected ::, got :\n", line_number);
            exit(1);
        }
    }
    else if (cur_char == '(')
    {
        init_token(cur_pos, cur_pos, LPAREN);
        consume_cur_char(); // (
    }
    else if (cur_char == ')')
    {
        init_token(cur_pos, cur_pos, RPAREN);
        consume_cur_char(); // )
    }
    else if (cur_char == '|')
    {
        init_token(cur_pos, cur_pos, PIPE);
        consume_cur_char(); // |
    }
    else if (cur_char == '&')
    {
        init_token(cur_pos, cur_pos, AMPERSAND);
        consume_cur_char(); // &
    }
    else if (cur_char == '~')
    {
        init_token(cur_pos, cur_pos, TILDE);
        consume_cur_char(); // ~
    }
    else if (cur_char == '.')
    {
        // Check whether this is token is . or ..
        if (upcoming_char() == '.')
        {
            int last_pos = cur_pos;
            consume_cur_char(); //first .
            init_token(last_pos, cur_pos, DOTDOT);
            consume_cur_char(); //second .
        }
        else
        {
            init_token(cur_pos, cur_pos, DOT);
            consume_cur_char(); //.
        }
    }
    else if (cur_char == '[')
    {
        init_token(cur_pos, cur_pos, LBRKT);
        consume_cur_char(); // [
    }
    else if (cur_char == ']')
    {
        init_token(cur_pos, cur_pos, RBRKT);
        consume_cur_char(); // ]
    }
    else if (cur_char == ',')
    {
        init_token(cur_pos, cur_pos, COMMA);
        consume_cur_char(); // ,
    }
    else if (cur_char == '{')
    {
        init_token(cur_pos, cur_pos, LBRACE);
        consume_cur_char(); // {
    }
    else if (cur_char == '}')
    {
        init_token(cur_pos, cur_pos, RBRACE);
        consume_cur_char(); // }
    }
    else if (cur_char == '\"')
    {
        // Get characters between quotations.
        consume_cur_char(); //first "
        int start_pos = cur_pos;

        while (cur_char != '\"')
        {
            // Don't allow special characters in the string.
            // No carriage returns, newlines,
            // tabs, escape characters and percentage.
            if (cur_char == '\r' || cur_char == '\n'
            || cur_char == '\t' || cur_char == '\\' || cur_char == '%')
            {
                printf("%d: Illegal character in string.\n", line_number);
                exit(1);
            }
            else
            {
                consume_cur_char(); //each char in string
            }
        }
        init_token(start_pos, cur_pos-1, STRLIT);
        consume_cur_char(); //second "
    }
    else if (isdigit(cur_char))
    {
        // Leading character is a digit,
        // so this must be a number.
        // Get all consecutive digits.
        int start_pos = cur_pos;
        while (isdigit(upcoming_char()))
        {
            consume_cur_char(); //each digit of whole part
                                //except the last one
        }
        init_token(start_pos, cur_pos, INTLIT);
        consume_cur_char(); //last digit of whole part
    }
    else if (isalpha(cur_char))
    {
        // Leading character is a letter,
        // so this must be an identifier or a keyword.
        // Get all consecutive alpha numeric characters and underscore.
        int start_pos = cur_pos;
        while (isalnum(upcoming_char()) || upcoming_char()=='_')
        {
            consume_cur_char(); //each char of identifier/keyword
                                //except the last one
        }

        // Check if the token is in the list of keywords.
        // token kind will be decided inside this function
        init_token(start_pos, cur_pos, TBD);
        consume_cur_char(); //last char of identifier/keyword
    }
    else if (cur_char == '\'')
    {
        // Get character between single quote.
        consume_cur_char(); //first '

        if (upcoming_char() == '\'')
        {
            init_token(cur_pos, cur_pos, CHARLIT);
            consume_cur_char(); // character
        }
        else
        {
            printf("%d: Expected char literal\n", line_number);
            exit(1);
        }

        consume_cur_char(); //second '
    }
    else if (cur_char == '\n')
    {
        // Newline.
        init_token(cur_pos, cur_pos, NEWLINE);
        consume_cur_char(); // \n

        line_number++;
    }
    else if (cur_char == '\0')
    {
        // ENDOFFILE.
        init_token(cur_pos, cur_pos, ENDOFFILE);
        consume_cur_char(); // \0
    }
    else
    {
        // Unknown token!
        printf("%d: Lexing error: ", line_number);
        printf("Unknown token: ");
        printf("%c\n", cur_char);
        exit(1);
    }
}
