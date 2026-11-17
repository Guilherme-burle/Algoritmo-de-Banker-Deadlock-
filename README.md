# Algoritmo de Banker

Implementação do algoritmo de Banker em C para prevenção de deadlocks, permitindo controlar a alocação e liberação de recursos entre diferentes clientes.

## Compilação

Para compilar o projeto, execute na pasta principal:

```bash
make
```

Para limpar os arquivos gerados pela compilação:

```bash
make clean
```

## Execução

Execute o programa informando a quantidade total de instâncias de cada tipo de recurso como argumentos:

```bash
./banker 10 5 7
```

Nesse exemplo, o sistema possui três tipos de recursos, com 10, 5 e 7 instâncias, respectivamente.

## Arquivos de entrada

**customer.txt**

Contém a necessidade máxima de recursos de cada cliente. Cada linha representa um cliente, e os valores são separados por vírgulas.

Exemplo:

```text
7,5,3
3,2,2
9,0,2
2,2,2
4,3,3
```

**commands.txt**

Contém a sequência de operações que serão executadas pelo programa.

Os comandos disponíveis são:

- `RQ`: solicita recursos para um cliente.
- `RL`: libera recursos de um cliente.
- `*`: exibe o estado atual do sistema.

Exemplo:

```text
RQ 0 0 2 1
RL 0 0 1 1
*
```

## Arquivos do projeto

- `main.c`: implementação do algoritmo de Banker.
- `customer.txt`: dados de necessidade máxima dos clientes.
- `commands.txt`: operações de solicitação e liberação de recursos.
- `result.txt`: arquivo gerado pelo programa com os resultados da execução.
- `evidencias.log`: registro dos testes realizados durante o desenvolvimento.
- `Makefile`: comandos para compilar e limpar o projeto.

## Resultado

Após a execução, os resultados são gravados automaticamente no arquivo `result.txt`, incluindo as operações realizadas, as requisições aceitas ou negadas e o estado dos recursos quando solicitado pelo comando `*`.