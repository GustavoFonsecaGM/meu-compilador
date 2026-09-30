/* sym.h — tabela de símbolos */
#pragma once

/* Um símbolo é ou um escalar (int x) ou um vetor (int v[N]). */
typedef enum {
    SYM_SCALAR,   /* int x;     */
    SYM_ARRAY     /* int v[N];  */
} SymKind;

typedef struct Symbol {
    const char *name;      /* nome da variável (o texto pertence ao AST/scanner) */
    SymKind     kind;      /* escalar ou vetor                                   */
    int         size;      /* nº de posições: 1 (escalar) ou N (vetor)           */
    int         address;   /* endereço-base na memória de dados da VM            */
    struct Symbol *next;   /* próximo símbolo na lista                           */
} Symbol;

void    symInit(void);                                /* esvazia a tabela              */
Symbol *symLookup(const char *name);                  /* busca; NULL se não achar       */
Symbol *symDeclareScalar(const char *name);           /* declara escalar; NULL se já existe */
Symbol *symDeclareArray (const char *name, int size); /* declara vetor;   NULL se já existe */
int     symDataSize(void);                            /* total de posições reservadas   */

Symbol *symFindByAddress(int address);   /* qual símbolo cobre este endereço? (NULL se nenhum) */