/* ast.h — definição da árvore sintática abstrata (AST) */

#pragma once  /* incluir esse arquivo no maximo uma vez por compilacao */

/* O "tipo" de cada nó: diz o que aquele nó representa. */
typedef enum {
    ND_NUM,        /* literal inteiro ............. 3          */
    ND_VAR,        /* uso de uma variável ......... x          */
    ND_BINOP,      /* operação binária ............ a + b      */
    ND_ASSIGN,     /* atribuição a variável ....... x = expr   */
    ND_PRINT,      /* comando print ............... print e    */
    ND_DECL,       /* declaração de inteiro ....... int x      */
    ND_IF,         /* if (cond) corpo [else corpo]             */
    ND_WHILE,      /* while (cond) corpo                       */
    ND_INDEX,      /* leitura de elemento ......... v[i]       */
    ND_ASSIGN_IDX, /* escrita em elemento ......... v[i] = e   */
    ND_DECL_ARR,    /* declaração de vetor ......... int v[10]  */
    ND_BREAK,      /* break;    — sai do laço                  */
    ND_CONTINUE    /* continue; — volta à condição do laço     */
} NodeKind;

/* Qual operador, para os nós ND_BINOP. */
typedef enum {
    OP_ADD,   /* +  */
    OP_SUB,   /* -  */
    OP_MUL,   /* *  */
    OP_DIV,   /* /  */
    OP_MOD,   /* %  */
    OP_EQ,    /* == */
    OP_NE,    /* != */
    OP_LT,    /* <  */
    OP_LE,    /* <= */
    OP_GT,    /* >  */
    OP_GE     /* >= */
} OpKind;

/* O nó da AST. Um único struct atende a todos os kinds:
 * cada kind usa só os campos que fazem sentido para ele. */
typedef struct Node {
    NodeKind kind;         /* qual dos tipos acima este nó é                 */
    int      line;         /* linha no código-fonte (para mensagens de erro) */
    int      num;          /* NUM=valor do literal · DECL_ARR=tamanho        */
    char    *name;         /* nome da variável/vetor (VAR, ASSIGN, DECL,
                              INDEX, ASSIGN_IDX, DECL_ARR)                    */
    OpKind   op;           /* BINOP: qual operador                           */

    struct Node *left;     /* sub-árvore 1, conforme o kind:
                              BINOP=operando esq. · ASSIGN/PRINT=expressão
                              IF/WHILE=condição · INDEX/ASSIGN_IDX=índice     */
    struct Node *right;    /* sub-árvore 2, conforme o kind:
                              BINOP=operando dir. · IF/WHILE=corpo
                              ASSIGN_IDX=valor                                */
    struct Node *third;    /* sub-árvore 3: só o IF usa (corpo do else)      */

    struct Node *next;     /* próximo comando na sequência do programa       */
} Node;

/* Construtores — criam um nó já preenchido e devolvem o ponteiro. */
Node *makeNum      (int value);
Node *makeVar      (char *name);
Node *makeBinOp    (OpKind op, Node *left, Node *right);
Node *makeAssign   (char *name, Node *expr);
Node *makePrint    (Node *expr);
Node *makeDecl     (char *name);
Node *makeIf       (Node *cond, Node *thenBody, Node *elseBody);
Node *makeWhile    (Node *cond, Node *body);
Node *makeIndex    (char *name, Node *index);             /* v[i]      */
Node *makeAssignIdx(char *name, Node *index, Node *value); /* v[i] = e */
Node *makeDeclArr  (char *name, int size);                /* int v[10] */
Node *makeBreak    (void);
Node *makeContinue (void);