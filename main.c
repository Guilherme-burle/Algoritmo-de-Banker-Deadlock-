#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>

int converter_inteiro(char *texto, int *valor) {
    char *fim;
    long numero;

    errno = 0;
    numero = strtol(texto, &fim, 10);

    if (errno != 0 || texto == fim || *fim != '\0' || numero <= 0 || numero > INT_MAX) {
        return 0;
    }

    *valor = (int)numero;
    return 1;
}

int ler_cliente(char *linha, int recursos, int *valores) {
    char *posicao = linha;
    char *fim;
    long numero;

    for (int i = 0; i < recursos; i++) {
        while (isspace((unsigned char)*posicao)) {
            posicao++;
        }

        errno = 0;
        numero = strtol(posicao, &fim, 10);

        if (posicao == fim || errno != 0 || numero < 0 || numero > INT_MAX) {
            return 0;
        }

        valores[i] = (int)numero;
        posicao = fim;

        while (isspace((unsigned char)*posicao)) {
            posicao++;
        }

        if (i < recursos - 1) {
            if (*posicao != ',') {
                return 0;
            }
            posicao++;
        }
    }

    while (isspace((unsigned char)*posicao)) {
        posicao++;
    }

    return *posicao == '\0';
}

int verificar_seguranca(int **need, int **allocation, int *available, int clientes, int recursos, int *sequencia) {
    int *work = malloc(recursos * sizeof(int));
    int *finish = calloc(clientes > 0 ? clientes : 1, sizeof(int));

    if (work == NULL || finish == NULL) {
        free(work);
        free(finish);
        return -1;
    }

    for (int i = 0; i < recursos; i++) {
        work[i] = available[i];
    }

    int concluidos = 0;
    int progresso = 1;

    while (progresso) {
        progresso = 0;

        for (int i = 0; i < clientes; i++) {
            if (finish[i]) {
                continue;
            }

            int pode_terminar = 1;

            for (int j = 0; j < recursos; j++) {
                if (need[i][j] > work[j]) {
                    pode_terminar = 0;
                    break;
                }
            }

            if (pode_terminar) {
                for (int j = 0; j < recursos; j++) {
                    work[j] += allocation[i][j];
                }

                finish[i] = 1;
                sequencia[concluidos++] = i;
                progresso = 1;
            }
        }
    }

    free(work);
    free(finish);

    return concluidos == clientes;
}

int main(int argc, char *argv[]) {
    int recursos = argc - 1;
    int clientes = 0;
    int *available = NULL;
    int **max = NULL;
    int **allocation = NULL;
    int **need = NULL;
    int *sequencia = NULL;
    char *linha = NULL;
    size_t capacidade = 0;
    ssize_t tamanho;
    FILE *arquivo_clientes = NULL;
    FILE *arquivo_comandos = NULL;
    int quantidade_comandos = 0;
    int resultado_seguranca;
    int sucesso = 0;

    if (recursos <= 0) {
        fprintf(stderr, "Erro: informe a quantidade de cada recurso.\n");
        fprintf(stderr, "Uso: ./banker recurso1 recurso2 ...\n");
        goto finalizar;
    }

    available = malloc(recursos * sizeof(int));

    if (available == NULL) {
        fprintf(stderr, "Erro ao alocar memoria.\n");
        goto finalizar;
    }

    for (int i = 0; i < recursos; i++) {
        if (!converter_inteiro(argv[i + 1], &available[i])) {
            fprintf(stderr, "Erro: quantidade de recurso invalida.\n");
            goto finalizar;
        }
    }

    arquivo_clientes = fopen("customer.txt", "r");

    if (arquivo_clientes == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir customer.txt.\n");
        goto finalizar;
    }

    while ((tamanho = getline(&linha, &capacidade, arquivo_clientes)) != -1) {
        int *valores = malloc(recursos * sizeof(int));
        int *nova_alocacao = NULL;
        int *nova_necessidade = NULL;

        if (valores == NULL) {
            fprintf(stderr, "Erro ao alocar memoria.\n");
            goto finalizar;
        }

        if (!ler_cliente(linha, recursos, valores)) {
            free(valores);
            fprintf(stderr, "Erro: formato invalido em customer.txt.\n");
            goto finalizar;
        }

        int **nova_max = realloc(max, (clientes + 1) * sizeof(int *));

        if (nova_max == NULL) {
            free(valores);
            fprintf(stderr, "Erro ao alocar memoria.\n");
            goto finalizar;
        }

        max = nova_max;

        int **nova_matriz_alocacao = realloc(allocation, (clientes + 1) * sizeof(int *));

        if (nova_matriz_alocacao == NULL) {
            free(valores);
            fprintf(stderr, "Erro ao alocar memoria.\n");
            goto finalizar;
        }

        allocation = nova_matriz_alocacao;

        int **nova_matriz_necessidade = realloc(need, (clientes + 1) * sizeof(int *));

        if (nova_matriz_necessidade == NULL) {
            free(valores);
            fprintf(stderr, "Erro ao alocar memoria.\n");
            goto finalizar;
        }

        need = nova_matriz_necessidade;

        nova_alocacao = calloc(recursos, sizeof(int));
        nova_necessidade = malloc(recursos * sizeof(int));

        if (nova_alocacao == NULL || nova_necessidade == NULL) {
            free(valores);
            free(nova_alocacao);
            free(nova_necessidade);
            fprintf(stderr, "Erro ao alocar memoria.\n");
            goto finalizar;
        }

        max[clientes] = valores;
        allocation[clientes] = nova_alocacao;
        need[clientes] = nova_necessidade;

        for (int j = 0; j < recursos; j++) {
            need[clientes][j] = max[clientes][j] - allocation[clientes][j];
        }

        clientes++;
    }

    if (ferror(arquivo_clientes)) {
        fprintf(stderr, "Erro durante a leitura de customer.txt.\n");
        goto finalizar;
    }

    fclose(arquivo_clientes);
    arquivo_clientes = NULL;

    arquivo_comandos = fopen("commands.txt", "r");

    if (arquivo_comandos == NULL) {
        fprintf(stderr, "Erro: nao foi possivel abrir commands.txt.\n");
        goto finalizar;
    }

    while (getline(&linha, &capacidade, arquivo_comandos) != -1) {
        quantidade_comandos++;
    }

    if (ferror(arquivo_comandos)) {
        fprintf(stderr, "Erro durante a leitura de commands.txt.\n");
        goto finalizar;
    }

    sequencia = malloc((clientes > 0 ? clientes : 1) * sizeof(int));

    if (sequencia == NULL) {
        fprintf(stderr, "Erro ao alocar memoria.\n");
        goto finalizar;
    }

    resultado_seguranca = verificar_seguranca(need, allocation, available, clientes, recursos, sequencia);

    if (resultado_seguranca == -1) {
        fprintf(stderr, "Erro ao verificar a seguranca do sistema.\n");
        goto finalizar;
    }

    printf("=== ESTADO ATUAL DO SISTEMA ===\n");

    printf("\nMAXIMUM | ALLOCATION | NEED\n");

    for (int i = 0; i < clientes; i++) {
        for (int j = 0; j < recursos; j++) {
            printf("%d ", max[i][j]);
        }

        printf("| ");

        for (int j = 0; j < recursos; j++) {
            printf("%d ", allocation[i][j]);
        }

        printf("| ");

        for (int j = 0; j < recursos; j++) {
            printf("%d ", need[i][j]);
        }

        printf("\n");
    }

    printf("\nAVAILABLE ");

    for (int i = 0; i < recursos; i++) {
        printf("%d ", available[i]);
    }

    printf("\n");

    if (resultado_seguranca) {
        printf("\nEstado seguro.\n");
        printf("Sequencia segura: ");

        for (int i = 0; i < clientes; i++) {
            printf("%d", sequencia[i]);

            if (i < clientes - 1) {
                printf(" -> ");
            }
        }

        printf("\n");
    } else {
        printf("\nEstado inseguro: nao existe uma sequencia segura.\n");
    }

    printf("\nQuantidade de comandos encontrados: %d\n", quantidade_comandos);

    sucesso = 1;

finalizar:
    if (arquivo_clientes != NULL) {
        fclose(arquivo_clientes);
    }

    if (arquivo_comandos != NULL) {
        fclose(arquivo_comandos);
    }

    for (int i = 0; i < clientes; i++) {
        free(max[i]);
        free(allocation[i]);
        free(need[i]);
    }

    free(max);
    free(allocation);
    free(need);
    free(available);
    free(sequencia);
    free(linha);

    return sucesso ? EXIT_SUCCESS : EXIT_FAILURE;
}