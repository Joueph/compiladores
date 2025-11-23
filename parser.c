// parser.c
#include "parser.h"
#include "globals.h"
#include "gerador.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Variável global para controle de endereços locais (dentro de funções)
int enderecoLocal = 0;

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
    int varsAlocadas = 0;
    // Se for global (nível 0), usa contador global, senão local
    int inicio = (nivelAtual == 0) ? enderecoAtual : enderecoLocal;

    if (token.simbolo == SVAR) {
        analisaEtapaVariaveis();
    }

    int fim = (nivelAtual == 0) ? enderecoAtual : enderecoLocal;
    
    // --- CORREÇÃO DE ALOCAÇÃO ---
    if (nivelAtual == 0) {
        varsAlocadas = fim - inicio; // Global: aloca o que cresceu
    } else {
        varsAlocadas = fim; // Local: aloca tudo até o topo (inclui slot 0 de retorno)
    }

    if (varsAlocadas > 0) {
        // Usa 0 no primeiro arg para garantir formatação correta
        gera("ALLOC", 0, varsAlocadas, NULL);
    }

    if (token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        analisaSubrotinas();
    }

    analisaComandos();

    if (varsAlocadas > 0) {
        gera("DALLOC", 0, varsAlocadas, NULL);
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
        
        // Lógica de endereço: Global vs Local
        int addr = (nivelAtual == 0) ? enderecoAtual++ : enderecoLocal++;
        insere_tabela(token.lexema, "variavel", addr); 

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
        
        // Marca nível e reseta endereço local para 0
        gera("NIVEL", nivelAtual, -1, NULL);
        enderecoLocal = 0; 
        
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
    int rotuloFunc = -1;
    getToken(); // 'funcao'
    if(token.simbolo == SIDENTIFICADOR) {
        char nomeFuncao[50];
        strcpy(nomeFuncao, token.lexema);
        
        getToken();
        if(token.simbolo == SDOISPONTOS) {
            getToken();
            
            rotuloFunc = novoRotulo();
            
            if(token.simbolo == SINTEIRO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) erro_semantico("Duplicidade", nomeFuncao);
                insere_tabela(nomeFuncao, "funcao inteiro", rotuloFunc);
            } else if (token.simbolo == SBOOLEANO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) erro_semantico("Duplicidade", nomeFuncao);
                insere_tabela(nomeFuncao, "funcao booleano", rotuloFunc);
            } else {
                erro_sintatico("Tipo de retorno invalido");
            }

            geraRotulo(rotuloFunc);
            entra_escopo(); 

            gera("NIVEL", nivelAtual, -1, NULL);
            // Variáveis locais começam no 1 (0 é retorno)
            enderecoLocal = 1; 

            getToken();
            
            if(token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
            } else {
                erro_sintatico("Ponto e virgula esperado");
            }

            // Função termina com RETF (Retorna Valor)
            gera("RETF", -1, -1, NULL);

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

    int indice = consulta_tabela(nomeId);
    if (indice == -1) {
        erro_semantico("Identificador nao declarado", nomeId);
        while (token.simbolo != SPONTOVIRGULA && token.simbolo != SFIM && strlen(token.lexema) > 0) getToken();
        return;
    }
    
    char tipo[30]; strcpy(tipo, tabelaSimbolos[indice].tipo);
    int nivel = tabelaSimbolos[indice].nivel;
    int addr = tabelaSimbolos[indice].endereco;

    getToken();
    
    if(token.simbolo == SATRIBUICAO){ 
        if (strcmp(tipo, "procedimento") == 0 || strcmp(tipo, "programa") == 0) {
            erro_semantico("Atribuicao invalida", nomeId);
        }

        getToken();
        analisaExpressao(); 

        if (strncmp(tipo, "funcao", 6) == 0) {
            // Atribuição p/ função: salva no offset 0 do nível atual
            gera("STS", 0, nivelAtual, NULL); 
        } else {
            if (nivel == 0) {
                gera("STR", addr, -1, NULL);
            } else {
                gera("STS", addr, nivel, NULL);
            }
        }
    } else { // Chamada
        if (strcmp(tipo, "procedimento") != 0) {
            erro_semantico("Chamada invalida", nomeId);
        }
        gera("CALL", addr, -1, NULL);
    }
}

// ----------------- leia / escreva -----------------
void analisaLeitura() {
    getToken(); 
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            // Salva o nome da variável ANTES de ler o próximo token
            char nomeVar[50];
            strcpy(nomeVar, token.lexema);

            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            } else {
                int nivel = tabelaSimbolos[indice].nivel;
                int addr = tabelaSimbolos[indice].endereco;
                
                getToken(); // Lê o ')'
                if(token.simbolo == SFECHAPARENTESES) {
                    if (nivel == 0) {
                        // Passa 'nomeVar' salvo, não token.lexema (que agora é ")")
                        gera("RD", addr, -1, nomeVar);
                    } else {
                        gera("RDS", addr, nivel, nomeVar);
                    }
                    getToken(); // Consome o ')'
                } else erro_sintatico("Fecha parenteses esperado");
            }
        } else erro_sintatico("Identificador esperado");
    } else erro_sintatico("Abre parenteses esperado");
}

void analisaEscrita() {
    getToken(); 
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            } else {
                int nivel = tabelaSimbolos[indice].nivel;
                int addr = tabelaSimbolos[indice].endereco;
                
                // Se for função, chama e imprime retorno
                if (strncmp(tabelaSimbolos[indice].tipo, "funcao", 6) == 0) {
                    gera("CALL", addr, -1, NULL);
                    // O RETF ja deixa o valor no topo, entao PRN imprime direto.
                    // Não precisamos de LDV 0.
                } else {
                    if (nivel == 0) gera("LDV", addr, -1, NULL);
                    else gera("LDS", addr, nivel, NULL);
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
        int indice = consulta_tabela(token.lexema);
        if (indice == -1) {
            erro_semantico("Identificador nao declarado", token.lexema);
            getToken();
            return -1;
        }
        
        const char* tipoId = tabelaSimbolos[indice].tipo;

        if (strcmp(tipoId, "inteiro") == 0) {
            tipo = 0;
            if (tabelaSimbolos[indice].nivel == 0) gera("LDV", tabelaSimbolos[indice].endereco, -1, NULL);
            else gera("LDS", tabelaSimbolos[indice].endereco, tabelaSimbolos[indice].nivel, NULL);

        } else if (strcmp(tipoId, "booleano") == 0) {
            tipo = 1;
            if (tabelaSimbolos[indice].nivel == 0) gera("LDV", tabelaSimbolos[indice].endereco, -1, NULL);
            else gera("LDS", tabelaSimbolos[indice].endereco, tabelaSimbolos[indice].nivel, NULL);

        } else if (strcmp(tipoId, "funcao inteiro") == 0) {
            tipo = 0;
            // Chamada de função em expressão: CALL + RETF já resolve, não precisa LDV 0
            gera("CALL", tabelaSimbolos[indice].endereco, -1, NULL);
        } else if (strcmp(tipoId, "funcao booleano") == 0) {
            tipo = 1;
            gera("CALL", tabelaSimbolos[indice].endereco, -1, NULL);
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