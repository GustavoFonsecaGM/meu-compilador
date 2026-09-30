/* codegen.h — gerador de código: instruções e interface pública */
#pragma once
#include "ast.h"

/* Conjunto de instruções do bytecode (a máquina de pilha do Passo D executa). */
typedef enum {
    VM_PUSH,                                  /* PUSH n     : empilha a constante n            */
    VM_LOAD,                                  /* LOAD end   : empilha data[end]                */
    VM_STORE,                                 /* STORE end  : desempilha e grava em data[end]  */
    VM_ADD, VM_SUB, VM_MUL, VM_DIV, VM_MOD,   /* aritméticos                                   */
    VM_LT, VM_LE, VM_GT, VM_GE, VM_EQ, VM_NE, /* relacionais → 0 ou 1                          */
    VM_LOAD_IDX,                              /* LOAD_IDX base  : i → data[base+i]             */
    VM_STORE_IDX,                             /* STORE_IDX base : i x → grava em data[base+i]  */
    VM_JMP,                                   /* JMP end    : salta para a instrução 'end'     */
    VM_JZ,                                    /* JZ end     : desempilha c; se 0, salta        */
    VM_PRINT,                                 /* PRINT      : desempilha e imprime             */
    VM_HALT                                   /* HALT       : encerra                          */
} OpCode;

typedef struct {
    OpCode op;
    int    arg;   /* operando: constante, endereço, base ou alvo de salto (conforme o op) */
} Instr;

void   genProgram(Node *root);   /* percorre a AST e gera o bytecode             */
int    hadSemanticError(void);   /* 1 se houve erro semântico durante a geração   */
void   dumpCode(void);           /* imprime o bytecode (desassembler) — só teste  */

Instr *getCode(void);            /* vetor de instruções (para a VM)   */
int    getCodeSize(void);        /* quantidade de instruções                      */