// parser.c
#include "parser.h"
#include "globals.h"
#include "gerador.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ----------------- Tratamento de erro sintático -----------------
void erro_sintatico(const char* mensagem) {
    errosCompilacao++; 
    printf("Erro Sintatico: %s. Ultimo token lido: '%s'\n", mensagem, token.lexema);
    
    // Modo pânico: consome até delimitador seguro
    while (token.simbolo != SPONTOVIRGULA && 
           token.simbolo != SFIM && 
           token.simbolo != SPONTO &&
           strlen(token.lexema) > 0) {
        getToken();
    }

    if (token.simbolo == SPONTOVIRGULA) {
        getToken();
    }
}

// ----------------- Entrada principal do parser -----------------
void analisadorSintatico() {
    errosCompilacao = 0;

    gera("START", -1, -1, NULL);

    // Modelo Estático: Endereço 0 é o registrador de retorno global
    gera("ALLOC", 0, 1, NULL);
    enderecoAtual = 1; // Globais começam no 1

    getToken();
    analisaPrograma();

    gera("DALLOC", 0, 1, NULL);
    
    gera("HLT", -1, -1, NULL);
    
    if (errosCompilacao == 0) {
        printf("Analises concluidas com sucesso!\n");
    } else {
        printf("Analise finalizada com %d erros.\n", errosCompilacao);
        remove("programa.mvd");
    }
}

// ----------------- <programa> -----------------
void analisaPrograma() {
    if (token.simbolo == SPROGRAMA) {
        getToken();
        if (token.simbolo == SIDENTIFICADOR) {
            insere_tabela(token.lexema, "programa", -1);
            getToken();
            if (token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
                if (token.simbolo != SPONTO) {
                    erro_sintatico("Ponto final esperado no fim do programa");
                }
                getToken();
            } else {
                erro_sintatico("Ponto e virgula esperado apos o nome do programa");
            }
        } else {
            erro_sintatico("Identificador esperado para o nome do programa");
        }
    } else {
        erro_sintatico("Palavra reservada 'programa' esperada");
    }
}

// ----------------- <bloco> -----------------
void analisaBloco() {
    int inicioBloco = enderecoAtual;
    int varsAlocadas = 0;

    if (token.simbolo == SVAR) {
        analisaEtapaVariaveis();
    }

    varsAlocadas = enderecoAtual - inicioBloco;

    if (varsAlocadas > 0) {
        gera("ALLOC", inicioBloco, varsAlocadas, NULL);
    }

    if (token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        analisaSubrotinas();
    }

    analisaComandos();

    if (varsAlocadas > 0) {
        gera("DALLOC", inicioBloco, varsAlocadas, NULL);
    }
}

// ----------------- Declaração de variáveis -----------------
void analisaEtapaVariaveis() {
    if(token.simbolo == SVAR) {
        getToken();
        while(token.simbolo == SIDENTIFICADOR) {
            analisaDeclaracaoVariaveis();
            if(token.simbolo == SPONTOVIRGULA) {
                getToken();
            } else {
                erro_sintatico("Ponto e virgula esperado apos declaracao de variavel");
            }
        }
    }
}

void analisaDeclaracaoVariaveis() {
    while(token.simbolo == SIDENTIFICADOR) {
        if (consulta_duplicidade_escopo(token.lexema)) {
            erro_semantico("Identificador ja declarado neste escopo", token.lexema);
        }
        
        insere_tabela(token.lexema, "variavel", enderecoAtual);
        enderecoAtual++;

        getToken();
        if (token.simbolo == SVIRGULA) {
            getToken();
        } else if (token.simbolo == SDOISPONTOS) {
            break; 
        } else {
             erro_sintatico("Virgula ou dois-pontos esperado na declaracao de variavel");
             break; 
        }
    }
    if(token.simbolo == SDOISPONTOS) {
        getToken();
        analisaTipo();
    } else {
        erro_sintatico("Dois-pontos esperado apos identificadores de variaveis");
    }
}

void analisaTipo() {
    if(token.simbolo == SINTEIRO) {
        coloca_tipo_tabela("inteiro");
        getToken();
    } else if (token.simbolo == SBOOLEANO) {
        coloca_tipo_tabela("booleano");
        getToken();
    } else {
        erro_sintatico("Tipo 'inteiro' ou 'booleano' esperado");
    }
}

// ----------------- Sub-rotinas -----------------
void analisaSubrotinas() {
    int rotuloFimSub = novoRotulo();
    gera("JMP", rotuloFimSub, -1, NULL);

    while(token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        if(token.simbolo == SPROCEDIMENTO) {
            analisaDeclaracaoProcedimento();
        } else {
            analisaDeclaracaoFuncao();
        }
        if(token.simbolo == SPONTOVIRGULA) {
            getToken();
        } else {
            erro_sintatico("Ponto e virgula esperado no final da sub-rotina");
        }
    }
    
    geraRotulo(rotuloFimSub);
}

void analisaDeclaracaoProcedimento() {
    int rotulo = novoRotulo();
    getToken(); // 'procedimento'
    if(token.simbolo == SIDENTIFICADOR) {
        if (consulta_duplicidade_escopo(token.lexema)) {
            erro_semantico("Identificador ja declarado neste escopo", token.lexema);
        }
        insere_tabela(token.lexema, "procedimento", rotulo);
        geraRotulo(rotulo);
        
        entra_escopo(); 
        getToken();
        
        if(token.simbolo == SPONTOVIRGULA) {
            getToken();
            analisaBloco();
        } else {
            erro_sintatico("Ponto e virgula esperado apos nome do procedimento");
        }

        gera("RETURN", -1, -1, NULL);
        sai_escopo(); 
    } else {
        erro_sintatico("Identificador esperado para nome de procedimento");
    }
}

void analisaDeclaracaoFuncao() {
    int rotulo = novoRotulo();
    getToken(); // 'funcao'
    if(token.simbolo == SIDENTIFICADOR) {
        char nome[50]; strcpy(nome, token.lexema);
        
        getToken();
        if(token.simbolo == SDOISPONTOS) {
            getToken();
            
            if(token.simbolo == SINTEIRO) {
                if (consulta_duplicidade_escopo(nome)) erro_semantico("Duplicidade", nome);
                insere_tabela(nome, "funcao inteiro", rotulo);
            } else if (token.simbolo == SBOOLEANO) {
                if (consulta_duplicidade_escopo(nome)) erro_semantico("Duplicidade", nome);
                insere_tabela(nome, "funcao booleano", rotulo);
            } else {
                erro_sintatico("Tipo de retorno invalido");
            }

            geraRotulo(rotulo);
            entra_escopo(); 
            getToken();
            
            if(token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
            } else erro_sintatico("; esperado");

            gera("RETURN", -1, -1, NULL);
            sai_escopo(); 
        } else {
            erro_sintatico("Dois-pontos esperado apos nome da funcao");
        }
    } else {
        erro_sintatico("Identificador esperado para nome da funcao");
    }
}

// ----------------- Comandos -----------------
void analisaComandos() {
    if (token.simbolo == SINICIO) {
        getToken();
        analisaComandoSimples();
        while (token.simbolo != SFIM && strlen(token.lexema) > 0) {
            if (token.simbolo == SPONTOVIRGULA) {
                getToken();
                if (token.simbolo != SFIM) { 
                   analisaComandoSimples();
                }
            } else {
                erro_sintatico("Ponto e virgula esperado entre comandos");
            }
        }
        getToken(); // 'fim'
    } else {
        erro_sintatico("'inicio' esperado para o bloco de comandos");
    }
}

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
    } else if (token.simbolo == SINICIO) {
        analisaComandos();
    } else {
        if (token.simbolo != SFIM && token.simbolo != SPONTOVIRGULA) {
             erro_sintatico("Comando invalido");
        }
    }
}

// ----------------- Atribuição ou chamada -----------------
void analisaAtribuicaoOuChamadaProcedimento(){
    char nomeId[50];
    strcpy(nomeId, token.lexema);

    int idx = consulta_tabela(nomeId);
    if (idx == -1) {
        erro_semantico("Identificador nao declarado", nomeId);
        while (token.simbolo != SPONTOVIRGULA && token.simbolo != SFIM && strlen(token.lexema) > 0) getToken();
        return;
    }
    
    int addr = tabelaSimbolos[idx].endereco;
    char tipo[30]; strcpy(tipo, tabelaSimbolos[idx].tipo);

    getToken();
    
    if(token.simbolo == SATRIBUICAO){ 
        getToken();
        analisaExpressao(); 

        if (strncmp(tipo, "funcao", 6) == 0) {
            gera("STR", 0, -1, NULL);
        } else {
            gera("STR", addr, -1, NULL);
        }
    } else {
        if (strcmp(tipo, "procedimento") != 0) {
            erro_semantico("Chamada invalida", nomeId);
        }
        gera("CALL", addr, -1, NULL);
    }
}

// ----------------- leia / escreva -----------------
void analisaLeitura() {
    int enderecoVar = -1;

    getToken(); // depois de 'leia'

    if(token.simbolo == SABREPARENTESES) {
        getToken();

        if(token.simbolo == SIDENTIFICADOR) {
            int idx = consulta_tabela(token.lexema);
            if(idx == -1) {
                erro_semantico("Variavel nao declarada", token.lexema);
            } else {
                if (strcmp(tabelaSimbolos[idx].tipo, "inteiro") != 0) {
                    erro_semantico("Comando 'leia' so aceita inteiro", token.lexema);
                }
                enderecoVar = tabelaSimbolos[idx].endereco;
            }

            getToken();

            if(token.simbolo == SFECHAPARENTESES) {
                // RD sozinho (empilha o valor lido)
                gera("RD", -1, -1, NULL);

                // STR guarda na memória absoluta
                if(enderecoVar != -1)
                    gera("STR", enderecoVar, -1, NULL);

                getToken(); // consome ')'
            } else {
                erro_sintatico("Fecha parenteses esperado no comando 'leia'");
            }
        } else {
            erro_sintatico("Identificador esperado no comando 'leia'");
        }
    } else {
        erro_sintatico("Abre parenteses esperado no comando 'leia'");
    }
}

void analisaEscrita() {
    getToken(); 
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            int idx = consulta_tabela(token.lexema);
            if (idx == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            } else {
                if (strncmp(tabelaSimbolos[idx].tipo, "funcao", 6) == 0) {
                    gera("CALL", tabelaSimbolos[idx].endereco, -1, NULL);
                    gera("LDV", 0, -1, NULL);
                } else {
                    gera("LDV", tabelaSimbolos[idx].endereco, -1, NULL);
                }
            }
            getToken();
            if(token.simbolo == SFECHAPARENTESES) {
                gera("PRN", -1, -1, NULL);
                getToken();
            } else erro_sintatico("Fecha parenteses esperado");
        } else erro_sintatico("Identificador esperado");
    } else erro_sintatico("Abre parenteses esperado");
}

// ----------------- Comandos de Controle -----------------
void analisaEnquanto() {
    int rotuloInicio = novoRotulo();
    int rotuloFim = novoRotulo();
    geraRotulo(rotuloInicio); 
    getToken(); 
    analisaExpressao();
    gera("JMPF", rotuloFim, -1, NULL);
    if(token.simbolo == SFACA) {
        getToken();
        analisaComandoSimples();
        gera("JMP", rotuloInicio, -1, NULL);
        geraRotulo(rotuloFim); 
    } else erro_sintatico("'faca' esperado");
}

void analisaSe() {
    int rotuloSenao = novoRotulo();
    int rotuloFimSe = -1;
    getToken(); 
    analisaExpressao();
    gera("JMPF", rotuloSenao, -1, NULL); 
    if(token.simbolo == ENTAO) {
        getToken();
        analisaComandoSimples();
        if(token.simbolo == SSENAO) {
            rotuloFimSe = novoRotulo();
            gera("JMP", rotuloFimSe, -1, NULL);
            geraRotulo(rotuloSenao);
            getToken();
            analisaComandoSimples();
            geraRotulo(rotuloFimSe);
        } else {
            geraRotulo(rotuloSenao);
        }
    } else erro_sintatico("'entao' esperado");
}

// ----------------- Expressões -----------------
int analisaExpressao() {
    int tipo = analisaExpressaoSimples();

    if (token.simbolo == SIGUAL || token.simbolo == SDIFERENTE ||
        token.simbolo == SMENOR || token.simbolo == SMENORIGUAL ||
        token.simbolo == SMAIOR || token.simbolo == SMAIORIGUAL) {
        
        Simbolo op = token.simbolo;
        getToken();
        int tipo2 = analisaExpressaoSimples();

        if (tipo != tipo2) {
            erro_semantico("Tipos incompativeis em expressao relacional", "");
        }
        
        switch(op) {
            case SIGUAL:       gera("CEQ",  -1, -1, NULL); break;
            case SDIFERENTE:   gera("CDIF", -1, -1, NULL); break;
            case SMENOR:       gera("CME",  -1, -1, NULL); break;
            case SMAIOR:       gera("CMA",  -1, -1, NULL); break;
            case SMENORIGUAL:  gera("CMEQ", -1, -1, NULL); break;
            case SMAIORIGUAL:  gera("CMAQ", -1, -1, NULL); break;
            default: break;
        }
        return 1; 
    }
    return tipo; 
}

int analisaExpressaoSimples() {
    int tipo;
    if(token.simbolo == SMAIS || token.simbolo == SMENOS) {
        Simbolo opUnario = token.simbolo;
        getToken();
        tipo = analisaTermo();
        if (tipo != 0) { 
            erro_semantico("Operador unario '+' ou '-' so pode ser usado com tipo inteiro", "");
        }

        if (opUnario == SMENOS) {
            gera("INV", -1, -1, NULL);
        }

    } else {
        tipo = analisaTermo();
    }

    while(token.simbolo == SMAIS || token.simbolo == SMENOS || token.simbolo == SOU) {
        Simbolo op = token.simbolo;
        getToken();
        int tipo2 = analisaTermo();

        if (op == SOU) {
            if (tipo != 1 || tipo2 != 1) {
                erro_semantico("Operador 'ou' so pode ser usado com expressoes booleanas", "");
            }
            tipo = 1; 
            gera("OR", -1, -1, NULL);
        } else { 
             if (tipo != 0 || tipo2 != 0) {
                erro_semantico("Operador '+' ou '-' so pode ser usado com expressoes inteiras", "");
            }
            if (op == SMAIS) gera("ADD", -1, -1, NULL);
            else             gera("SUB", -1, -1, NULL);
            tipo = 0; 
        }
    }
    return tipo;
}

int analisaTermo() {
    int tipo = analisaFator();
    
    while(token.simbolo == SMULT || token.simbolo == SDIV || token.simbolo == SE) {
        Simbolo op = token.simbolo;
        getToken();
        int tipo2 = analisaFator();
        
        if (op == SE) {
            if (tipo != 1 || tipo2 != 1) {
                erro_semantico("Operador 'e' so pode ser usado com expressoes booleanas", "");
            }
            tipo = 1; 
            gera("AND", -1, -1, NULL);
        } else { 
             if (tipo != 0 || tipo2 != 0) {
                erro_semantico("Operador '*' ou 'div' so pode ser usado com expressoes inteiras", "");
            }
            if (op == SMULT) gera("MULT", -1, -1, NULL);
            else             gera("DIVI", -1, -1, NULL);
            tipo = 0; 
        }
    }
    return tipo;
}

// ----------------- Fator -----------------
int analisaFator() {
    int tipo = -1; 
    
    if (token.simbolo == SIDENTIFICADOR) {
        int idx = consulta_tabela(token.lexema);
        if (idx == -1) {
            erro_semantico("Identificador nao declarado", token.lexema);
            getToken();
            return -1;
        }
        
        if (strncmp(tabelaSimbolos[idx].tipo, "funcao", 6) == 0) {
            gera("CALL", tabelaSimbolos[idx].endereco, -1, NULL);
            gera("LDV", 0, -1, NULL);
            tipo = (strcmp(tabelaSimbolos[idx].tipo, "funcao inteiro") == 0) ? 0 : 1;
        } else {
            gera("LDV", tabelaSimbolos[idx].endereco, -1, NULL);
            tipo = (strcmp(tabelaSimbolos[idx].tipo, "inteiro") == 0) ? 0 : 1;
        }
        
        getToken();

    } else if (token.simbolo == SNUMERO) {
        tipo = 0; 
        gera("LDC", atoi(token.lexema), -1, NULL); 
        getToken();

    } else if (token.simbolo == SNAO) {
        getToken();
        tipo = analisaFator();
        if (tipo != 1) {
            erro_semantico("Operador 'nao' so pode ser usado com tipo booleano", "");
        }
        tipo = 1; 
        gera("NEG", -1, -1, NULL); 

    } else if (token.simbolo == SABREPARENTESES) {
        getToken();
        tipo = analisaExpressao();
        if (token.simbolo == SFECHAPARENTESES) {
            getToken();
        } else {
            erro_sintatico("Fecha parenteses esperado na expressao");
        }
    } else if(token.simbolo == SVERDADEIRO || token.simbolo == SFALSO) {
        tipo = 1; 
        if (token.simbolo == SVERDADEIRO) gera("LDC", 1, -1, NULL); 
        else                              gera("LDC", 0, -1, NULL); 
        getToken();
    } else {
        erro_sintatico("Fator inesperado na expressao");
        getToken();
    }
    
    return tipo;
}
