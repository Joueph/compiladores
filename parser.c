#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

// Declaração externa do token global e do arquivo
extern Token token;
FILE *inputFile;

// Função para reportar erros e terminar
void erro(const char* mensagem) {
    printf("Erro sintatico na linha %d: %s. Token lido: '%s'\n", token.linha, mensagem, token.lexema);
    exit(1);
}

// Inicia a análise
void analisadorSintatico(FILE *file) {
    inputFile = file;
    getToken(inputFile); // Pega o primeiro token
    analisaPrograma();
    printf("Analise sintatica concluida com sucesso!\n");
}

// <programa> ::= programa <identificador>; <bloco>.
void analisaPrograma() {
    if (token.simbolo == SPROGRAMA) {
        getToken(inputFile);
        if (token.simbolo == SIDENTIFICADOR) {
            getToken(inputFile);
            if (token.simbolo == SPONTOVIRGULA) {
                getToken(inputFile);
                analisaBloco();
                if (token.simbolo != SPONTO) {
                    erro("Ponto final esperado no fim do programa");
                }
            } else {
                erro("Ponto e virgula esperado apos o nome do programa");
            }
        } else {
            erro("Identificador esperado para o nome do programa");
        }
    } else {
        erro("Palavra reservada 'programa' esperada");
    }
}

// <bloco> ::= [<etapa de declaração de variáveis>] [<etapa de declaração de sub-rotinas>] <comandos>
void analisaBloco() {
    if (token.simbolo == SVAR) {
        analisaEtapaVariaveis();
    }
    if (token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        analisaSubrotinas();
    }
    analisaComandos();
}

// <etapa de declaração de variáveis> ::= var <declaração de variáveis> ; {<declaração de variáveis>;}
void analisaEtapaVariaveis() {
    if(token.simbolo == SVAR) {
        getToken(inputFile);
        while(token.simbolo == SIDENTIFICADOR) {
            analisaDeclaracaoVariaveis();
            if(token.simbolo == SPONTOVIRGULA) {
                getToken(inputFile);
            } else {
                erro("Ponto e virgula esperado apos declaracao de variavel");
            }
        }
    }
}

// <declaração de variáveis>::= <identificador> {, <identificador>} : <tipo>
void analisaDeclaracaoVariaveis() {
    while(token.simbolo == SIDENTIFICADOR) {
        getToken(inputFile);
        if (token.simbolo == SVIRGULA) {
            getToken(inputFile);
        } else if (token.simbolo == SDOISPONTOS) {
            break; // Sai para analisar o tipo
        } else {
             erro("Virgula ou dois-pontos esperado na declaracao de variavel");
        }
    }
    if(token.simbolo == SDOISPONTOS) {
        getToken(inputFile);
        analisaTipo();
    } else {
        erro("Dois-pontos esperado apos identificadores de variaveis");
    }
}

// <tipo> ::= (inteiro | booleano)
void analisaTipo() {
    if(token.simbolo == SINTEIRO || token.simbolo == SBOOLEANO) {
        getToken(inputFile);
    } else {
        erro("Tipo 'inteiro' ou 'booleano' esperado");
    }
}

// <etapa de declaração de sub-rotinas>
void analisaSubrotinas() {
    while(token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        if(token.simbolo == SPROCEDIMENTO) {
            analisaDeclaracaoProcedimento();
        } else {
            analisaDeclaracaoFuncao();
        }
        if(token.simbolo == SPONTOVIRGULA) {
            getToken(inputFile);
        } else {
            erro("Ponto e virgula esperado no final da sub-rotina");
        }
    }
}

// <declaração de procedimento> ::= procedimento <identificador>; <bloco>
void analisaDeclaracaoProcedimento() {
    getToken(inputFile); // Consome 'procedimento'
    if(token.simbolo == SIDENTIFICADOR) {
        getToken(inputFile);
        if(token.simbolo == SPONTOVIRGULA) {
            getToken(inputFile);
            analisaBloco();
        } else {
            erro("Ponto e virgula esperado apos nome do procedimento");
        }
    } else {
        erro("Identificador esperado para nome de procedimento");
    }
}

// <declaração de função> ::= funcao <identificador>: <tipo>; <bloco>
void analisaDeclaracaoFuncao() {
    getToken(inputFile); // Consome 'funcao'
    if(token.simbolo == SIDENTIFICADOR) {
        getToken(inputFile);
        if(token.simbolo == SDOISPONTOS) {
            getToken(inputFile);
            analisaTipo();
            if(token.simbolo == SPONTOVIRGULA) {
                getToken(inputFile);
                analisaBloco();
            } else {
                erro("Ponto e virgula esperado apos tipo de retorno da funcao");
            }
        } else {
            erro("Dois-pontos esperado apos nome da funcao");
        }
    } else {
        erro("Identificador esperado para nome da funcao");
    }
}


// <comandos>::= inicio <comando>{;<comando>}[;] fim
void analisaComandos() {
    if (token.simbolo == SINICIO) {
        getToken(inputFile);
        analisaComandoSimples();
        while (token.simbolo != SFIM) {
            if (token.simbolo == SPONTOVIRGULA) {
                getToken(inputFile);
                if (token.simbolo != SFIM) { // Permite ponto e vírgula opcional antes do 'fim'
                   analisaComandoSimples();
                }
            } else {
                erro("Ponto e virgula esperado entre comandos");
            }
        }
        getToken(inputFile); // Consome 'fim'
    } else {
        erro("'inicio' esperado para o bloco de comandos");
    }
}

// <comando> ::= (<atribuição_chprocedimento> | <comando condicional> | ... )
void analisaComandoSimples() {
    if (token.simbolo == SIDENTIFICADOR) {
        analisaAtribuicaoOuChamadaProcedimento();
    } else if (token.simbolo == SSE) {
        analisaSe();
    } else if (token.simbolo == SENQUANTO) {
        analisaEnquanto();
    } else if (token.simbolo == SLEIA) {
        analisaLeitura();
    } else if (token.simbolo == SESCREVA) {
        analisaEscrita();
    } else {
        analisaComandos();
}}

// <atribuição_chprocedimento>
void analisaAtribuicaoOuChamadaProcedimento(){
    getToken(inputFile);
    if(token.simbolo == SATRIBUICAO){ // Atribuição
        getToken(inputFile);
        analisaExpressao();
    }
    // Senão é uma chamada de procedimento, e já consumimos o identificador
}

// <comando leitura> ::= leia (<identificador>)
void analisaLeitura() {
    getToken(inputFile); // Consome 'leia'
    if(token.simbolo == SABREPARENTESES) {
        getToken(inputFile);
        if(token.simbolo == SIDENTIFICADOR) {
            getToken(inputFile);
            if(token.simbolo == SFECHAPARENTESES) {
                getToken(inputFile);
            } else {
                erro("Fecha parenteses esperado no comando 'leia'");
            }
        } else {
            erro("Identificador esperado no comando 'leia'");
        }
    } else {
        erro("Abre parenteses esperado no comando 'leia'");
    }
}

// <comando escrita> ::= escreva (<identificador>)
void analisaEscrita() {
    getToken(inputFile); // Consome 'escreva'
    if(token.simbolo == SABREPARENTESES) {
        getToken(inputFile);
        if(token.simbolo == SIDENTIFICADOR) { // A gramática simplificada só permite identificador
            getToken(inputFile);
            if(token.simbolo == SFECHAPARENTESES) {
                getToken(inputFile);
            } else {
                erro("Fecha parenteses esperado no comando 'escreva'");
            }
        } else {
            erro("Identificador esperado no comando 'escreva'");
        }
    } else {
        erro("Abre parenteses esperado no comando 'escreva'");
    }
}

// <comando enquanto> ::= enquanto <expressão> faca <comando>
void analisaEnquanto() {
    getToken(inputFile); // Consome 'enquanto'
    analisaExpressao();
    if(token.simbolo == SFACA) {
        getToken(inputFile);
        analisaComandoSimples();
    } else {
        erro("'faca' esperado no comando 'enquanto'");
    }
}

// <comando condicional>::= se <expressão> entao <comando> [senao <comando>]
void analisaSe() {
    getToken(inputFile); // Consome 'se'
    analisaExpressao();
    if(token.simbolo == SENTAO) {
        getToken(inputFile);
        analisaComandoSimples();
        if(token.simbolo == SSENAO) {
            getToken(inputFile);
            analisaComandoSimples();
        }
    } else {
        erro("'entao' esperado no comando 'se'");
    }
}

// <expressão>::= <expressão simples> [<operador relacional><expressão simples>]
void analisaExpressao() {
    analisaExpressaoSimples();
    if (token.simbolo == SIGUAL || token.simbolo == SDIFERENTE ||
        token.simbolo == SMENOR || token.simbolo == SMENORIGUAL ||
        token.simbolo == SMAIOR || token.simbolo == SMAIORIGUAL) {
        getToken(inputFile);
        analisaExpressaoSimples();
    }
}

// <expressão simples> ::= [+|-] <termo> {(+|-|ou) <termo> }
void analisaExpressaoSimples() {
    if(token.simbolo == SMAIS || token.simbolo == SMENOS) {
        getToken(inputFile);
    }
    analisaTermo();
    while(token.simbolo == SMAIS || token.simbolo == SMENOS || token.simbolo == SOU) {
        getToken(inputFile);
        analisaTermo();
    }
}

// <termo>::= <fator> {(* | div | e) <fator>}
void analisaTermo() {
    analisaFator();
    while(token.simbolo == SMULT || token.simbolo == SDIV || token.simbolo == SE) {
        getToken(inputFile);
        analisaFator();
    }
}

// <fator> ::= (<variável> | <número> | <chamada de função> | (<expressão>) | verdadeiro | falso | nao <fator>)
void analisaFator() {
    if (token.simbolo == SIDENTIFICADOR) {
        // Pode ser variável ou chamada de função. Sintaticamente, são iguais.
        getToken(inputFile);
    } else if (token.simbolo == SNUMERO) {
        getToken(inputFile);
    } else if (token.simbolo == SNAO) {
        getToken(inputFile);
        analisaFator();
    } else if (token.simbolo == SABREPARENTESES) {
        getToken(inputFile);
        analisaExpressao();
        if (token.simbolo == SFECHAPARENTESES) {
            getToken(inputFile);
        } else {
            erro("Fecha parenteses esperado na expressao");
        }
    } else if(token.simbolo == SVERDADEIRO || token.simbolo == SFALSO) {
        getToken(inputFile);
    } else {
        erro("Fator inesperado na expressao");
    }
}