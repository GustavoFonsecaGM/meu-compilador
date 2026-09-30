# Compilador e Máquina Virtual de Pilha

Compilador para uma linguagem imperativa própria, desenvolvido em **C** com **Flex** e **Bison**. O código-fonte é traduzido em uma árvore sintática abstrata (AST), depois em **bytecode**, que é executado por uma **máquina virtual de pilha** — o mesmo modelo usado pela JVM e pelo Python.

## Pipeline

```
código-fonte → [scanner] → tokens → [parser] → AST → [gerador] → bytecode → [VM] → saída
               Flex                 Bison            + tabela de símbolos     máquina de pilha
```

| Etapa | Arquivo | O que faz |
|---|---|---|
| Análise léxica | `scanner.l` | Transforma o texto em tokens |
| Análise sintática | `parser.y` | Reconhece a gramática e monta a AST |
| AST | `ast.h`, `ast.c` | Estrutura da árvore e seus construtores |
| Tabela de símbolos | `sym.h`, `sym.c` | Traduz nomes de variáveis em endereços de memória |
| Geração de código | `codegen.h`, `codegen.c` | Percorre a AST, gera bytecode e faz a análise semântica |
| Máquina virtual | `vm.h`, `vm.c` | Executa o bytecode sobre uma pilha |

## A linguagem

```c
int x;                    // declaração de inteiro
int v[10];                // declaração de vetor
x = 3 + 4 * 2;            // aritmética: + - * / %
v[i] = x;                 // escrita em vetor
print x;                  // saída

if (x < 3) { ... } else { ... }   // chaves obrigatórias
while (x < 3) { ... }
break;  continue;

// comparadores: == != < <= > >=
```

## Destaques técnicos

- **Gramática sem conflitos** — a ambiguidade das expressões é resolvida por declarações de precedência e associatividade (`%left`/`%right`), e o *dangling else* é eliminado por chaves obrigatórias.
- **Menos unário** traduzido como `0 - x`, reaproveitando a subtração sem criar instrução nova.
- **Backpatching** para os saltos de `if`, `while` e `break`, gerando o código em uma única passagem.
- **Pilha de laços** para resolver `break` e `continue` corretamente em laços aninhados.
- **Análise semântica** — detecta variável não declarada, redeclaração, vetor usado como escalar (e vice-versa) e `break`/`continue` fora de laço, com o número da linha.
- **Erros em tempo de execução** — divisão por zero e estouro de pilha são tratados pela VM.
- **Desassembler** que exibe o bytecode anotado com o nome de cada variável.

## Como executar

Requer apenas o [Docker](https://www.docker.com/products/docker-desktop/), usando uma imagem que já traz Flex, Bison e GCC.

Na pasta do projeto:

**Linux / macOS**
```bash
docker run -it --rm -v "$PWD":/usr/src -w /usr/src phdcoder/flexbison bash run.sh
```

**Windows (PowerShell)**
```powershell
docker run -it --rm -v "${PWD}:/usr/src" -w /usr/src phdcoder/flexbison bash run.sh
```

**Windows (CMD)**
```cmd
docker run -it --rm -v "%cd%:/usr/src" -w /usr/src phdcoder/flexbison bash run.sh
```

O `run.sh` executa, em ordem: `flex` → `bison` → `gcc` → `./compiler < entrada`.

Para testar outro programa, edite o arquivo `entrada` e rode novamente.

## Exemplo

Entrada:

```c
int i;
i = 0;
while (i < 3) {
    print i;
    i = i + 1;
}
```

Bytecode gerado:

```
  0: PUSH      0
  1: STORE     0   ; i
  2: LOAD      0   ; i
  3: PUSH      3
  4: LT
  5: JZ        13
  6: LOAD      0   ; i
  7: PRINT
  8: LOAD      0   ; i
  9: PUSH      1
 10: ADD
 11: STORE     0   ; i
 12: JMP       2
 13: HALT
```

Saída:

```
0
1
2
```

## Limitações conhecidas

- Apenas o tipo `int` (comparações resultam em 0 ou 1).
- Sem escopo de bloco: todas as variáveis são globais.
- Vetores apenas unidimensionais, com tamanho fixo na declaração.
- Os acessos a vetor não verificam limites de índice em tempo de execução.
- O total de memória declarada não é verificado contra o limite da VM (10 000 posições).