// simbolo.h
#ifndef SIMBOLO_H
#define SIMBOLO_H

#define MAX_SIMBOLOS 100

// Definição da estrutura de um símbolo, baseada nos requisitos do CSD
typedef struct {
    char nome[50];      // Nome do identificador (lexema)
    char tipo[30];      // Tipo (padrão do identificador)
    int nivel;          // Escopo (nível de declaração)
    int endereco;       // Memória (endereço alocado)
} TipoSimbolo;

// Declaração da tabela como um vetor (pilha)
extern TipoSimbolo tabelaSimbolos[MAX_SIMBOLOS];
extern int topoTabela;
extern int nivelAtual;

// --- Protótipos das Funções de Gerenciamento ---

// Procedimentos básicos especificados no CSD

/**
 * Insere um novo símbolo na tabela.
 * Implementa a "Insere na Tabela"
 */
void insere_tabela(const char* nome, const char* tipo);

/**
 * Atualiza o tipo das últimas 'n' variáveis inseridas.
 * Implementa a "Coloca Tipo nas Variáveis"
 */
void coloca_tipo_tabela(const char* tipo);

/**
 * Procura um símbolo na tabela, do escopo atual para os externos.
 * Retorna o índice na tabela se encontrar, ou -1 se não encontrar.
 * Implementa a "Consulta a Tabela"
 */
int consulta_tabela(const char* nome);

/**
 * Procura por um símbolo com o mesmo nome *apenas no nível atual*.
 * Retorna 1 se encontrar duplicata, 0 se não.
 */
int consulta_duplicidade_escopo(const char* nome);

// Funções auxiliares para gerenciar o escopo (nível)
void entra_escopo();
void sai_escopo();

// Funções de verificação semântica
int get_tipo_simbolo(const char* nome); // Retorna o TIPO (ex: 0=int, 1=bool, -1=erro)
void erro_semantico(const char* mensagem, const char* lexema);

#endif //SIMBOLO_H