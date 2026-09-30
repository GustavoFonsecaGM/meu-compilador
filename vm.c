/* vm.c — máquina virtual de pilha que executa o bytecode */
#include <stdio.h>
#include <stdlib.h>
#include "codegen.h"   /* Instr, OpCode, getCode(), getCodeSize() */

#define STACK_MAX 1024
#define DATA_MAX  10000

static int stack[STACK_MAX];
static int sp;                 /* stack pointer: aponta para a PRÓXIMA posição livre */

static int data[DATA_MAX];     /* memória de dados: indexada por endereço (da tabela) */

/* ---------- operações de pilha, com verificação ---------- */
static void push(int v) {
    if (sp >= STACK_MAX) { fprintf(stderr, "Erro VM: estouro de pilha\n"); exit(1); }
    stack[sp++] = v;
}
static int pop(void) {
    if (sp <= 0) { fprintf(stderr, "Erro VM: pilha vazia\n"); exit(1); }
    return stack[--sp];
}

void runVM(void) {
    Instr *code = getCode();
    int    n    = getCodeSize();

    sp = 0;                    /* pilha começa vazia               */
    int ip = 0;                /* começa na primeira instrução     */

    for (int i = 0; i < DATA_MAX; i++) data[i] = 0;   /* zera a memória de dados */

    while (ip < n) {
        Instr instr = code[ip];

        switch (instr.op) {

        case VM_PUSH:  push(instr.arg);                break;
        case VM_LOAD:  push(data[instr.arg]);          break;
        case VM_STORE: data[instr.arg] = pop();        break;

        /* aritméticos: tira dois (b no topo, a embaixo), devolve a OP b */
        case VM_ADD: { int b = pop(), a = pop(); push(a + b); break; }
        case VM_SUB: { int b = pop(), a = pop(); push(a - b); break; }
        case VM_MUL: { int b = pop(), a = pop(); push(a * b); break; }
        case VM_DIV: {
            int b = pop(), a = pop();
            if (b == 0) { fprintf(stderr, "Erro VM: divisao por zero\n"); exit(1); }
            push(a / b);
            break;
        }
        case VM_MOD: {
            int b = pop(), a = pop();
            if (b == 0) { fprintf(stderr, "Erro VM: modulo por zero\n"); exit(1); }
            push(a % b);
            break;
        }

        /* relacionais: devolvem 1 (verdadeiro) ou 0 (falso) */
        case VM_LT: { int b = pop(), a = pop(); push(a <  b); break; }
        case VM_LE: { int b = pop(), a = pop(); push(a <= b); break; }
        case VM_GT: { int b = pop(), a = pop(); push(a >  b); break; }
        case VM_GE: { int b = pop(), a = pop(); push(a >= b); break; }
        case VM_EQ: { int b = pop(), a = pop(); push(a == b); break; }
        case VM_NE: { int b = pop(), a = pop(); push(a != b); break; }

        /* vetores: o índice está na pilha; o operando é o endereço-base */
        case VM_LOAD_IDX: {
            int i = pop();
            push(data[instr.arg + i]);
            break;
        }
        case VM_STORE_IDX: {
            int v = pop();               /* valor (empilhado por último) */
            int i = pop();               /* índice                       */
            data[instr.arg + i] = v;
            break;
        }

        /* saltos: escrevem diretamente no ip */
        case VM_JMP:
            ip = instr.arg;
            continue;                    /* NÃO faz o ip++ lá embaixo */
        case VM_JZ:
            if (pop() == 0) { ip = instr.arg; continue; }
            break;                       /* condição não-zero: segue em frente */

        case VM_PRINT: printf("%d\n", pop()); break;

        case VM_HALT:  return;           /* fim da execução */

        default:
            fprintf(stderr, "Erro VM: opcode desconhecido %d\n", instr.op);
            exit(1);
        }

        ip++;                            /* avança para a próxima instrução */
    }
}