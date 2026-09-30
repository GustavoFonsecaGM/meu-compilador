if flex scanner.l ; then
  if bison -vd parser.y ; then
    if gcc -std=c99 -Wno-implicit-function-declaration lex.yy.c parser.tab.c ast.c sym.c codegen.c vm.c -o compiler -lfl ; then
      ./compiler < entrada
    fi
  fi
fi

#-Wno-implicit-function-declaration
