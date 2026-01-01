#include <stdio.h> //for fopen, printf
#include <stdlib.h> //for malloc, exit

#include "types.h"
#include "lex.h"
#include "parse.h"
#include "emit.h"
#include "vm.h"

extern char mem[MEM_SIZE]; //vm.c
extern int data_top_empty; //vm.c

int main(int argc, char*argv[])
{
    FILE *fp;
    char *buffer;
    int file_size;

    FILE *mm_fp;
    int mm_file_size;

    if (argc == 1)
    {
        printf("No input .gwd file.\n");
        printf("\n");
        printf("Usage: goodc.exe <sourcefile>.gwd [<mm>.gwd]\n");
        printf("If you use new and free in <sourcefile>.gwd,\n");
        printf("then supply your memory manager file <mm>.gwd\n");
        exit(1);
    }
    else if(argc == 2)
    {
        fp = fopen(argv[1], "r");
        if (fp == NULL)
        {
            printf("Unable to open file.\n");
            exit(1);
        }
        else
        {
            //read to a buffer

            // Seek to the end of the file to get the size
            fseek(fp, 0, SEEK_END);
            file_size = ftell(fp);
            rewind(fp); // Go back to the beginning

            // Allocate memory for the buffer
            buffer = (char *)malloc(file_size + 1);
            if (buffer == NULL)
            {
                printf("Memory allocation failed.\n");
                fclose(fp);
                exit(1);
            }

            // Read the entire file into the buffer
            size_t read_size = fread(buffer, 1, file_size, fp); //one one byte till count is file_size
            //printf("filesize %d\n", file_size);
            //printf("read_size %d\n", read_size);
            //printf("buff data %#x\n", buffer[read_size-1]);
            //printf("buff data %#x\n", buffer[read_size-2]);
            //printf("buff data %#x\n", buffer[read_size-3]);

            buffer[read_size] = '\n'; // add newline at the end

            lex_init(buffer, read_size+1);
            parse_init();
            program();

            ir_emit_outfile("out.ir.txt");
            if(DEBUG)
            {
                print_types();
                print_symbols();
                print_funcs();
                print_musts();
                print_text();
                print_data();
            }
            if(INTERPRET)
                run_vm();

            free(buffer);
            fclose(fp);
        }
    }
    else if(argc == 3)
    {
        mm_fp = fopen(argv[2], "r");
        if (mm_fp == NULL)
        {
            printf("Unable to open .gwd memory manager file.\n");
            exit(1);
        }
        else
        {
            // Seek to the end of the file to get the size
            fseek(mm_fp, 0, SEEK_END);
            mm_file_size = ftell(mm_fp);
            rewind(mm_fp); // Go back to the beginning

            fp = fopen(argv[1], "r");
            if (fp == NULL)
            {
                printf("Unable to open .gwd source file.\n");
                fclose(mm_fp);
                exit(1);
            }
            else
            {
                //read to a buffer

                // Seek to the end of the file to get the size
                fseek(fp, 0, SEEK_END);
                file_size = ftell(fp);
                rewind(fp); // Go back to the beginning

                // Allocate memory for the buffer
                buffer = (char *)malloc(mm_file_size + file_size + 1);
                if (buffer == NULL)
                {
                    printf("Memory allocation failed.\n");
                    fclose(fp);
                    exit(1);
                }

                // Read the entire memory manager file into the buffer
                size_t read_size = fread(buffer, 1, mm_file_size, mm_fp); //one one byte till count is mm_file_size

                // Read the entire source file into the buffer
                read_size += fread(buffer+mm_file_size, 1, file_size, fp); //one one byte till count is file_size
                //printf("filesize %d\n", file_size);
                //printf("read_size %d\n", read_size);
                //printf("buff data %#x\n", buffer[read_size-1]);
                //printf("buff data %#x\n", buffer[read_size-2]);
                //printf("buff data %#x\n", buffer[read_size-3]);

                buffer[read_size] = '\n'; // add newline at the end

                lex_init(buffer, read_size+1);
                parse_init();
                program();

                //to set heap_base
                //first align to 64 block size
                int blocks = (data_top_empty-16000) / 64;
                //printf("data_top_empty = %d\n", data_top_empty);
                //printf("blocks = %d\n", blocks);
                if((blocks*64)+16000 != data_top_empty)
                {
                    blocks = blocks + 1;
                }
                //printf("now blocks = %d\n", blocks);
                int heap_base = (blocks*64)+16000;
                //printf("heap_base = %d\n", heap_base);

                //set heap_base
                //memory manager's int heap_base is at addr 16000
                *((int*)&mem[TEXT_SIZE]) = heap_base;

                //bitmap index starts from 0 to 374, total 375
                //to cover 16000 to 40000, total 24000 bytes
                //using block size 64. 64*375=24000
                //index 0 means 16000-16064, 1 means 16064-16128, and so on
                //our global data takes memory from 16000 to data_top_empty
                //so lets find our first index which is free
                int start_index = (heap_base-16000)/64;
                //printf("start_index = %d\n", start_index);

                //set heap_bitmap
                //memory manager's int heap_base is at addr 16000
                //its char bitmap[375] is at addr 16004. 
                //and we know we will not touch some of index
                //of bitmap, because global data takes some memory
                //from 16000 onwards, and our bitmap index 0 also means 16000-16064
                //so we take care of this overlap
                for(int i=TEXT_SIZE+4+start_index; i < TEXT_SIZE+4+375; i++)
                {
                    if(i == TEXT_SIZE+4+start_index)
                        mem[i] = 'f';
                    else
                        mem[i] = 'g';
                }

                ir_emit_outfile("out.ir.txt");
                if(DEBUG)
                {
                    print_types();
                    print_symbols();
                    print_funcs();
                    print_musts();
                    print_text();
                    print_data();
                }
                if(INTERPRET)
                    run_vm();

                free(buffer);
                fclose(fp);
            }

            fclose(mm_fp);
        }
    }

    return 0;
}

