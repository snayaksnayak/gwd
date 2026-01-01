#define DEBUG 1
#define INTERPRET 0

#define MAX_TOK_LEN 128
#define MAX_TOK_LEN_USABLE 120 //to keep some space from token len for internal use

typedef struct
{
    char text[MAX_TOK_LEN+1];
    int kind;
} struct_token;

//kind
enum {
    TBD = 0, //token kind to be decided
    ENDOFFILE = 1,
    NEWLINE = 2,
    INTLIT = 3,
    CHARLIT = 4,
    IDENT = 5,
    STRLIT = 6,
    LPAREN = 7,
    RPAREN = 8,
    DOT = 9,
    LBRKT = 10,
    RBRKT = 11,
    COMMA = 12,
    LBRACE = 13,
    RBRACE = 14,
    DOTDOT = 15,
    TWOCOLON = 16,
    // Keywords.
    TYPE = 100,
    PRINT = 101,
    INPUT = 102,
    IF = 103,
    THEN = 104,
    ELSE = 105,
    ENDIF = 106,
    WHILE = 107,
    REPEAT = 108,
    ENDWHILE = 109,
    INT = 110,
    CHAR = 111,
    POINTS = 112,
    ARRAY = 113,
    RECORD = 114,
    ENDREC = 115,
    BIND = 116,
    NEW = 117,
    MOVE = 118,
    FREE = 119,
    REF = 120,
    FUNC = 121,
    FDEF = 122,
    RETURN = 123,
    MUST = 124,
    MDEF = 125,
    CAN = 126,
    CDEF = 127,
    POLY = 128,
    // Operators.
    EQUAL = 200,
    PLUS = 201,
    MINUS = 202,
    ASTERISK = 203,
    SLASH = 204,
    EQEQ = 205,
    NOTEQ = 206,
    LESS = 207,
    LTEQ = 208,
    GRET = 209,
    GTEQ = 210,
    PIPE = 211,
    AMPERSAND = 212,
    TILDE = 213
};

#define MAX_SYMBOLS 1000
#define MAX_MEMBERS 30

#define MAX_TYPES 100

#define MAX_FUNCS 100
#define MAX_MUSTS 100
#define MAX_CANS 100

#define MAX_PARAMS 10
#define MAX_LOCALS 10

typedef struct
{
    char text[MAX_TOK_LEN+1]; //variable name or string itself
    char type[MAX_TOK_LEN+1]; //string literal,int,char,typename
    int addr; //addr in data section
} struct_symbol;

typedef struct
{
    char text[MAX_TOK_LEN+1]; //function name
    int addr; //addr in text section
    struct_symbol params[MAX_PARAMS];
    int func_param_cnt; //positiveint
    struct_symbol locals[MAX_LOCALS];
    int func_local_cnt; //positiveint
} struct_func;

typedef struct
{
    char text[MAX_TOK_LEN+1]; //int,char,typename
    int catg; //int,char,arr,ptr,rec,func
    int sizebyte; //positiveint; size of datatype, not of object
                  //for datatype char, size is 1; for object of char, size is 4
                  //for datatype array char 10, size is 10; for object of this type, size is 12

    char ptr_val_type[MAX_TOK_LEN+1]; //int,char,typename

    char arr_mem_type[MAX_TOK_LEN+1]; //int,char,typename
    int arr_mem_cnt; //positiveint

    struct_symbol rec_mem_list[MAX_MEMBERS]; //list of symbols
    int rec_mem_cnt; //positiveint

    char func_type[MAX_TOK_LEN+1]; //funcname

} struct_type;

//catg
enum {
    INT_CATG = 0,
    CHAR_CATG = 1,

    PTR_CATG = 2,
    ARR_CATG = 3,
    REC_CATG = 4,

    FUNC_CATG = 5,
};

#define MEM_SIZE 40000
#define TEXT_SIZE 16000
#define DATA_SIZE 4000
#define INVALID_ADDR -1

enum {
    HLT=0,
    LEA ,
    IMM ,
    JMP ,
    CALL,
    ICALL,
    SWAP,
    JZ  ,
    JNZ ,
    ENT ,
    ADJ ,
    LEV ,
    LI  ,
    LC  ,
    SI  ,
    SC  ,
    PUSH,
    OR  ,
    XOR ,
    AND ,
    EQ  ,
    NE  ,
    LT  ,
    GT  ,
    LE  ,
    GE  ,
    SHL ,
    SHR ,
    ADD ,
    SUB ,
    MUL ,
    DIV ,
    MOD ,
    ININT,
    INCHAR,
    OUTSTR,
    OUTINT,
    OUTCHAR
};

#define MAX_FIXES 10
typedef struct
{
    int fix_addr; //fix to be appied at this address
    int func_index; //function whose address to be put
} struct_fix;

enum {
    APPERR=+1000000000,
    SYSERR=-1000000000,

    SEUNKNOWN=-1000000000,
    SEBADIN=-1000000001,
    SENOMEM=-1000000002,
};
