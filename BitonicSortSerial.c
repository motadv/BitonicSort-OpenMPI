#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <mpi.h>

void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void compAndSwap(int a[], int i, int j, int dir)
{
    if ((a[i] > a[j] && dir == 1) || (a[i] < a[j] && dir == 0))
    {
        swap(&a[i], &a[j]);
    }
}

void bitonicMerge(int a[], int low, int cnt, int dir)
{
    if (cnt > 1)
    {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++)
        {
            compAndSwap(a, i, i + k, dir);
        }
        bitonicMerge(a, low, k, dir);
        bitonicMerge(a, low + k, k, dir);
    }
}

void bitonicSort(int a[], int low, int cnt, int dir)
{
    if (cnt > 1)
    {
        int k = cnt / 2;
        bitonicSort(a, low, k, 1);
        bitonicSort(a, low + k, k, 0);
        bitonicMerge(a, low, cnt, dir);
    }
}

void sort(int a[], int N, int up)
{
    bitonicSort(a, 0, N, up);
}

int main(int argc, char *argv[])
{

    int num_processos, process_rank;
    // Le um arquivo de entrava passado como argumento para o programa
    // e salva os números em um array
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processos);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    if (num_processos > 1)
    {
        printf("O programa deve ser executado com apenas um processo.\n");
        MPI_Finalize();
        return 0;
    }

    if (argc != 2)
    {
        printf("Uso: %s <arquivo_de_entrada>\n", argv[0]);
        MPI_Finalize();
        return 0;
    }

    FILE *arquivo = fopen(argv[1], "r");

    if (arquivo == NULL)
    {
        printf("Não foi possível abrir o arquivo '%s'.\n", argv[1]);
        MPI_Finalize();
        return 0;
    }

    int n;
    fscanf(arquivo, "%d", &n);

    int *arr = (int *)malloc(n * sizeof(int));

    if (arr == NULL)
    {
        printf("Falha na alocação de memória.\n");
        MPI_Finalize();
        return 0;
    }

    for (int i = 0; i < n; i++)
    {
        fscanf(arquivo, "%d", &arr[i]);
    }

    fclose(arquivo);

    double timer_start = MPI_Wtime();
    sort(arr, n, 1);
    double timer_end = MPI_Wtime();

    printf("Tempo de execuão: %f segundos\n", timer_end - timer_start);

    MPI_Finalize();
    return 0;
}
