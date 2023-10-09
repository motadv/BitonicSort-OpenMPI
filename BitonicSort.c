#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include "mpi.h"
#include "bitonic.h"
#include <string.h>
#include <unistd.h>

#define MASTER 0

// Variáveis de debug
int verbose = 0;
int verify = 0;

// Variáveis de tempo
double timer_start;
double timer_end;

// Variáveis MPI
int process_rank;
int num_processos;

// Variáveis de entrada
int *input_numbers;
int input_n;

// Variáveis locais
int *local_numbers;
int local_n;

// Variáveis de comunicação
int *buffer_receive;

int main(int argc, char *argv[])
{
    int i, j;

    // Inicializa o MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processos);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    // Verificação de erros
    if (num_processos < 2)
    {
        printf("O número mínimo de processos é 2.\n");
        MPI_Finalize();
        return 1;
    }

    // Verificar se foi passado o arquivo de entrada
    if (argc != 2)
    {
        if (process_rank == MASTER)
        {
            printf("Uso: %s <arquivo_de_entrada>\n", argv[0]);
        }
        MPI_Finalize();
        return 0;
    }

    // Verifica se o número de processos é uma potência de 2
    int power_of_two = 1;
    while (power_of_two < num_processos)
    {
        power_of_two *= 2;
    }
    if (power_of_two != num_processos)
    {
        printf("O número de processos deve ser uma potência de 2.\n");
        free(input_numbers);
        MPI_Finalize();
        return 1;
    }

    // Verfica se o arquivo de entrada existe
    char *filename = argv[1];
    if (access(filename, F_OK) == -1)
    {
        printf("O arquivo de entrada não existe.\n");
        MPI_Finalize();
        return 1;
    }

    if (process_rank == 0)
    {
        // Processo coordenador abre arquivo de entrada
        FILE *file = fopen(filename, "r");
        if (!file)
        {
            printf("Erro ao abrir o arquivo.\n");
            MPI_Finalize();
            return 1;
        }

        input_n = 0;
        int list_size = 0;

        // Lê o número de elementos no arquivo
        // Arquivos de numeros começam com a quantidade de elementos na primeira linha
        fscanf(file, "%d", &input_n);
        printf("Tamanho da entrada: %d \n", input_n);

        // Como o bitonic sort necessita que a quantidade de elementos seja múltipla do numero de processos
        if (input_n % num_processos == 0)
        {
            list_size = input_n;
        }
        else
        {
            list_size = input_n + (num_processos - input_n % num_processos);
        }
        // list_size = (int)pow(2, ceil(log2(input_n)));
        printf("Tamanho da lista alocada: %d \n", list_size);

        // Aloca memória para armazenar os números
        // Decidimos guardar os numeros de entrada todos no processo 0 e depois distribuir
        // Poderiamos ter lido os números em todos os processos, mas isso complicaria o código
        input_numbers = (int *)malloc(list_size * sizeof(int));

        // Lê os números do arquivo e preenche no vetor
        int i;
        for (i = 0; i < input_n; i++)
        {
            fscanf(file, "%d", &input_numbers[i]);
        }
        // Fecha o arquivo
        fclose(file);

        // Continua depois de N preenchendo com um valor lixo para ser ignorado
        // Usamos o máximo inteiro para garantir que todos esses dados ficarão no final da lista ordenada
        // Na hora de salvar a lista ordenada ou imprimir, basta ignorar os valores maiores que N
        for (i; i < list_size; i++)
        {
            input_numbers[i] = __INT_MAX__;
        }

        // Determina o tamanho do array local que cada processo precisa ter
        local_n = (list_size / num_processos);
    }

    // Processo 0 envia por broadcast o tamanho do array local de cada processo
    MPI_Bcast(&local_n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // .Todos os processos alocal o array local para dar sort
    local_numbers = (int *)malloc(local_n * sizeof(int));

    // Processo 0 divide e envia a lista de input aleatória para todos os processos
    // Cada processo aloca sua fatia da lista no array local_numbers de tamanho local_n
    MPI_Scatter(input_numbers, local_n, MPI_INT, local_numbers, local_n, MPI_INT, 0, MPI_COMM_WORLD);

    // Configurações iniciais do bitonic sort

    // Direção da ordenação do array local
    int dir;
    dir = process_rank % 2;

    // Array que cada processo usará para comparar com o seu array local
    buffer_receive = malloc((local_n) * sizeof(int));

    // Garantir que todos estão no mesmo ponto antes de começar o algorítmo
    MPI_Barrier(MPI_COMM_WORLD);

    int etapas = Log2N(num_processos);

    // Começa o algorítmo de bitonic sort
    if (process_rank == MASTER)
    {
        printf("Número de processos criados: %d\n", num_processos);
        timer_start = MPI_Wtime();
    }

    sortLocal(local_numbers, local_n, dir);

    // Para N processos, são necessárias log2(N) etapas
    for (i = 0; i < etapas; i++)
    {
        // Para cada etapa, são necessárias i + 1 comparações
        for (j = i; j >= 0; j--)
        {

            if (((process_rank >> (i + 1)) % 2 == 0 && (process_rank >> j) % 2 == 0) ||
                ((process_rank >> (i + 1)) % 2 != 0 && (process_rank >> j) % 2 != 0))
            {

                CompareLow(j);
            }
            else
            {
                CompareHigh(j);
            }
        }

        dir = (process_rank >> i + 1) % 2;
        sortLocal(local_numbers, local_n, dir);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Não há mais comparaões, então podemos liberar a memória do buffer
    free(buffer_receive);

    if (process_rank == MASTER)
    {
        timer_end = MPI_Wtime();
        printf("Tempo de execuão: %f segundos\n", timer_end - timer_start);
    }

    if (verbose)
    {
        if (process_rank == MASTER)
        {
            qsort(input_numbers, input_n, sizeof(int), ComparisonFunc);
            printf("Resultado esperado (qsort):\n");
            for (i = 0; i < input_n; i++)
            {
                printf("%d ", input_numbers[i]);
            }
            printf("\n");
        }
    }

    // Processo 0 recebe todos os arrays locais e os junta em um array global

    // Processo 0 imprime os input_n primeiros números do array global
    MPI_Gather(local_numbers, local_n, MPI_INT, input_numbers, local_n, MPI_INT, 0, MPI_COMM_WORLD);
    free(local_numbers);

    // Checa se a lista foi de fato ordenada
    if (verify)
    {

        if (process_rank == MASTER)
        {
            for (i = 0; i < input_n - 1; i++)
            {
                if (input_numbers[i] > input_numbers[i + 1])
                {
                    printf("Erro na ordenação. Elemento %d > %d\n", i, i + 1);
                    MPI_Finalize();
                    return 1;
                }
            }
        }
    }

    if (verbose)
    {
        if (process_rank == MASTER)
        {
            printf("Lista ordenada (bitonic sort):\n");
            for (i = 0; i < input_n; i++)
            {
                printf("%d ", input_numbers[i]);
            }
            printf("\n");
            free(input_numbers);
        }
    }
    else
    {
        // processo 0 salva os input_n primeiros números do array global em um arquivo de saída
        if (process_rank == MASTER)
        {
            FILE *file = fopen("output.txt", "w");
            if (!file)
            {
                printf("Erro ao abrir o arquivo.\n");
                MPI_Finalize();
                return 1;
            }

            for (i = 0; i < input_n; i++)
            {
                fprintf(file, "%d\n", input_numbers[i]);
            }
            fclose(file);
            free(input_numbers);
        }
    }

    MPI_Finalize();
    return 0;
}

void CompareLow(int bit)
{
    MPI_Send(
        local_numbers,
        local_n,
        MPI_INT,
        process_rank ^ (1 << bit),
        0,
        MPI_COMM_WORLD);

    MPI_Recv(
        buffer_receive,
        local_n,
        MPI_INT,
        process_rank ^ (1 << bit),
        0,
        MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);

    // Comparar um a um o array recebido e o meu array local
    // Se o valor for menor que o meu, substituir
    // Senão, fazer nada

    for (int i = 0; i < local_n; i++)
    {
        if (local_numbers[i] > buffer_receive[i])
        {
            local_numbers[i] = buffer_receive[i];
        }
    }

    return;
}

void CompareHigh(int bit)
{
    MPI_Recv(
        buffer_receive,
        local_n,
        MPI_INT,
        process_rank ^ (1 << bit),
        0,
        MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);

    MPI_Send(
        local_numbers,
        local_n,
        MPI_INT,
        process_rank ^ (1 << bit),
        0,
        MPI_COMM_WORLD);

    // Comparar um a um o array recebido e o meu array local
    // Se o valor for maior que o meu, substituir
    // Senão, fazer nada

    for (int i = 0; i < local_n; i++)
    {
        if (local_numbers[i] < buffer_receive[i])
        {
            local_numbers[i] = buffer_receive[i];
        }
    }

    return;
}

unsigned int Log2N(unsigned int n)
{
    return (n > 1) ? 1 + Log2N(n / 2) : 0;
}
// Função de comparação para qsort crescente
int ComparisonFunc(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}
// Função de comparação para qsort decrescente
int ComparisonFuncReverse(const void *a, const void *b)
{
    return (*(int *)b - *(int *)a);
}
// Função de sort direcional
void sortLocal(int *array, int array_size, int dir)
{
    if (dir == 0)
    {
        // Se for par, ordena crescente
        qsort(array, array_size, sizeof(int), ComparisonFunc);
    }
    else
    {
        // Se for impar, ordena decrescente
        qsort(array, array_size, sizeof(int), ComparisonFuncReverse);
    }
}
