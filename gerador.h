// gerador.h
#ifndef GERADOR_H
#define GERADOR_H

#include <stdio.h>

// --- Variáveis Globais do Gerador ---
extern FILE *outputFileMVD; // Arquivo de saída para o código MVD
extern int rotuloAtual;    // Contador para rótulos únicos (L1, L2, ...)

// --- Funções Principais ---

/**
 * @brief Inicia o gerador, abrindo o arquivo de saída.
 * @param filename O nome do arquivo de saída (ex: "programa.mvd")
 */
void iniciaGerador(const char* filename);

/**
 * @brief Finaliza o gerador, fechando o arquivo de saída.
 */
void finalizaGerador();

/**
 * @brief Gera (escreve) uma instrução MVD no arquivo de saída.
 * Usa formato similar ao printf.
 * * Ex: 
 * gera("LDC", 1, -1);      // LDC 1
 * gera("LDV", 0, 5);      // LDV 5
 * gera("ADD", -1, -1);    // ADD
 * gera("JMPF", 0, 1);     // JMPF L1
 * gera("STR", 0, 3);      // STR 3
 */
void gera(const char* instrucao, int p1, int p2);

/**
 * @brief Gera (escreve) um rótulo (Label) no formato MVD (ex: "L1 NULL").
 * @param rotulo O número do rótulo a ser gerado.
 */
void geraRotulo(int rotulo);

/**
 * @brief Solicita um novo número de rótulo único.
 * @return O número do próximo rótulo (ex: 1, depois 2, ...).
 */
int novoRotulo();

#endif //GERADOR_H