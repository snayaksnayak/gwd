#include <stdio.h> //for fopen, printf
#include <stdlib.h> //for malloc, exit

#include "types.h"
#include "emit.h"

//C code emiting variables and functions
char header_buffer[10000];  // allocate a large enough buffer
int cur_header_len = 0;     // keeps track of the current length
char code_buffer[50000];  // allocate a large enough buffer
int cur_code_len = 0;     // keeps track of the current length

void c_emit_header(char* text)
{
    //go on appending to header_buffer
    cur_header_len += snprintf(header_buffer + cur_header_len,
                            sizeof(header_buffer) - cur_header_len, text);
}

void c_emit_header_line(char* text)
{
    //go on appending to header_buffer
    cur_header_len += snprintf(header_buffer + cur_header_len,
                            sizeof(header_buffer) - cur_header_len, text);
    cur_header_len += snprintf(header_buffer + cur_header_len,
                            sizeof(header_buffer) - cur_header_len, "\n");
}

void c_emit_code(char* text)
{
    //go on appending to code_buffer
    cur_code_len += snprintf(code_buffer + cur_code_len,
                            sizeof(code_buffer) - cur_code_len, text);
}

void c_emit_code_line(char* text)
{
    //go on appending to code_buffer
    cur_code_len += snprintf(code_buffer + cur_code_len,
                            sizeof(code_buffer) - cur_code_len, text);
    cur_code_len += snprintf(code_buffer + cur_code_len,
                            sizeof(code_buffer) - cur_code_len, "\n");
}

void c_emit_outfile(char* outfile)
{
    FILE* fp;
    fp = fopen(outfile, "w");
    if (fp == NULL)
    {
        printf("Unable to open output file.\n");
        exit(1);
    }
    else
    {
        //write c to file
        header_buffer[cur_header_len]='\0';
        code_buffer[cur_code_len]='\0';
        fprintf(fp, "%s", header_buffer);
        fprintf(fp, "%s", code_buffer);
        fclose(fp);
    }
}

//IR code emiting variables and functions
char ir_buffer[50000];  // allocate a large enough buffer
int cur_ir_len = 0;     // keeps track of the current length

void ir_emit_code_line(char* text)
{
    //go on appending to ir_buffer
    cur_ir_len += snprintf(ir_buffer + cur_ir_len,
                            sizeof(ir_buffer) - cur_ir_len, text);
    cur_ir_len += snprintf(ir_buffer + cur_ir_len,
                            sizeof(ir_buffer) - cur_ir_len, "\n");
    if(DEBUG)
    {
        printf("%s\n", text);
    }
}

void ir_emit_outfile(char* outfile)
{
    FILE* fp;
    fp = fopen(outfile, "w");
    if (fp == NULL)
    {
        printf("Unable to open output file.\n");
        exit(1);
    }
    else
    {
        //write ir to file
        ir_buffer[cur_ir_len]='\0';
        fprintf(fp, "%s", ir_buffer);
        fclose(fp);
    }
}
