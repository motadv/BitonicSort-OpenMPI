#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
    int N = 50000000;                            // Defina o tamanho da lista aqui
    int *lista = (int *)malloc(N * sizeof(int)); // Aloque espaço para a lista

    if (lista == NULL)
    {
        fprintf(stderr, "Falha na alocação de memória.\n");
        return 1;
    }

    // Inicialize o gerador de números aleatórios com uma semente
    srand(time(NULL));

    // Gere números aleatórios e os armazene na lista
    for (int i = 0; i < N; i++)
    {
        lista[i] = rand();
    }

    // Abra um arquivo para salvar os números
    FILE *arquivo = fopen("numeros.txt", "w");

    if (arquivo == NULL)
    {
        fprintf(stderr, "Não foi possível abrir o arquivo para escrita.\n");
        return 1;
    }

    // Escreva o tamanho da lista na primeira linha
    fprintf(arquivo, "%d\n", N);

    // Escreva os números aleatórios no arquivo
    for (int i = 0; i < N; i++)
    {
        fprintf(arquivo, "%d\n", lista[i]);
    }

    // Feche o arquivo
    fclose(arquivo);

    // Libere a memória alocada para a lista
    free(lista);

    printf("Números gerados e salvos no arquivo 'numeros.txt'.\n");

    return 0;
}
