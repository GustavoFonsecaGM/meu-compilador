/* ast.c — construtores da AST */
#include <stdlib.h>   /* calloc */
#include "ast.h"

extern int yylineno;   /* mantido pelo Flex; marca a linha de cada nó */

/* Aloca um nó zerado com o kind indicado.
 * calloc zera tudo, então todo campo não usado já começa em 0/NULL. */
static Node *newNode(NodeKind kind) {
    Node *n = calloc(1, sizeof(Node));
    n->kind = kind;
    n->line = yylineno;    /* carimba a linha atual no momento em que o nó é criado */
    return n;
}

Node *makeNum(int value) {
    Node *n = newNode(ND_NUM);
    n->num = value;
    return n;
}

Node *makeVar(char *name) {
    Node *n = newNode(ND_VAR);
    n->name = name;
    return n;
}

Node *makeBinOp(OpKind op, Node *left, Node *right) {
    Node *n = newNode(ND_BINOP);
    n->op    = op;
    n->left  = left;
    n->right = right;
    return n;
}

Node *makeAssign(char *name, Node *expr) {
    Node *n = newNode(ND_ASSIGN);
    n->name = name;
    n->left = expr;
    return n;
}

Node *makePrint(Node *expr) {
    Node *n = newNode(ND_PRINT);
    n->left = expr;
    return n;
}

Node *makeDecl(char *name) {
    Node *n = newNode(ND_DECL);
    n->name = name;
    return n;
}


Node *makeIf(Node *cond, Node *thenBody, Node *elseBody) {
    Node *n = newNode(ND_IF);
    n->left  = cond;        /* condição                        */
    n->right = thenBody;    /* corpo executado se verdadeiro   */
    n->third = elseBody;    /* corpo do else (NULL se não há)  */
    return n;
}

Node *makeWhile(Node *cond, Node *body) {
    Node *n = newNode(ND_WHILE);
    n->left  = cond;        /* condição       */
    n->right = body;        /* corpo do laço  */
    return n;
}

Node *makeIndex(char *name, Node *index) {
    Node *n = newNode(ND_INDEX);
    n->name = name;         /* nome do vetor        */
    n->left = index;        /* expressão do índice  */
    return n;
}

Node *makeAssignIdx(char *name, Node *index, Node *value) {
    Node *n = newNode(ND_ASSIGN_IDX);
    n->name  = name;        /* nome do vetor        */
    n->left  = index;       /* expressão do índice  */
    n->right = value;       /* valor a armazenar    */
    return n;
}

Node *makeDeclArr(char *name, int size) {
    Node *n = newNode(ND_DECL_ARR);
    n->name = name;         /* nome do vetor    */
    n->num  = size;         /* tamanho          */
    return n;
}

Node *makeBreak(void) {
    Node *n = newNode(ND_BREAK);
    return n;
}

Node *makeContinue(void) {
    Node *n = newNode(ND_CONTINUE);
    return n;
}