// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "simbolo.h" // Inclui a tabela de símbolos
#include <stdio.h>

// Função de inicialização
void analisadorSintatico();

// Protótipos para cada não-terminal da gramática
void analisaPrograma();
void analisaBloco();
void analisaEtapaVariaveis();
void analisaDeclaracaoVariaveis();
void analisaTipo();
void analisaComandos();
void analisaComandoSimples();
void analisaAtribuicaoOuChamadaProcedimento();
void analisaLeitura();
void analisaEscrita();
void analisaEnquanto();
void analisaSe();
void analisaSubrotinas();
void analisaDeclaracaoProcedimento();
void analisaDeclaracaoFuncao();

// Funções de expressão agora retornam o TIPO (0=int, 1=bool)
int analisaExpressao();
int analisaExpressaoSimples();
int analisaTermo();
int analisaFator();

void erro_sintatico(const char* mensagem);

#endif //PARSER_H