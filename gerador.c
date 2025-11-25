// gerador.c
#include "gerador.h"
#include <stdlib.h>
#include <string.h> // Adicionado para strcmp

FILE *outputFileMVD = NULL;
int rotuloAtual = 0;

void iniciaGerador(const char* filename) {
    outputFileMVD = fopen(filename, "w");
    if (!outputFileMVD) {
        perror("Nao foi possivel criar o arquivo de saida MVD");
        exit(1);
    }
    rotuloAtual = 0;
}

void finalizaGerador() {
    if (outputFileMVD) {
        fclose(outputFileMVD);
    }
}

// Gera uma instrução MVD
// p1 e p2 são os parâmetros. -1 ou valores negativos são ignorados.
// Adicionado p3 como string para o nome da variável (usado em RD)
void gera(const char* instrucao, int p1, int p2, const char* p3) {

    fprintf(outputFileMVD, "%s", instrucao);
    
    // Parâmetro 1 (pode ser endereço ou rótulo)
    if (p1 >= 0) {
        // Se for instrução de desvio, formata como Rótulo (ex: L1)
        if (strcmp(instrucao, "JMP") == 0 || strcmp(instrucao, "JMPF") == 0 || strcmp(instrucao, "CALL") == 0) {
            fprintf(outputFileMVD, " L%d", p1);
        } else { // Senão, formata como número (endereço ou valor)
            fprintf(outputFileMVD, " %d", p1);
        }
    }
    
    // Parâmetro 2 (usado por ALLOC/DALLOC) [cite: 2527]
    if (p2 >= 0) {
        fprintf(outputFileMVD, ",%d", p2);
    }
    
    // Parâmetro 3 (string, para nome de variável)
    if (p3 != NULL) {
        fprintf(outputFileMVD, ",%s", p3);
    }
    
    fprintf(outputFileMVD, "\n");
}

void geraRotulo(int rotulo) {
    if (!outputFileMVD) return;
    // Um rótulo na MVD é uma instrução NULL [cite: 2234]
    fprintf(outputFileMVD, "L%d NULL\n", rotulo);
}

int novoRotulo() {
    rotuloAtual++;
    return rotuloAtual;
}