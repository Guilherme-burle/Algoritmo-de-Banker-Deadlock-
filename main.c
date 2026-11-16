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

int main(int argc, char *argv[]) {
    int recursos = argc - 1;
    int clientes = 0;
    int *available = NULL;
    int **max = NULL;
    char *linha = NULL;
    size_t capacidade = 0;
    ssize_t tamanho;
    FILE *arquivo_clientes = NULL;
    FILE *arquivo_comandos = NULL;
    int quantidade_comandos = 0;
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

        if (valores == NULL) {
            fprintf(stderr, "Erro ao alocar memoria.\n");
            goto finalizar;
        }

        if (!ler_cliente(linha, recursos, valores)) {
            free(valores);
            fprintf(stderr, "Erro: formato invalido em customer.txt.\n");
            goto finalizar;
        }

        int **nova_matriz = realloc(max, (clientes + 1) * sizeof(int *));

        if (nova_matriz == NULL) {
            free(valores);
            fprintf(stderr, "Erro ao alocar memoria.\n");
            goto finalizar;
        }

        max = nova_matriz;
        max[clientes] = valores;
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

    printf("=== DADOS CARREGADOS ===\n");
    printf("Quantidade de recursos: %d\n", recursos);
    printf("Quantidade de clientes: %d\n", clientes);

    printf("\nRecursos disponiveis:\n");

    for (int i = 0; i < recursos; i++) {
        printf("%d ", available[i]);
    }

    printf("\n\nMatriz MAX:\n");

    for (int i = 0; i < clientes; i++) {
        printf("Cliente %d: ", i);

        for (int j = 0; j < recursos; j++) {
            printf("%d ", max[i][j]);
        }

        printf("\n");
    }

    printf("\nQuantidade de comandos encontrados: %d\n", quantidade_comandos);
    printf("\nLeitura dos arquivos concluida com sucesso!\n");

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
    }

    free(max);
    free(available);
    free(linha);

    return sucesso ? EXIT_SUCCESS : EXIT_FAILURE;
}