#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

int converter_inteiro(const char *texto, int *valor) {
    char *fim;
    long numero;

    errno = 0;
    numero = strtol(texto, &fim, 10);

    if (texto == fim || *fim != '\0' || errno == ERANGE || numero <= 0 || numero > INT_MAX)
        return 0;

    *valor = (int)numero;
    return 1;
}

int converter_nao_negativo(const char *texto, int *valor) {
    char *fim;
    long numero;

    errno = 0;
    numero = strtol(texto, &fim, 10);

    if (texto == fim || *fim != '\0' || errno == ERANGE || numero < 0 || numero > INT_MAX)
        return 0;

    *valor = (int)numero;
    return 1;
}

int ler_cliente(char *linha, int recursos, int *vetor) {
    char *token;
    char *resto;
    int i = 0;

    token = strtok_r(linha, ",\r\n", &resto);

    while (token != NULL && i < recursos) {
        while (isspace((unsigned char)*token))
            token++;

        char *fim = token + strlen(token) - 1;

        while (fim >= token && isspace((unsigned char)*fim))
            *fim-- = '\0';

        if (!converter_nao_negativo(token, &vetor[i]))
            return 0;

        i++;
        token = strtok_r(NULL, ",\r\n", &resto);
    }

    return i == recursos && token == NULL;
}

int verificar_seguranca(int clientes, int recursos, int **allocation, int **need, int *available, int mostrar_sequencia) {
    int *work = malloc(recursos * sizeof(int));
    int *finish = calloc(clientes, sizeof(int));
    int *sequencia = malloc(clientes * sizeof(int));

    if (work == NULL || finish == NULL || sequencia == NULL) {
        free(work);
        free(finish);
        free(sequencia);
        return -1;
    }

    for (int j = 0; j < recursos; j++)
        work[j] = available[j];

    int quantidade = 0;
    int progresso = 1;

    while (quantidade < clientes && progresso) {
        progresso = 0;

        for (int i = 0; i < clientes; i++) {
            if (finish[i])
                continue;

            int pode_executar = 1;

            for (int j = 0; j < recursos; j++) {
                if (need[i][j] > work[j]) {
                    pode_executar = 0;
                    break;
                }
            }

            if (pode_executar) {
                for (int j = 0; j < recursos; j++)
                    work[j] += allocation[i][j];

                finish[i] = 1;
                sequencia[quantidade++] = i;
                progresso = 1;
            }
        }
    }

    int seguro = quantidade == clientes;

    if (mostrar_sequencia) {
        if (seguro) {
            printf("Estado seguro. Sequencia segura: ");

            for (int i = 0; i < quantidade; i++) {
                printf("%d", sequencia[i]);

                if (i < quantidade - 1)
                    printf(" -> ");
            }

            printf("\n");
        } else {
            printf("Estado inseguro.\n");
        }
    }

    free(work);
    free(finish);
    free(sequencia);

    return seguro;
}

void imprimir_vetor(FILE *saida, int *vetor, int recursos) {
    for (int j = 0; j < recursos; j++)
        fprintf(saida, "%s%d", j == 0 ? "" : " ", vetor[j]);
}

void imprimir_estado(FILE *saida, int clientes, int recursos, int **max, int **allocation, int **need, int *available) {
    fprintf(saida, "MAXIMUM | ALLOCATION | NEED\n");

    for (int i = 0; i < clientes; i++) {
        imprimir_vetor(saida, max[i], recursos);
        fprintf(saida, " | ");
        imprimir_vetor(saida, allocation[i], recursos);
        fprintf(saida, " | ");
        imprimir_vetor(saida, need[i], recursos);
        fprintf(saida, "\n");
    }

    fprintf(saida, "AVAILABLE ");
    imprimir_vetor(saida, available, recursos);
    fprintf(saida, "\n");
}

int ler_comando(char *linha, int recursos, char *tipo, int *cliente, int *vetor) {
    char *token;
    char *resto;
    int quantidade = 0;
    int valor;

    token = strtok_r(linha, " \t\r\n", &resto);

    if (token == NULL)
        return 0;

    if (strcmp(token, "*") == 0) {
        if (strtok_r(NULL, " \t\r\n", &resto) != NULL)
            return -1;

        *tipo = '*';
        return 3;
    }

    if (strcmp(token, "RQ") != 0 && strcmp(token, "RL") != 0)
        return -1;

    *tipo = token[1];

    token = strtok_r(NULL, " \t\r\n", &resto);

    if (token == NULL || !converter_nao_negativo(token, cliente))
        return -1;

    while ((token = strtok_r(NULL, " \t\r\n", &resto)) != NULL) {
        if (quantidade >= recursos || !converter_nao_negativo(token, &valor))
            return -1;

        vetor[quantidade++] = valor;
    }

    if (quantidade != recursos)
        return -1;

    return *tipo == 'Q' ? 1 : 2;
}

int main(int argc, char *argv[]) {
    int recursos = argc - 1;
    int clientes = 0;
    int capacidade = 10;
    int resultado;
    int cliente;
    int *total = NULL;
    int *available = NULL;
    int **max = NULL;
    int **allocation = NULL;
    int **need = NULL;
    FILE *arquivo_clientes = NULL;
    FILE *arquivo_comandos = NULL;
    FILE *arquivo_resultado = NULL;
    char *linha = NULL;
    size_t tamanho = 0;
    ssize_t lidos;

    if (argc < 2) {
        fprintf(stderr, "Uso: ./banker recurso1 recurso2 ...\n");
        return 1;
    }

    total = malloc(recursos * sizeof(int));
    available = malloc(recursos * sizeof(int));

    if (total == NULL || available == NULL) {
        fprintf(stderr, "Erro de memoria.\n");
        goto finalizar;
    }

    for (int j = 0; j < recursos; j++) {
        if (!converter_inteiro(argv[j + 1], &total[j])) {
            fprintf(stderr, "Quantidade de recursos invalida.\n");
            goto finalizar;
        }

        available[j] = total[j];
    }

    max = calloc(capacidade, sizeof(int *));
    allocation = calloc(capacidade, sizeof(int *));
    need = calloc(capacidade, sizeof(int *));

    if (max == NULL || allocation == NULL || need == NULL) {
        fprintf(stderr, "Erro de memoria.\n");
        goto finalizar;
    }

    arquivo_clientes = fopen("customer.txt", "r");

    if (arquivo_clientes == NULL) {
        fprintf(stderr, "Nao foi possivel abrir customer.txt.\n");
        goto finalizar;
    }

    while ((lidos = getline(&linha, &tamanho, arquivo_clientes)) != -1) {
        if (lidos == 0)
            continue;

        if (clientes == capacidade) {
            int nova_capacidade = capacidade * 2;
            int **novo_max = realloc(max, nova_capacidade * sizeof(int *));
            int **novo_allocation = realloc(allocation, nova_capacidade * sizeof(int *));
            int **novo_need = realloc(need, nova_capacidade * sizeof(int *));

            if (novo_max == NULL || novo_allocation == NULL || novo_need == NULL) {
                if (novo_max != NULL)
                    max = novo_max;
                if (novo_allocation != NULL)
                    allocation = novo_allocation;
                if (novo_need != NULL)
                    need = novo_need;

                fprintf(stderr, "Erro de memoria.\n");
                goto finalizar;
            }

            max = novo_max;
            allocation = novo_allocation;
            need = novo_need;
            capacidade = nova_capacidade;
        }

        max[clientes] = malloc(recursos * sizeof(int));
        allocation[clientes] = calloc(recursos, sizeof(int));
        need[clientes] = malloc(recursos * sizeof(int));

        if (max[clientes] == NULL || allocation[clientes] == NULL || need[clientes] == NULL) {
            fprintf(stderr, "Erro de memoria.\n");
            goto finalizar;
        }

        if (!ler_cliente(linha, recursos, max[clientes])) {
            fprintf(stderr, "Formato invalido em customer.txt.\n");
            goto finalizar;
        }

        for (int j = 0; j < recursos; j++)
            need[clientes][j] = max[clientes][j];

        clientes++;
    }

    fclose(arquivo_clientes);
    arquivo_clientes = NULL;

    if (clientes == 0) {
        fprintf(stderr, "Nenhum cliente encontrado em customer.txt.\n");
        goto finalizar;
    }

    arquivo_comandos = fopen("commands.txt", "r");

    if (arquivo_comandos == NULL) {
        fprintf(stderr, "Nao foi possivel abrir commands.txt.\n");
        goto finalizar;
    }

    arquivo_resultado = fopen("result.txt", "w");

    if (arquivo_resultado == NULL) {
        fprintf(stderr, "Nao foi possivel criar result.txt.\n");
        goto finalizar;
    }

    while ((lidos = getline(&linha, &tamanho, arquivo_comandos)) != -1) {
        if (lidos == 0)
            continue;

        char *copia = strdup(linha);

        if (copia == NULL) {
            fprintf(stderr, "Erro de memoria.\n");
            goto finalizar;
        }

        int *vetor = malloc(recursos * sizeof(int));

        if (vetor == NULL) {
            free(copia);
            fprintf(stderr, "Erro de memoria.\n");
            goto finalizar;
        }

        char tipo;
        resultado = ler_comando(copia, recursos, &tipo, &cliente, vetor);
        free(copia);

        if (resultado == 0) {
            free(vetor);
            continue;
        }

        if (resultado == -1) {
            fprintf(arquivo_resultado, "Comando invalido.\n");
            free(vetor);
            continue;
        }

        if (resultado == 3) {
            imprimir_estado(arquivo_resultado, clientes, recursos, max, allocation, need, available);
            free(vetor);
            continue;
        }

        if (cliente < 0 || cliente >= clientes) {
            fprintf(arquivo_resultado, "Cliente invalido na requisicao.\n");
            free(vetor);
            continue;
        }

        if (tipo == 'Q') {
            int excede_necessidade = 0;
            int recursos_insuficientes = 0;

            for (int j = 0; j < recursos; j++) {
                if (vetor[j] > need[cliente][j])
                    excede_necessidade = 1;

                if (vetor[j] > available[j])
                    recursos_insuficientes = 1;
            }

            if (excede_necessidade) {
                fprintf(arquivo_resultado, "The customer %d request ", cliente);
                imprimir_vetor(arquivo_resultado, vetor, recursos);
                fprintf(arquivo_resultado, " was denied because exceed its maximum allocation\n");
                free(vetor);
                continue;
            }

            if (recursos_insuficientes) {
                fprintf(arquivo_resultado, "The resources ");
                imprimir_vetor(arquivo_resultado, available, recursos);
                fprintf(arquivo_resultado, " are not enough to customer %d request ", cliente);
                imprimir_vetor(arquivo_resultado, vetor, recursos);
                fprintf(arquivo_resultado, "\n");
                free(vetor);
                continue;
            }

            for (int j = 0; j < recursos; j++) {
                available[j] -= vetor[j];
                allocation[cliente][j] += vetor[j];
                need[cliente][j] -= vetor[j];
            }

            resultado = verificar_seguranca(clientes, recursos, allocation, need, available, 0);

            if (resultado == 1) {
                fprintf(arquivo_resultado, "Allocate to customer %d the resources ", cliente);
                imprimir_vetor(arquivo_resultado, vetor, recursos);
                fprintf(arquivo_resultado, "\n");
            } else {
                for (int j = 0; j < recursos; j++) {
                    available[j] += vetor[j];
                    allocation[cliente][j] -= vetor[j];
                    need[cliente][j] += vetor[j];
                }

                if (resultado == 0) {
                    fprintf(arquivo_resultado, "The customer %d request ", cliente);
                    imprimir_vetor(arquivo_resultado, vetor, recursos);
                    fprintf(arquivo_resultado, " was denied because result in an unsafe state\n");
                } else {
                    fprintf(stderr, "Erro ao verificar estado de seguranca.\n");
                    free(vetor);
                    goto finalizar;
                }
            }
        } else if (tipo == 'L') {
            int excede_alocacao = 0;

            for (int j = 0; j < recursos; j++) {
                if (vetor[j] > allocation[cliente][j])
                    excede_alocacao = 1;
            }

            if (excede_alocacao) {
                fprintf(arquivo_resultado, "The customer %d release ", cliente);
                imprimir_vetor(arquivo_resultado, vetor, recursos);
                fprintf(arquivo_resultado, " was denied because exceed its allocation\n");
                free(vetor);
                continue;
            }

            for (int j = 0; j < recursos; j++) {
                available[j] += vetor[j];
                allocation[cliente][j] -= vetor[j];
                need[cliente][j] += vetor[j];
            }

            fprintf(arquivo_resultado, "Release from customer %d the resources ", cliente);
            imprimir_vetor(arquivo_resultado, vetor, recursos);
            fprintf(arquivo_resultado, "\n");
        }

        free(vetor);
    }

    fclose(arquivo_comandos);
    arquivo_comandos = NULL;

    fclose(arquivo_resultado);
    arquivo_resultado = NULL;

    printf("Resultado salvo em result.txt.\n");

finalizar:
    if (arquivo_clientes != NULL)
        fclose(arquivo_clientes);

    if (arquivo_comandos != NULL)
        fclose(arquivo_comandos);

    if (arquivo_resultado != NULL)
        fclose(arquivo_resultado);

    free(linha);
    free(total);
    free(available);

    if (max != NULL) {
        for (int i = 0; i < clientes; i++)
            free(max[i]);
    }

    if (allocation != NULL) {
        for (int i = 0; i < clientes; i++)
            free(allocation[i]);
    }

    if (need != NULL) {
        for (int i = 0; i < clientes; i++)
            free(need[i]);
    }

    free(max);
    free(allocation);
    free(need);

    return 0;
}