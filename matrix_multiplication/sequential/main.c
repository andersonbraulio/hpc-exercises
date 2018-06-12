#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>

/* prototypes */
int * init_matrix(int, int);
int * init_zero_matrix(int, int);
int * init_one_matrix(int, int);

void print_matrix(char *, int *, int, int);
void print_borderless_matrix(char *, int *, int, int);

int main(int argc, char **argv) {
    int _index, _row, _column;
    int value, multiplications, rA, cA, rB, cB;
    int *mA, *mB, *mC, *row_content, *column_content;
    double initial_time, final_time, execution_time;

    srand(time(NULL));

    /* read dimensions of matrices */
    if (argc == 5 && argv[1] != NULL && argv[2] != NULL && argv[3] != NULL && argv[4] != NULL) {
        rA = abs(atoi(argv[1]));
        cA = abs(atoi(argv[2]));
        rB = abs(atoi(argv[3]));
        cB = abs(atoi(argv[4]));

        if (cA != rB)
            exit(EXIT_FAILURE);
    } else {
        exit(EXIT_FAILURE);
    }

    /* alloc memory to result matrices */
    mA = (int *) malloc(sizeof(int) * (rA * cA));
    mB = (int *) malloc(sizeof(int) * (rB * cB));
    mC = (int *) malloc(sizeof(int) * (rA * cB));

    /* number of multiplications realized on each calculation */
    multiplications = cA;

    /* init matrices */
    mA = init_matrix(rA, cA);
    mB = init_matrix(rB, cB);

    /* used to call function mpi_wtime */
    MPI_Init(&argc, &argv);
    initial_time = MPI_Wtime();

    for (_row = 0; _row < rA; _row++) {
        for (_column = 0; _column < cB; _column++) {
            value = 0;
            for (_index = 0; _index < multiplications; _index++) {
                value += mA[_row * multiplications + _index] * mB[_index * cB + _column];
            }
            mC[_row * cB + _column] = value;
        }
    }

    /* get the execution time */
    final_time = MPI_Wtime();
    execution_time = final_time - initial_time;

    MPI_Finalize();

    /* prints the processing result */
    printf("threads:%d\n", 1);
    printf("time:%lf\n", execution_time);
    printf("work:[%d,%d]x[%d,%d]\n", rA, cA, rB, cB);

    return 0;
}

/**
 * init a matrix (vector, in memory) with rand numbers of length: rows and columns
 * @param rows
 * @param columns
 * @return pointer to vector that represents the matrix initialized
 */
int * init_matrix(int rows, int columns) {
    int *matrix, counter;
    matrix = (int *) malloc(sizeof(int) * (rows * columns));

    for (counter = 0; counter < (rows * columns); counter++) {
        matrix[counter] = (int) rand() % 10;
    }
    return matrix;
}

/**
 * init a matrix (vector, in memory) with zeros
 * @param rows
 * @param columns
 * @return pointer to vector that represents the matrix initialized
 */
int * init_zero_matrix(int rows, int columns) {
    int *matrix, counter;
    matrix = (int *) malloc(sizeof(int) * (rows * columns));

    for (counter = 0; counter < (rows * columns); counter++) {
        matrix[counter] = 0;
    }
    return matrix;
}

/**
 * init a matrix (vector, in memory) with ones
 * @param rows
 * @param columns
 * @return pointer to vector that represents the matrix initialized
 */
int * init_one_matrix(int rows, int columns) {
    int *matrix, counter;
    matrix = (int *) malloc(sizeof(int) * (rows * columns));

    for (counter = 0; counter < (rows * columns); counter++) {
        matrix[counter] = 1;
    }
    return matrix;
}

/**
 * print the matrix
 * @param name
 * @param matrix
 * @param n_rows
 * @param n_columns
 */
void print_matrix(char *name, int *matrix, int n_rows, int n_columns) {
    int row, column;

    printf("\n%s: %dx%d\n", name, n_rows, n_columns);
    printf("\n|");
    for (column = 0; column < n_columns; column++) {
        printf("---|");
    }
    printf("\n");

    for (row = 0; row < n_rows; row++) {
        printf("|");
        for (column = 0; column < n_columns; column++) {
            printf(" %d |", matrix[row * n_columns + column]);
        }
        printf("\n|");
        for (column = 0; column < n_columns; column++) {
            printf("---|");
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * print the matrix
 * @param name
 * @param matrix
 * @param n_rows
 * @param n_columns
 */
void print_borderless_matrix(char *name, int *matrix, int n_rows, int n_columns) {
    int row, column;

    printf("\n%s: %dx%d\n\n", name, n_rows, n_columns);
    for (row = 0; row < n_rows; row++) {
        for (column = 0; column < n_columns; column++) {
            printf("%d\t", matrix[row * n_columns + column]);
        }
        printf("\n");
    }
    printf("\n");
}