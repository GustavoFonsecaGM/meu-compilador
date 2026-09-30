/* sym.c — tabela de símbolos (implementação) */
#include <stdlib.h>   /* calloc, free */
#include <string.h>   /* strcmp       */
#include "sym.h"

static Symbol *table = NULL;   /* lista encadeada de símbolos                */
static int nextAddress = 0;    /* próximo endereço livre na memória de dados */

/* Esvaziar tabela */
void symInit(void) {
    Symbol *p = table;
    while (p != NULL) {
        Symbol *tmp = p;
        p = p->next;
        free(tmp);             /* libera só o nó (o 'name' pertence ao AST) */
    }
    table = NULL;
    nextAddress = 0;
}

/* busca; NULL se não achar       */
Symbol *symLookup(const char *name) {
    for (Symbol *p = table; p != NULL; p = p->next) {
        if (strcmp(p->name, name) == 0) return p;   /* compara o TEXTO */
    }
    return NULL;
}

/* cria e insere um símbolo, reservando 'size' endereços consecutivos */
static Symbol *insert(const char *name, SymKind kind, int size) {
    Symbol *s = calloc(1, sizeof(Symbol));
    s->name    = name;          /* aponta para o nome já existente (não copia) */
    s->kind    = kind;
    s->size    = size;
    s->address = nextAddress;   /* endereço-base = início do bloco livre        */
    nextAddress += size;        /* reserva 'size' posições                      */
    s->next    = table;         /* insere no início da lista                    */
    table = s;
    return s;
}

Symbol *symDeclareScalar(const char *name) {
    if (symLookup(name) != NULL) return NULL;   /* já declarada → recusa */
    return insert(name, SYM_SCALAR, 1);
}

Symbol *symDeclareArray(const char *name, int size) {
    if (symLookup(name) != NULL) return NULL;   /* já declarada → recusa */
    return insert(name, SYM_ARRAY, size);
}

int symDataSize(void) {
    return nextAddress;
}

/* Descobre qual variável ocupa um dado endereço.
 * Para um escalar, é o próprio endereço; para um vetor v de tamanho N em base B,
 * qualquer endereço de B até B+N-1 pertence a v. */
Symbol *symFindByAddress(int address) {
    for (Symbol *p = table; p != NULL; p = p->next) {
        if (address >= p->address && address < p->address + p->size)
            return p;
    }
    return NULL;
}