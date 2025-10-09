#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdio.h>

// Função de inicialização
void analisadorSintatico(FILE *file);

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
void analisaExpressao();
void analisaExpressaoSimples();
void analisaTermo();
void analisaFator();

#endif //PARSER_H