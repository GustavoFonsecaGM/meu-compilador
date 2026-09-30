%{
#include "parser.tab.h"  
#include <stdio.h>
#include <stdlib.h>      
#include <string.h>      

char *strdup(const char *s);   /* declara strdup (some o warning no modo -std=c99) */
%}

/* Definições de opções do Flex */ 
%option noyywrap    
%option yylineno   

/* Açucar sintatico, dando nome para os tokens, para que o parser possa referenciá-los */
DIGIT   [0-9]
LETTER  [a-zA-Z_]
ID      {LETTER}({LETTER}|{DIGIT})*

%%

"//"[^\n]*        { /* [^\n] qualquer caractere que não seja quebra de linha */ }
[ \t\r]+          { /* espaços e tabulações: ignora */ }
\n                { /* quebra de linha: ignora (yylineno avança sozinho) */ }

"int"             { return INT; }
"print"           { return PRINT; }
"if"              { return IF; }
"else"            { return ELSE; }
"while"           { return WHILE; }
"break"           { return BREAK; }
"continue"        { return CONTINUE; }

"=="              { return EQ; }
"!="              { return NE; }
"<="              { return LE; }
">="              { return GE; }

{DIGIT}+          { yylval.ival = atoi(yytext); return NUM; }
{ID}              { yylval.sval = strdup(yytext); return ID;  }

"+"               { return '+'; }
"-"               { return '-'; }
"*"               { return '*'; }
"/"               { return '/'; }
"%"               { return '%'; }
"="               { return '='; }
"<"               { return '<'; }
">"               { return '>'; }
"("               { return '('; }
")"               { return ')'; }
"{"               { return '{'; }
"}"               { return '}'; }
"["               { return '['; }
"]"               { return ']'; }
";"               { return ';'; }

.                 { fprintf(stderr, "Caractere inválido '%c' na linha %d\n", yytext[0], yylineno); }

%%