// simbolo.c
#include "simbolo.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Definição das variáveis globais da tabela
TipoSimbolo tabelaSimbolos[MAX_SIMBOLOS];
int topoTabela = -1; // Inicia a pilha como vazia
int nivelAtual = 0;    // Nível 0 é o escopo global

[cite_start]// Implementa a "Insere na Tabela" [cite: 1157]
void insere_tabela(const char* nome, const char* tipo) {
    if (topoTabela >= MAX_SIMBOLOS - 1) {
        printf("Erro Semantico: Tabela de simbolos cheia!\n");
        exit(1);
    }
    
    topoTabela++;
    strcpy(tabelaSimbolos[topoTabela].nome, nome);
    strcpy(tabelaSimbolos[topoTabela].tipo, tipo);
    tabelaSimbolos[topoTabela].nivel = nivelAtual;
    tabelaSimbolos[topoTabela].endereco = topoTabela; // Endereço simples baseado na pilha
}

[cite_start]// Implementa a "Coloca Tipo nas Variáveis" [cite: 1159]
void coloca_tipo_tabela(const char* tipo) {
    int i = topoTabela;
    // Itera do topo para baixo, apenas no nível atual
    while (i >= 0 && tabelaSimbolos[i].nivel == nivelAtual) {
        // Se o tipo for "variavel", atualiza
        if (strcmp(tabelaSimbolos[i].tipo, "variavel") == 0) {
            strcpy(tabelaSimbolos[i].tipo, tipo);
        }
        i--;
    }
}

[cite_start]// Implementa a "Consulta a Tabela" [cite: 1158]
[cite_start]// A busca é feita do mais recente para o mais antigo [cite: 1126]
int consulta_tabela(const char* nome) {
    int i = topoTabela;
    while (i >= 0) {
        if (strcmp(tabelaSimbolos[i].nome, nome) == 0) {
            // Encontrou o símbolo. Retorna seu índice.
            return i;
        }
        i--;
    }
    return -1; // Não encontrou
}

int consulta_duplicidade(const char* nome) {
    int i = topoTabela;
    [cite_start]// Itera do topo para baixo, *apenas no nível atual* [cite: 1758]
    while (i >= 0 && tabelaSimbolos[i].nivel == nivelAtual) {
        if (strcmp(tabelaSimbolos[i].nome, nome) == 0) {
            return 1; // Encontrou duplicata no mesmo escopo
        }
        i--;
    }
    return 0; // Não há duplicata neste escopo
}

void entra_escopo() {
    nivelAtual++;
}

void sai_escopo() {
    // "Desempilha" todos os símbolos do nível atual
    [cite_start]// A tabela funciona como uma pilha [cite: 1142]
    while (topoTabela >= 0 && tabelaSimbolos[topoTabela].nivel == nivelAtual) {
        topoTabela--;
    }
    nivelAtual--;
}