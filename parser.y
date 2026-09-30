%code requires {
    #include "ast.h"
}

%{
#include "ast.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include "vm.h"

Node *ast_root = NULL;        /* raiz da AST, preenchida ao fim da análise  */

int  yylex(void);             /* função do scanner (lex.yy.c)               */
void yyerror(const char *s);  /* chamada pelo Bison quando há erro           */
extern int yylineno;          /* linha atual, mantida pelo Flex              */
%}

%union {
    int   ival;   /* valor de um NUM              */
    char *sval;   /* texto de um ID               */
    Node *node;   /* um nó (ou sub-árvore) da AST  */
}

/* tokens que carregam valor */
%token <ival> NUM
%token <sval> ID

/* tokens sem valor (palavras-chave e relacionais de 2 caracteres) */
%token INT PRINT IF ELSE WHILE BREAK CONTINUE
%token EQ NE LE GE

/* não-terminais que produzem nós da AST */
%type <node> stmt_list stmt block expr

/* Precedência e associatividade — da MAIS BAIXA (topo) à MAIS ALTA (base).
   É esta seção que desfaz a ambiguidade das expressões, sem reescrever regra. */
%left EQ NE
%left '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'
%right UMINUS        /* menos unário: a precedência mais alta de todas */

%start program

%%

program
    : stmt_list                        { ast_root = $1; }
    ;

stmt_list
    : /* vazio */                      { $$ = NULL; }
    | stmt stmt_list                   { $1->next = $2; $$ = $1; }
    ;

stmt
    : INT ID ';'                       { $$ = makeDecl($2); }
    | INT ID '[' NUM ']' ';'           { $$ = makeDeclArr($2, $4); }
    | ID '=' expr ';'                  { $$ = makeAssign($1, $3); }
    | ID '[' expr ']' '=' expr ';'     { $$ = makeAssignIdx($1, $3, $6); }
    | PRINT expr ';'                   { $$ = makePrint($2); }
    | IF '(' expr ')' block            { $$ = makeIf($3, $5, NULL); }
    | IF '(' expr ')' block ELSE block { $$ = makeIf($3, $5, $7); }
    | WHILE '(' expr ')' block         { $$ = makeWhile($3, $5); }
    | BREAK ';'                        { $$ = makeBreak(); }
    | CONTINUE ';'                     { $$ = makeContinue(); }
    ;

block
    : '{' stmt_list '}'                { $$ = $2; }
    ;

expr
    : NUM                              { $$ = makeNum($1); }
    | ID                               { $$ = makeVar($1); }
    | ID '[' expr ']'                  { $$ = makeIndex($1, $3); }
    | expr '+' expr                    { $$ = makeBinOp(OP_ADD, $1, $3); }
    | expr '-' expr                    { $$ = makeBinOp(OP_SUB, $1, $3); }
    | expr '*' expr                    { $$ = makeBinOp(OP_MUL, $1, $3); }
    | expr '/' expr                    { $$ = makeBinOp(OP_DIV, $1, $3); }
    | expr '%' expr                    { $$ = makeBinOp(OP_MOD, $1, $3); }
    | expr EQ expr                     { $$ = makeBinOp(OP_EQ, $1, $3); }
    | expr NE expr                     { $$ = makeBinOp(OP_NE, $1, $3); }
    | expr '<' expr                    { $$ = makeBinOp(OP_LT, $1, $3); }
    | expr LE expr                     { $$ = makeBinOp(OP_LE, $1, $3); }
    | expr '>' expr                    { $$ = makeBinOp(OP_GT, $1, $3); }
    | expr GE expr                     { $$ = makeBinOp(OP_GE, $1, $3); }
    | '-' expr %prec UMINUS            { $$ = makeBinOp(OP_SUB, makeNum(0), $2); }
    | '(' expr ')'                     { $$ = $2; }
    ;

%%

/* Funções auxiliares para manipulação da AST */

static const char *opName(OpKind op) {
    switch (op) {
        case OP_ADD: return "+";   case OP_SUB: return "-";
        case OP_MUL: return "*";   case OP_DIV: return "/";
        case OP_MOD: return "%";   case OP_EQ:  return "==";
        case OP_NE:  return "!=";  case OP_LT:  return "<";
        case OP_LE:  return "<=";  case OP_GT:  return ">";
        case OP_GE:  return ">=";
    }
    return "?";
}

static void indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

/* percorre a AST imprimindo cada nó com recuo conforme a profundidade */
static void printNode(Node *n, int depth) {
    if (n == NULL) return;
    indent(depth);

    switch (n->kind) {
        case ND_NUM:      printf("NUM %d\n", n->num);                   break;
        case ND_VAR:      printf("VAR %s\n", n->name);                  break;
        case ND_DECL:     printf("DECL %s\n", n->name);                 break;
        case ND_DECL_ARR: printf("DECL_ARR %s[%d]\n", n->name, n->num); break;
        case ND_BREAK:    printf("BREAK\n");                            break;
        case ND_CONTINUE: printf("CONTINUE\n");                         break;

        case ND_BINOP:
            printf("BINOP %s\n", opName(n->op));
            printNode(n->left,  depth + 1);
            printNode(n->right, depth + 1);
            break;

        case ND_ASSIGN:
            printf("ASSIGN %s\n", n->name);
            printNode(n->left, depth + 1);       /* expressão atribuída */
            break;

        case ND_PRINT:
            printf("PRINT\n");
            printNode(n->left, depth + 1);
            break;

        case ND_INDEX:
            printf("INDEX %s\n", n->name);
            printNode(n->left, depth + 1);        /* índice */
            break;

        case ND_ASSIGN_IDX:
            printf("ASSIGN_IDX %s\n", n->name);
            printNode(n->left,  depth + 1);       /* índice */
            printNode(n->right, depth + 1);       /* valor  */
            break;

        case ND_IF:
            printf("IF\n");
            indent(depth + 1); printf("cond:\n"); printNode(n->left,  depth + 2);
            indent(depth + 1); printf("then:\n"); printNode(n->right, depth + 2);
            if (n->third) {
                indent(depth + 1); printf("else:\n"); printNode(n->third, depth + 2);
            }
            break;

        case ND_WHILE:
            printf("WHILE\n");
            indent(depth + 1); printf("cond:\n"); printNode(n->left,  depth + 2);
            indent(depth + 1); printf("body:\n"); printNode(n->right, depth + 2);
            break;
    }

    printNode(n->next, depth);   /* próximo comando da sequência, no mesmo nível */
}

void yyerror(const char *s) {
    fprintf(stderr, "Erro de sintaxe: %s (linha %d)\n", s, yylineno);
}

int main(void) {
    if (yyparse() != 0) return 1;          /* erro de sintaxe: para aqui */

    printNode(ast_root, 0);  

    genProgram(ast_root);                   /* gera o bytecode e checa a semântica */
    if (hadSemanticError()) return 1;       /* houve erro semântico: não emite bytecode */

    printf("\n=== BYTECODE ===\n");
    dumpCode();

    printf("\n=== EXECUCAO ===\n");
    runVM();                     /* executa! */

    return 0;
}