// parser.c
#include "parser.h"
#include "globals.h" // Importa 'token' e 'errosCompilacao'
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

    // Reserva SEMPRE a célula 0 para retorno de funções
    gera("ALLOC", 0, 1, NULL);
    enderecoAtual = 1; // primeira variável "normal" será no endereço 1

    getToken();
    analisaPrograma();

    // Libera célula de retorno e finaliza
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
                if (strlen(token.lexema) > 0) {
                    erro_sintatico("Codigo encontrado apos o ponto final do programa");
                }
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
    int enderecoInicioBloco = enderecoAtual;
    int varsAlocadas = 0;

    if (token.simbolo == SVAR) {
        analisaEtapaVariaveis();
    }

    varsAlocadas = enderecoAtual - enderecoInicioBloco;

    if (varsAlocadas > 0) {
        gera("ALLOC", enderecoInicioBloco, varsAlocadas, NULL);
    }

    if (token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        analisaSubrotinas();
    }

    analisaComandos();

    if (varsAlocadas > 0) {
        gera("DALLOC", enderecoInicioBloco, varsAlocadas, NULL);
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
    int rotuloFimSubrotinas = novoRotulo();
    gera("JMP", rotuloFimSubrotinas, -1, NULL);

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
    
    geraRotulo(rotuloFimSubrotinas);
}

void analisaDeclaracaoProcedimento() {
    int rotuloProc = novoRotulo();

    getToken(); // 'procedimento'
    if(token.simbolo == SIDENTIFICADOR) {
        if (consulta_duplicidade_escopo(token.lexema)) {
            erro_semantico("Identificador ja declarado neste escopo", token.lexema);
        }
        insere_tabela(token.lexema, "procedimento", rotuloProc);
        geraRotulo(rotuloProc); 
        
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

// ----------- Função: usa célula 0 como retorno global -----------
void analisaDeclaracaoFuncao() {
    int rotuloFunc = -1;
    getToken(); // 'funcao'
    if(token.simbolo == SIDENTIFICADOR) {
        char nomeFuncao[50];
        strcpy(nomeFuncao, token.lexema);
        
        getToken();
        if(token.simbolo == SDOISPONTOS) {
            getToken();
            
            rotuloFunc = novoRotulo();
            char tipoRetorno[20]; 

            if(token.simbolo == SINTEIRO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) {
                     erro_semantico("Identificador ja declarado neste escopo", nomeFuncao);
                }
                insere_tabela(nomeFuncao, "funcao inteiro", rotuloFunc);
                strcpy(tipoRetorno, "inteiro");
            } else if (token.simbolo == SBOOLEANO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) {
                     erro_semantico("Identificador ja declarado neste escopo", nomeFuncao);
                }
                insere_tabela(nomeFuncao, "funcao booleano", rotuloFunc);
                strcpy(tipoRetorno, "booleano");
            } else {
                erro_sintatico("Tipo de retorno 'inteiro' ou 'booleano' esperado para a funcao");
                strcpy(tipoRetorno, "inteiro");
            }

            geraRotulo(rotuloFunc);

            entra_escopo(); 

            // OBS: NÃO insere variável oculta com o nome da função.
            // O retorno é sempre na célula 0.

            getToken();
            
            if(token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
            } else {
                erro_sintatico("Ponto e virgula esperado apos tipo de retorno da funcao");
            }

            // Função termina com RETURN simples (retorno já está em 0)
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

// ----------------- Atribuição ou chamada de procedimento -----------------
void analisaAtribuicaoOuChamadaProcedimento(){
    char nomeId[50];
    strcpy(nomeId, token.lexema);

    int indice = consulta_tabela(nomeId);
    if (indice == -1) {
        erro_semantico("Identificador nao declarado", nomeId);
        while (token.simbolo != SPONTOVIRGULA && token.simbolo != SFIM && strlen(token.lexema) > 0) getToken();
        return;
    }
    
    getToken();
    
    if(token.simbolo == SATRIBUICAO){ 
        const char* tipoId = tabelaSimbolos[indice].tipo;
        int enderecoDestino;

        if (strcmp(tipoId, "procedimento") == 0 || strcmp(tipoId, "programa") == 0) {
            erro_semantico("Nao se pode atribuir valor a um procedimento ou programa", nomeId);
            enderecoDestino = 0;
        } else if (strncmp(tipoId, "funcao", 6) == 0) {
            // Atribuição para o nome da função => célula 0
            enderecoDestino = 0;
        } else {
            // variável normal
            enderecoDestino = tabelaSimbolos[indice].endereco;
        }
        
        int tipoVariavel = get_tipo_simbolo(nomeId); 
        
        getToken();
        int tipoExpressao = analisaExpressao(); 

        if (tipoVariavel != -1 && tipoVariavel != tipoExpressao) {
            erro_semantico("Tipos incompativeis na atribuicao", nomeId);
        }

        gera("STR", enderecoDestino, -1, NULL); 

    } else { // chamada de procedimento
        const char* tipoId = tabelaSimbolos[indice].tipo;
        if (strcmp(tipoId, "procedimento") != 0) {
            // se não for procedimento, é provável que seja função usada como comando
            erro_semantico("Chamada de procedimento invalida (identificador nao e procedimento)", nomeId);
        }
        gera("CALL", tabelaSimbolos[indice].endereco, -1, NULL);
    }
}

// ----------------- leia / escreva -----------------
void analisaLeitura() {
    int enderecoVar = -1;
    getToken(); 
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            char nomeVar[50]; strcpy(nomeVar, token.lexema);
            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            } else {
                if (strcmp(tabelaSimbolos[indice].tipo, "inteiro") != 0) {
                    erro_semantico("Comando 'leia' so aceita variaveis do tipo inteiro", token.lexema);
                }
                enderecoVar = tabelaSimbolos[indice].endereco;
            }
            
            getToken();
            if(token.simbolo == SFECHAPARENTESES) {
                // Gera RD com o endereço e o nome da variável
                gera("RD", enderecoVar, -1, nomeVar);
                getToken();
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
    int enderecoVar = -1;
    getToken(); 
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            } else {
                 if (strcmp(tabelaSimbolos[indice].tipo, "inteiro") != 0 &&
                     strcmp(tabelaSimbolos[indice].tipo, "funcao inteiro") != 0) {
                    erro_semantico("Comando 'escreva' so aceita variavel ou funcao do tipo inteiro", token.lexema);
                }
                enderecoVar = tabelaSimbolos[indice].endereco;
            }

            getToken();
            if(token.simbolo == SFECHAPARENTESES) {
                if (strcmp(tabelaSimbolos[indice].tipo, "funcao inteiro") == 0) {
                    // escreva(nomeFuncao) => chama a função e usa célula 0
                    gera("CALL", tabelaSimbolos[indice].endereco, -1, NULL);
                    gera("LDV", 0, -1, NULL);
                } else {
                    if (enderecoVar != -1) gera("LDV", enderecoVar, -1, NULL);
                }
                gera("PRN", -1, -1, NULL);
                getToken();
            } else {
                erro_sintatico("Fecha parenteses esperado no comando 'escreva'");
            }
        } else {
            erro_sintatico("Identificador esperado no comando 'escreva'");
        }
    } else {
        erro_sintatico("Abre parenteses esperado no comando 'escreva'");
    }
}

// ----------------- enquanto -----------------
void analisaEnquanto() {
    int rotuloInicio = novoRotulo();
    int rotuloFim = novoRotulo();
    geraRotulo(rotuloInicio); 

    getToken(); 
    
    int tipoExpressao = analisaExpressao();
    if (tipoExpressao != 1) { 
        erro_semantico("Expressao booleana esperada no comando 'enquanto'", "");
    }

    // JMPF LrotuloFim
    gera("JMPF", rotuloFim, -1, NULL);

    if(token.simbolo == SFACA) {
        getToken();
        analisaComandoSimples();
        // JMP Linicio
        gera("JMP", rotuloInicio, -1, NULL);
        geraRotulo(rotuloFim); 

    } else {
        erro_sintatico("'faca' esperado no comando 'enquanto'");
    }
}

// ----------------- se / entao / senao -----------------
void analisaSe() {
    int rotuloSenao = -1, rotuloFimSe = -1;

    getToken(); 
    
    int tipoExpressao = analisaExpressao();
    if (tipoExpressao != 1) { 
        erro_semantico("Expressao booleana esperada no comando 'se'", "");
    }

    rotuloSenao = novoRotulo();
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
    } else {
        erro_sintatico("'entao' esperado no comando 'se'");
    }
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
        int indice = consulta_tabela(token.lexema);
        if (indice == -1) {
            erro_semantico("Identificador nao declarado", token.lexema);
            getToken();
            return -1;
        }
        
        const char* tipoId = tabelaSimbolos[indice].tipo;

        if (strcmp(tipoId, "inteiro") == 0) {
            tipo = 0;
            gera("LDV", tabelaSimbolos[indice].endereco, -1, NULL);
        } else if (strcmp(tipoId, "booleano") == 0) {
            tipo = 1;
            gera("LDV", tabelaSimbolos[indice].endereco, -1, NULL);
        } else if (strcmp(tipoId, "funcao inteiro") == 0) {
            tipo = 0;
            // Chamada de função em expressão: CALL + LDV 0
            gera("CALL", tabelaSimbolos[indice].endereco, -1, NULL);
            gera("LDV", 0, -1, NULL);
        } else if (strcmp(tipoId, "funcao booleano") == 0) {
            tipo = 1;
            gera("CALL", tabelaSimbolos[indice].endereco, -1, NULL);
            gera("LDV", 0, -1, NULL);
        } else {
            erro_semantico("Identificador nao pode ser usado em uma expressao (deve ser var ou funcao)", token.lexema);
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
