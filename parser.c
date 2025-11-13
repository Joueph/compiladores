// parser.c
#include "parser.h"
#include "globals.h" // Importa 'token'
#include "gerador.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Função para reportar erros sintáticos e terminar
void erro_sintatico(const char* mensagem) {
    printf("Erro Sintatico: %s. Ultimo token lido: '%s'\n", mensagem, token.lexema);
    exit(1);
}

// Inicia a análise
void analisadorSintatico() {
    gera("START", -1, -1); // Gera a primeira instrução MVD
    getToken(); // Pega o primeiro token
    analisaPrograma();
    gera("HLT", -1, -1);
    printf("Analises de erros concluidas com sucesso!\n");
}

// <programa> ::= programa <identificador>; <bloco>.
void analisaPrograma() {
    if (token.simbolo == SPROGRAMA) {
        getToken();
        if (token.simbolo == SIDENTIFICADOR) {
            // Ação Semântica: Insere o nome do programa na tabela
            insere_tabela(token.lexema, "programa", -1);
            getToken();
            if (token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
                if (token.simbolo != SPONTO) {
                    erro_sintatico("Ponto final esperado no fim do programa");
                }

                getToken(); 
                
                // Verifica se há lixo depois do '.'
                // Se o lexema não estiver vazio, significa que o lexer encontrou algo
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

// <bloco> ::= [<etapa de declaração de variáveis>] [<etapa de declaração de sub-rotinas>] <comandos>
void analisaBloco() {
    int enderecoInicioBloco = enderecoAtual;
    int varsAlocadas = 0;

    if (token.simbolo == SVAR) {
        analisaEtapaVariaveis();
    }

    varsAlocadas = enderecoAtual - enderecoInicioBloco;

    if (varsAlocadas > 0) {
        gera("ALLOC", 0, varsAlocadas);
    }

    if (token.simbolo == SPROCEDIMENTO || token.simbolo == SFUNCAO) {
        analisaSubrotinas();
    }
    analisaComandos();

    if (varsAlocadas > 0) {
        gera("DALLOC", 0, varsAlocadas);
    }
}

// <etapa de declaração de variáveis> ::= var <declaração de variáveis> ; {<declaração de variáveis>;}
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

// <declaração de variáveis>::= <identificador> {, <identificador>} : <tipo>
void analisaDeclaracaoVariaveis() {
    while(token.simbolo == SIDENTIFICADOR) {
        // Ação Semântica: Verifica duplicidade e insere
        if (consulta_duplicidade_escopo(token.lexema)) {
            erro_semantico("Identificador ja declarado neste escopo", token.lexema);
        }
        insere_tabela(token.lexema, "variavel", enderecoAtual); // Tipo temporário
        enderecoAtual++;

        getToken();
        if (token.simbolo == SVIRGULA) {
            getToken();
        } else if (token.simbolo == SDOISPONTOS) {
            break; 
        } else {
             erro_sintatico("Virgula ou dois-pontos esperado na declaracao de variavel");
        }
    }
    if(token.simbolo == SDOISPONTOS) {
        getToken();
        analisaTipo();
    } else {
        erro_sintatico("Dois-pontos esperado apos identificadores de variaveis");
    }
}

// <tipo> ::= (inteiro | booleano)
void analisaTipo() {
    if(token.simbolo == SINTEIRO) {
        // Ação Semântica: Atualiza o tipo na tabela
        coloca_tipo_tabela("inteiro");
        getToken();
    } else if (token.simbolo == SBOOLEANO) {
        // Ação Semântica: Atualiza o tipo na tabela
        coloca_tipo_tabela("booleano");
        getToken();
    } else {
        erro_sintatico("Tipo 'inteiro' ou 'booleano' esperado");
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
            getToken();
        } else {
            erro_sintatico("Ponto e virgula esperado no final da sub-rotina");
        }
    }
}

// <declaração de procedimento> ::= procedimento <identificador>; <bloco>
void analisaDeclaracaoProcedimento() {

    int rotuloProc = novoRotulo();

    getToken(); // Consome 'procedimento'
    if(token.simbolo == SIDENTIFICADOR) {
        // Ação Semântica: Verifica duplicidade e insere
        if (consulta_duplicidade_escopo(token.lexema)) {
            erro_semantico("Identificador ja declarado neste escopo", token.lexema);
        }
        insere_tabela(token.lexema, "procedimento", rotuloProc);
        geraRotulo(rotuloProc); //"L1 NULL"
        
        entra_escopo(); // Ação Semântica: Entra em um novo nível de escopo
        getToken();
        
        if(token.simbolo == SPONTOVIRGULA) {
            getToken();
            analisaBloco();
        } else {
            erro_sintatico("Ponto e virgula esperado apos nome do procedimento");
        }

        gera ("RETURN", -1, -1);
        
        sai_escopo(); // Ação Semântica: Sai do escopo
    } else {
        erro_sintatico("Identificador esperado para nome de procedimento");
    }
}

// <declaração de função> ::= funcao <identificador>: <tipo>; <bloco>
void analisaDeclaracaoFuncao() {
    int rotuloFunc = -1;
    getToken(); // Consome 'funcao'
    if(token.simbolo == SIDENTIFICADOR) {
        char nomeFuncao[50];
        strcpy(nomeFuncao, token.lexema);
        
        getToken();
        if(token.simbolo == SDOISPONTOS) {
            getToken();
            
            // Ação Semântica: Define o tipo e insere na tabela
            rotuloFunc = novoRotulo();
            if(token.simbolo == SINTEIRO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) {
                     erro_semantico("Identificador ja declarado neste escopo", nomeFuncao);
                }
                insere_tabela(nomeFuncao, "funcao inteiro", rotuloFunc);
            } else if (token.simbolo == SBOOLEANO) {
                if (consulta_duplicidade_escopo(nomeFuncao)) {
                     erro_semantico("Identificador ja declarado neste escopo", nomeFuncao);
                }
                insere_tabela(nomeFuncao, "funcao booleano", rotuloFunc);
            } else {
                erro_sintatico("Tipo de retorno 'inteiro' ou 'booleano' esperado para a funcao");
            }

            geraRotulo(rotuloFunc);

            entra_escopo(); // Ação Semântica: Entra no escopo da função
            getToken();
            
            if(token.simbolo == SPONTOVIRGULA) {
                getToken();
                analisaBloco();
            } else {
                erro_sintatico("Ponto e virgula esperado apos tipo de retorno da funcao");
            }

            gera("RETURN", -1, -1);

            sai_escopo(); // Ação Semântica: Sai do escopo da função
        } else {
            erro_sintatico("Dois-pontos esperado apos nome da funcao");
        }
    } else {
        erro_sintatico("Identificador esperado para nome da funcao");
    }
}


// <comandos>::= inicio <comando>{;<comando>}[;] fim
void analisaComandos() {
    if (token.simbolo == SINICIO) {
        getToken();
        analisaComandoSimples();
        while (token.simbolo != SFIM) {
            if (token.simbolo == SPONTOVIRGULA) {
                getToken();
                if (token.simbolo != SFIM) { 
                   analisaComandoSimples();
                }
            } else {
                erro_sintatico("Ponto e virgula esperado entre comandos");
            }
        }
        getToken(); // Consome 'fim'
    } else {
        erro_sintatico("'inicio' esperado para o bloco de comandos");
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
    } else if (token.simbolo == SINICIO) {
        analisaComandos();
    } else {
        erro_sintatico("Comando invalido");
    }
}

// <atribuição_chprocedimento>
void analisaAtribuicaoOuChamadaProcedimento(){
    char nomeId[50];
    strcpy(nomeId, token.lexema);

    // Ação Semântica: Verifica se o ID foi declarado
    int indice = consulta_tabela(nomeId);
    if (indice == -1) {
        erro_semantico("Identificador nao declarado", nomeId);
    }
    
    getToken();
    
    if(token.simbolo == SATRIBUICAO){ // Atribuição

        int enderecoVar = tabelaSimbolos[indice].endereco;

        // Ação Semântica: Verifica se é uma variável ou função
        const char* tipoId = tabelaSimbolos[indice].tipo;
        if (strcmp(tipoId, "procedimento") == 0 || strcmp(tipoId, "programa") == 0) {
            erro_semantico("Nao se pode atribuir valor a um procedimento ou programa", nomeId);
        }
        
        int tipoVariavel = get_tipo_simbolo(nomeId); // 0=int, 1=bool
        
        getToken();
        int tipoExpressao = analisaExpressao(); // 0=int, 1=bool

        // Ação Semântica: Verificação de tipos
        if (tipoVariavel != tipoExpressao) {
            erro_semantico("Tipos incompativeis na atribuicao", nomeId);
        }

        gera ("STR", 0, enderecoVar); 

    } else { // Chamada de Procedimento
        // Ação Semântica: Verifica se é um procedimento
        if (strcmp(tabelaSimbolos[indice].tipo, "procedimento") != 0) {
            erro_semantico("Chamada de procedimento invalida, identificador nao e um procedimento", nomeId);
        }
        // Se fosse uma chamada de função, seria tratada em 'analisaFator'
        gera("CALL", tabelaSimbolos[indice].endereco, -1);
    }
}

// <comando leitura> ::= leia (<identificador>)
void analisaLeitura() {
    int enderecoVar = -1;
    getToken(); // Consome 'leia'
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
            // Ação Semântica
            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            }
            if (strcmp(tabelaSimbolos[indice].tipo, "inteiro") != 0) {
                erro_semantico("Comando 'leia' so aceita variaveis do tipo inteiro", token.lexema);
            }

            enderecoVar = tabelaSimbolos[indice].endereco;
            
            getToken();
            if(token.simbolo == SFECHAPARENTESES) {
                gera("RD", -1, -1);
                gera("STR", 0, enderecoVar);
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

// <comando escrita> ::= escreva (<identificador>)
void analisaEscrita() {
    int enderecoVar = -1;
    getToken(); // Consome 'escreva'
    if(token.simbolo == SABREPARENTESES) {
        getToken();
        if(token.simbolo == SIDENTIFICADOR) {
             // Ação Semântica
            int indice = consulta_tabela(token.lexema);
            if (indice == -1) {
                erro_semantico("Identificador nao declarado", token.lexema);
            }
            // A gramática só permite ID, vamos checar se é 'inteiro' ou 'funcao inteiro'
             if (strcmp(tabelaSimbolos[indice].tipo, "inteiro") != 0 &&
                 strcmp(tabelaSimbolos[indice].tipo, "funcao inteiro") != 0) {
                erro_semantico("Comando 'escreva' so aceita variavel ou funcao do tipo inteiro", token.lexema);
            }

            enderecoVar = tabelaSimbolos[indice].endereco;

            getToken();
            if(token.simbolo == SFECHAPARENTESES) {
                gera("LDV", 0, enderecoVar);
                gera("PRN", -1, -1);
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

// <comando enquanto> ::= enquanto <expressão> faca <comando>
void analisaEnquanto() {
    int rotuloInicio = novoRotulo();
    int rotuloFim = novoRotulo();
    geraRotulo(rotuloInicio); //gera L1 null

    getToken(); // Consome 'enquanto'
    
    // Ação Semântica: Expressão do 'enquanto' deve ser booleana
    int tipoExpressao = analisaExpressao();
    if (tipoExpressao != 1) { // 1 = booleano
        erro_semantico("Expressao booleana esperada no comando 'enquanto'", "");
    }

    gera("JMPF", 0, rotuloFim);

    if(token.simbolo == SFACA) {
        getToken();
        analisaComandoSimples();
        gera("JMP", 0, rotuloInicio);
        geraRotulo(rotuloFim); //gera L2 null

    } else {
        erro_sintatico("'faca' esperado no comando 'enquanto'");
    }
}

// <comando condicional>::= se <expressão> entao <comando> [senao <comando>]
void analisaSe() {
    int rotuloSenao = -1, rotuloFimSe = -1;
    int rotuloFim = -1;

    getToken(); // Consome 'se'
    
    // Ação Semântica: Expressão do 'se' deve ser booleana
    int tipoExpressao = analisaExpressao();
    if (tipoExpressao != 1) { // 1 = booleano
        erro_semantico("Expressao booleana esperada no comando 'se'", "");
    }

    rotuloSenao = novoRotulo();
    gera ("JMPF", 0, rotuloSenao); //JMPF L1
    
    if(token.simbolo == ENTAO) {
        getToken();
        analisaComandoSimples();
        if(token.simbolo == SSENAO) {
            rotuloFimSe = novoRotulo();
            gera("JMP", 0, rotuloFimSe);
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

// <expressão>::= <expressão simples> [<operador relacional><expressão simples>]
// Retorna 0 para inteiro, 1 para booleano
int analisaExpressao() {
    int tipo = analisaExpressaoSimples();
    if (token.simbolo == SIGUAL || token.simbolo == SDIFERENTE ||
        token.simbolo == SMENOR || token.simbolo == SMENORIGUAL ||
        token.simbolo == SMAIOR || token.simbolo == SMAIORIGUAL) {
        
        getToken();
        int tipo2 = analisaExpressaoSimples();

        // Ação Semântica: Tipos em comparação relacional devem ser iguais
        if (tipo != tipo2) {
            erro_semantico("Tipos incompativeis em expressao relacional", "");
        }
        return 1; // Resultado de uma expressão relacional é sempre booleano
    }
    return tipo; // Se não for relacional, o tipo é o da expressão simples
}

// <expressão simples> ::= [+|-] <termo> {(+|-|ou) <termo> }
// Retorna 0 para inteiro, 1 para booleano
int analisaExpressaoSimples() {
    int tipo;
    if(token.simbolo == SMAIS || token.simbolo == SMENOS) {
        Simbolo opUnario = token.simbolo;
        getToken();
        tipo = analisaTermo();
        if (tipo != 0) { // 0 = inteiro
            erro_semantico("Operador unario '+' ou '-' so pode ser usado com tipo inteiro", "");
        }

        if (opUnario == SMENOS) {
            gera("INV", -1, -1);
        }

    } else {
        tipo = analisaTermo();
    }

    while(token.simbolo == SMAIS || token.simbolo == SMENOS || token.simbolo == SOU) {
        Simbolo op = token.simbolo;
        getToken();
        int tipo2 = analisaTermo();

        // Ação Semântica: Verificação de tipos
        if (op == SOU) {
            if (tipo != 1 || tipo2 != 1) {
                erro_semantico("Operador 'ou' so pode ser usado com expressoes booleanas", "");
            }
            tipo = 1; // Resultado é booleano
            gera ("OR", -1, -1);
        } else { // + ou -
             if (tipo != 0 || tipo2 != 0) {
                erro_semantico("Operador '+' ou '-' so pode ser usado com expressoes inteiras", "");
            }
            if (op == SMAIS) gera("ADD", -1, -1);
            else gera("SUB", -1, -1);
            tipo = 0; // Resultado é inteiro
        }
    }
    return tipo;
}

// <termo>::= <fator> {(* | div | e) <fator>}
// Retorna 0 para inteiro, 1 para booleano
int analisaTermo() {
    int tipo = analisaFator();
    
    while(token.simbolo == SMULT || token.simbolo == SDIV || token.simbolo == SE) {
        Simbolo op = token.simbolo;
        getToken();
        int tipo2 = analisaFator();
        
        // Ação Semântica: Verificação de tipos
        if (op == SE) {
            if (tipo != 1 || tipo2 != 1) {
                erro_semantico("Operador 'e' so pode ser usado com expressoes booleanas", "");
            }
            tipo = 1; // Resultado é booleano
            gera("AND", -1, -1);
        } else { // * ou div
             if (tipo != 0 || tipo2 != 0) {
                erro_semantico("Operador '*' ou 'div' so pode ser usado com expressoes inteiras", "");
            }
            if (op == SMULT) gera ("MULT", -1, -1);
            else gera ("DIVI", -1, -1);
            tipo = 0; // Resultado é inteiro
        }
    }
    return tipo;
}

// <fator> ::= (<variável> | <número> | <chamada de função> | (<expressão>) | verdadeiro | falso | nao <fator>)
// Retorna 0 para inteiro, 1 para booleano
int analisaFator() {
    int tipo = -1; 
    
    if (token.simbolo == SIDENTIFICADOR) {
        int indice = consulta_tabela(token.lexema);
        if (indice == -1) {
            erro_semantico("Identificador nao declarado", token.lexema);
        }
        
        const char* tipoId = tabelaSimbolos[indice].tipo;
        if (strcmp(tipoId, "inteiro") == 0) {
            tipo = 0;
            gera("LDV", 0, tabelaSimbolos[indice].endereco);
        } else if (strcmp(tipoId, "booleano") == 0) {
            tipo = 1;
            gera("LDV", 0, tabelaSimbolos[indice].endereco);
        } else if (strcmp(tipoId, "funcao inteiro") == 0) {
            tipo = 0;
            gera("CALL", tabelaSimbolos[indice].endereco, -1);
        } else if (strcmp(tipoId, "funcao booleano") == 0) {
            tipo = 1;
            gera("CALL", tabelaSimbolos[indice].endereco, -1);
        } else {
            erro_semantico("Identificador nao pode ser usado em uma expressao (deve ser var ou funcao)", token.lexema);
        }
        
        getToken();

    } else if (token.simbolo == SNUMERO) {
        tipo = 0; // Números são inteiros
        gera("LDC", atoi(token.lexema), -1); 
        getToken();

    } else if (token.simbolo == SNAO) {
        getToken();
        tipo = analisaFator();
        if (tipo != 1) {
            erro_semantico("Operador 'nao' so pode ser usado com tipo booleano", "");
        }
        tipo = 1; // Resultado é booleano
        gera("NEG", -1, -1); 

    } else if (token.simbolo == SABREPARENTESES) {
        getToken();
        tipo = analisaExpressao();
        if (token.simbolo == SFECHAPARENTESES) {
            getToken();
        } else {
            erro_sintatico("Fecha parenteses esperado na expressao");
        }
    } else if(token.simbolo == SVERDADEIRO || token.simbolo == SFALSO) {
        tipo = 1; // 'verdadeiro' e 'falso' são booleanos
        if (token.simbolo == SVERDADEIRO) gera("LDC", 1, -1); 
        else gera("LDC", 0, -1); 
        getToken();
    } else {
        erro_sintatico("Fator inesperado na expressao");
    }
    
    return tipo;
}