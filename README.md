# Compilador Simples

Este projeto é um compilador desenvolvido para uma disciplina de Compiladores. Ele é capaz de realizar a análise léxica, sintática e semântica de um código-fonte escrito em uma linguagem estruturada simples (similar a Pascal) e gerar código de máquina para uma máquina virtual de pilha (MVD).

## Funcionalidades da Linguagem

A linguagem de programação suportada pelo compilador possui as seguintes características:

- **Estrutura Básica**: Programas são definidos dentro de um bloco `programa <nome>; ... .`
- **Declaração de Variáveis**: Variáveis são declaradas em um bloco `var` com os tipos `inteiro` ou `booleano`.
- **Sub-rotinas**: Suporte para `procedimento` (sem retorno) e `funcao` (com retorno `inteiro` ou `booleano`).
- **Comandos**:
    - Atribuição: `variavel := expressao`
    - Condicional: `se <expressao> entao <comando> [senao <comando>]`
    - Laço de Repetição: `enquanto <expressao> faca <comando>`
    - Entrada e Saída: `leia(variavel)` e `escreva(variavel_ou_funcao_inteira)`
    - Bloco de Comandos: `inicio ... fim`
- **Expressões**:
    - **Aritméticas**: `+`, `-`, `*`, `div`
    - **Relacionais**: `=`, `<>`, `<`, `>`, `<=`, `>=`
    - **Lógicas**: `e`, `ou`, `nao`
    - **Literais**: `verdadeiro`, `falso` e números inteiros.

## Estrutura do Compilador

O compilador é implementado em C e dividido nos seguintes componentes principais:

1.  **Analisador Léxico**: Responsável por ler o código-fonte e convertê-lo em uma sequência de tokens.
2.  **Analisador Sintático (Parser)**: Utiliza uma abordagem de descida recursiva para verificar se a sequência de tokens segue a gramática da linguagem. Ele também é responsável por orquestrar as outras fases.
3.  **Analisador Semântico**: Integrado ao parser, utiliza uma tabela de símbolos para verificar erros de tipo, declarações duplicadas e uso incorreto de identificadores.
4.  **Gerador de Código**: Gera instruções para uma máquina virtual de pilha (MVD) à medida que a análise sintática avança. O código gerado é salvo em um arquivo `programa.mvd`.

## Como Compilar e Executar

### Pré-requisitos

- Um compilador C (como GCC).
- `make` (opcional, mas recomendado para facilitar a compilação).

### Compilação

Para compilar o projeto, o método recomendado é usar o `make`. O `Makefile` já está configurado para compilar todos os arquivos necessários.

```bash
make
```

### Execução

### Na inaterface gráfica

Implementamos uma interface gráfica para que o usuário possa compilar o código de forma simples e rápida. Por ele é possível colar o código e baixar tanto o arquivo .mvd como o arquivo .obj

```
python3 web.py
```

### Na maquina

Para analisar um arquivo de código-fonte (por exemplo, `teste.pas`), execute o compilador passando o nome do arquivo como argumento:

```bash
./analisador teste.pas
```

Se a compilação for bem-sucedida, a mensagem "Analises concluidas com sucesso!" será exibida e um arquivo chamado `programa.mvd` será criado com o código de máquina virtual. Caso contrário, os erros encontrados serão exibidos e o arquivo `programa.mvd` será removido.

## Exemplo de Código-Fonte

Abaixo está um exemplo de um programa que pode ser compilado:

```pascal
programa Fatorial;
var
  n: inteiro;

inicio
  leia(n);
  escreva(n);
fim.
```