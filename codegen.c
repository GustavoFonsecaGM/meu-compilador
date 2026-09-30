/* codegen.c — gerador de bytecode a partir da AST */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ast.h"
#include "sym.h"
#include "codegen.h"

/* ---------- vetor de instruções ---------- */
#define MAX_CODE 10000
static Instr code[MAX_CODE];
static int   codeSize = 0;

/* Emite uma instrução e DEVOLVE seu índice  */
static int emit(OpCode op, int arg) {
    if (codeSize >= MAX_CODE) {
        fprintf(stderr, "Erro: programa excede %d instrucoes\n", MAX_CODE);
        exit(1);
    }
    code[codeSize].op  = op;
    code[codeSize].arg = arg;
    return codeSize++;
}

/* Corrige o operando de uma instrução já emitida (preenche o alvo de um salto). */
static void backpatch(int at, int target) {
    code[at].arg = target;
}

/* ---------- erros semânticos ---------- */
static int semErrors = 0;

static void semError(int line, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "Erro semantico (linha %d): ", line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    semErrors++;
}

int hadSemanticError(void) { return semErrors != 0; }

/* ---------- pilha de laços (para break/continue) ---------- */
#define MAX_LOOP  64
#define MAX_BREAK 64

typedef struct {
    int continueTarget;          /* índice do topo do laço (destino do continue) */
    int breakSites[MAX_BREAK];   /* índices dos JMP de break, a corrigir          */
    int breakCount;
} Loop;

static Loop loopStack[MAX_LOOP];
static int  loopDepth = 0;

static void pushLoop(int continueTarget) {
    loopStack[loopDepth].continueTarget = continueTarget;
    loopStack[loopDepth].breakCount     = 0;
    loopDepth++;
}
static void popLoop(void) { loopDepth--; }

static void patchBreaks(int target) {          /* corrige todos os break do laço atual */
    Loop *L = &loopStack[loopDepth - 1];
    for (int i = 0; i < L->breakCount; i++) backpatch(L->breakSites[i], target);
}

/* ---------- resolução de variáveis (com checagem semântica) ---------- */
static Symbol *resolveScalar(Node *n) {
    Symbol *s = symLookup(n->name);
    if (s == NULL)             { semError(n->line, "variavel '%s' nao declarada", n->name); return NULL; }
    if (s->kind != SYM_SCALAR) { semError(n->line, "'%s' e vetor, uso invalido como escalar", n->name); return NULL; }
    return s;
}
static Symbol *resolveArray(Node *n) {
    Symbol *s = symLookup(n->name);
    if (s == NULL)            { semError(n->line, "variavel '%s' nao declarada", n->name); return NULL; }
    if (s->kind != SYM_ARRAY) { semError(n->line, "'%s' e escalar, nao pode ser indexado", n->name); return NULL; }
    return s;
}

/* mapeia o operador da AST para o opcode da VM */
static OpCode binToVM(OpKind op) {
    switch (op) {
    case OP_ADD: return VM_ADD;   case OP_SUB: return VM_SUB;
    case OP_MUL: return VM_MUL;   case OP_DIV: return VM_DIV;
    case OP_MOD: return VM_MOD;   case OP_EQ:  return VM_EQ;
    case OP_NE:  return VM_NE;    case OP_LT:  return VM_LT;
    case OP_LE:  return VM_LE;    case OP_GT:  return VM_GT;
    case OP_GE:  return VM_GE;
    }
    return VM_HALT;   /* inalcançável */
}

/* ---------- percurso da AST ---------- */
static void genExpr(Node *n);
static void genStmt(Node *n);
static void genStmtList(Node *n);

static void genExpr(Node *n) {
    switch (n->kind) {
    case ND_NUM:
        emit(VM_PUSH, n->num);
        break;
    case ND_VAR: {
        Symbol *s = resolveScalar(n);
        emit(VM_LOAD, s ? s->address : 0);
        break;
    }
    case ND_INDEX: {                          /* v[i] : lê um elemento */
        Symbol *s = resolveArray(n);
        genExpr(n->left);                     /* índice → pilha */
        emit(VM_LOAD_IDX, s ? s->address : 0);
        break;
    }
    case ND_BINOP:
        genExpr(n->left);                     /* operando esquerdo primeiro */
        genExpr(n->right);                    /* depois o direito           */
        emit(binToVM(n->op), 0);
        break;
    default:
        semError(n->line, "expressao invalida");
        break;
    }
}

static void genIf(Node *n) {
    genExpr(n->left);                         /* condição → pilha */
    int jzElse = emit(VM_JZ, -1);             /* se falso, pula o 'then' (alvo a corrigir) */
    genStmtList(n->right);                    /* corpo do 'then' */

    if (n->third == NULL) {                   /* sem else */
        backpatch(jzElse, codeSize);          /* JZ cai logo após o 'then' */
    } else {                                  /* com else */
        int jmpEnd = emit(VM_JMP, -1);        /* fim do 'then' pula o 'else' */
        backpatch(jzElse, codeSize);          /* JZ cai no início do 'else' */
        genStmtList(n->third);                /* corpo do 'else' */
        backpatch(jmpEnd, codeSize);          /* JMP cai após o 'else' */
    }
}

static void genWhile(Node *n) {
    int top = codeSize;                       /* topo: destino do continue e do JMP final */
    genExpr(n->left);                         /* condição → pilha */
    int jzEnd = emit(VM_JZ, -1);              /* se falso, sai do laço (alvo a corrigir) */

    pushLoop(top);                            /* abre contexto do laço */
    genStmtList(n->right);                    /* corpo */
    emit(VM_JMP, top);                        /* volta reavaliar a condição */
    int end = codeSize;
    backpatch(jzEnd, end);                    /* condição falsa cai aqui */
    patchBreaks(end);                         /* todos os break saltam para cá */
    popLoop();
}

static void genStmt(Node *n) {
    switch (n->kind) {
    case ND_DECL:
        if (symDeclareScalar(n->name) == NULL)
            semError(n->line, "variavel '%s' ja declarada", n->name);
        break;                                /* declaração de escalar não emite bytecode */
    case ND_DECL_ARR:
        if (symDeclareArray(n->name, n->num) == NULL)
            semError(n->line, "variavel '%s' ja declarada", n->name);
        break;                                /* declaração de vetor só reserva espaço na tabela */
    case ND_ASSIGN: {
        Symbol *s = resolveScalar(n);
        genExpr(n->left);                     /* valor → pilha */
        emit(VM_STORE, s ? s->address : 0);
        break;
    }
    case ND_ASSIGN_IDX: {                      /* v[i] = e */
        Symbol *s = resolveArray(n);
        genExpr(n->left);                     /* índice → pilha */
        genExpr(n->right);                    /* valor  → pilha */
        emit(VM_STORE_IDX, s ? s->address : 0);
        break;
    }
    case ND_PRINT:
        genExpr(n->left);
        emit(VM_PRINT, 0);
        break;
    case ND_IF:
        genIf(n);
        break;
    case ND_WHILE:
        genWhile(n);
        break;
    case ND_BREAK: {
        if (loopDepth == 0) { semError(n->line, "'break' fora de laco"); break; }
        int site = emit(VM_JMP, -1);          /* alvo (fim do laço) a corrigir */
        Loop *L = &loopStack[loopDepth - 1];
        L->breakSites[L->breakCount++] = site;
        break;
    }
    case ND_CONTINUE:
        if (loopDepth == 0) { semError(n->line, "'continue' fora de laco"); break; }
        emit(VM_JMP, loopStack[loopDepth - 1].continueTarget);   /* destino já conhecido */
        break;
    default:
        semError(n->line, "comando invalido");
        break;
    }
}

static void genStmtList(Node *n) {
    for (; n != NULL; n = n->next) genStmt(n);   /* percorre a sequência pelo 'next' */
}

/* ---------- entrada pública ---------- */
void genProgram(Node *root) {
    symInit();
    codeSize  = 0;
    semErrors = 0;
    loopDepth = 0;
    genStmtList(root);
    emit(VM_HALT, 0);
}

/* ---------- desassembler (apenas para conferência) ---------- */
static const char *opcodeName(OpCode op) {
    switch (op) {
    case VM_PUSH: return "PUSH";  case VM_LOAD: return "LOAD";  case VM_STORE: return "STORE";
    case VM_ADD:  return "ADD";   case VM_SUB:  return "SUB";   case VM_MUL:   return "MUL";
    case VM_DIV:  return "DIV";   case VM_MOD:  return "MOD";
    case VM_LT:   return "LT";    case VM_LE:   return "LE";    case VM_GT:    return "GT";
    case VM_GE:   return "GE";    case VM_EQ:   return "EQ";    case VM_NE:    return "NE";
    case VM_LOAD_IDX: return "LOAD_IDX";  case VM_STORE_IDX: return "STORE_IDX";
    case VM_JMP:  return "JMP";   case VM_JZ:   return "JZ";
    case VM_PRINT: return "PRINT"; case VM_HALT: return "HALT";
    }
    return "???";
}
static int hasArg(OpCode op) {
    switch (op) {
    case VM_PUSH: case VM_LOAD: case VM_STORE:
    case VM_LOAD_IDX: case VM_STORE_IDX:
    case VM_JMP: case VM_JZ:
        return 1;
    default:
        return 0;
    }
}

void dumpCode(void) {
    for (int i = 0; i < codeSize; i++) {
        OpCode op = code[i].op;

        if (hasArg(op)) {
            printf("%3d: %-9s %d", i, opcodeName(op), code[i].arg);

            /* para instruções que endereçam memória, anota a variável ao lado */
            if (op == VM_LOAD || op == VM_STORE ||
                op == VM_LOAD_IDX || op == VM_STORE_IDX) {
                Symbol *s = symFindByAddress(code[i].arg);
                if (s != NULL) printf("   ; %s", s->name);
            }
            printf("\n");
        } else {
            printf("%3d: %s\n", i, opcodeName(op));
        }
    }
}

/* ---------- acesso para a VM (Passo D) ---------- */
Instr *getCode(void)     { return code; }
int    getCodeSize(void) { return codeSize; }