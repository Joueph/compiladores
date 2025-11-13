// simbolo.c
#include "simbolo.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "globals.h" // Para usar o token.lexema no erro

// Definição das variáveis globais da tabela
TipoSimbolo tabelaSimbolos[MAX_SIMBOLOS];
int topoTabela = -1;
int nivelAtual = 0;

void erro_semantico(const char* mensagem, const char* lexema) {
    printf("Erro Semantico: %s ['%s']\n", mensagem, lexema);
    exit(1);
}

void insere_tabela(const char* nome, const char* tipo, int endereco) {
    if (topoTabela >= MAX_SIMBOLOS - 1) {
        erro_semantico("Tabela de simbolos cheia!", nome);
    }
    
    topoTabela++;
    strcpy(tabelaSimbolos[topoTabela].nome, nome);
    strcpy(tabelaSimbolos[topoTabela].tipo, tipo);
    tabelaSimbolos[topoTabela].nivel = nivelAtual;
    tabelaSimbolos[topoTabela].endereco = endereco; 
}

void coloca_tipo_tabela(const char* tipo) {
    int i = topoTabela;
    while (i >= 0 && tabelaSimbolos[i].nivel == nivelAtual) {
        // Se o tipo for "variavel", atualiza para o tipo real
        if (strcmp(tabelaSimbolos[i].tipo, "variavel") == 0) {
            if (strcmp(tipo, "inteiro") == 0) {
                 strcpy(tabelaSimbolos[i].tipo, "inteiro");
            } else {
                 strcpy(tabelaSimbolos[i].tipo, "booleano");
            }
        }
        i--;
    }
}

// Procura do escopo atual para o global
int consulta_tabela(const char* nome) {
    int i = topoTabela;
    while (i >= 0) {
        if (strcmp(tabelaSimbolos[i].nome, nome) == 0) {
            return i; // Encontrou
        }
        i--;
    }
    return -1; // Não encontrou
}

// Procura duplicidade APENAS no nível atual
int consulta_duplicidade_escopo(const char* nome) {
    int i = topoTabela;
    while (i >= 0 && tabelaSimbolos[i].nivel == nivelAtual) {
        if (strcmp(tabelaSimbolos[i].nome, nome) == 0) {
            return 1; // Encontrou duplicata
        }
        i--;
    }
    return 0; // Não há duplicata
}

void entra_escopo() {
    nivelAtual++;
}

void sai_escopo() {
    while (topoTabela >= 0 && tabelaSimbolos[topoTabela].nivel == nivelAtual) {
        topoTabela--;
    }
    nivelAtual--;
}

// Retorna 0 para inteiro, 1 para booleano, -1 para outros/erro
int get_tipo_simbolo(const char* nome) {
    int i = consulta_tabela(nome);
    if (i == -1) return -1; // Não foi declarado

    if (strcmp(tabelaSimbolos[i].tipo, "inteiro") == 0) return 0;
    if (strcmp(tabelaSimbolos[i].tipo, "booleano") == 0) return 1;
    if (strcmp(tabelaSimbolos[i].tipo, "funcao inteiro") == 0) return 0;
    if (strcmp(tabelaSimbolos[i].tipo, "funcao booleano") == 0) return 1;
    
    return -1; // É um procedimento ou outro tipo
}