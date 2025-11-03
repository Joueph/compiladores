// simbolo.h
#ifndef SIMBOLO_H
#define SIMBOLO_H

#define MAX_SIMBOLOS 100

// Definição da estrutura de um símbolo, baseada nos requisitos do CSD
[cite_start]// [cite: 1150-1154]
typedef struct {
    char nome[50];      [cite_start]// Nome do identificador (lexema) [cite: 1151]
    char tipo[30];      [cite_start]// Tipo (padrão do identificador) [cite: 1153]
    int nivel;          [cite_start]// Escopo (nível de declaração) [cite: 1152]
    int endereco;       [cite_start]// Memória (endereço alocado) [cite: 1154]
} TipoSimbolo;

[cite_start]// Declaração da tabela como um vetor (pilha) [cite: 1124]
extern TipoSimbolo tabelaSimbolos[MAX_SIMBOLOS];
extern int topoTabela;
extern int nivelAtual;

// --- Protótipos das Funções de Gerenciamento ---

[cite_start]// Procedimentos básicos especificados no CSD [cite: 1155]

/**
 * Insere um novo símbolo na tabela.
 * [cite_start]Implementa a "Insere na Tabela" [cite: 1157]
 */
void insere_tabela(const char* nome, const char* tipo);

/**
 * Atualiza o tipo das últimas 'n' variáveis inseridas.
 * [cite_start]Implementa a "Coloca Tipo nas Variáveis" [cite: 1159]
 */
void coloca_tipo_tabela(const char* tipo);

/**
 * Procura um símbolo na tabela, do escopo atual para os externos.
 * Retorna o índice na tabela se encontrar, ou -1 se não encontrar.
 * [cite_start]Implementa a "Consulta a Tabela" [cite: 1158]
 */
int consulta_tabela(const char* nome);

/**
 * Procura por um símbolo com o mesmo nome *apenas no nível atual*.
 * Retorna 1 se encontrar duplicata, 0 se não.
 */
int consulta_duplicidade(const char* nome);

// Funções auxiliares para gerenciar o escopo (nível)
void entra_escopo();
void sai_escopo();

#endif //SIMBOLO_H