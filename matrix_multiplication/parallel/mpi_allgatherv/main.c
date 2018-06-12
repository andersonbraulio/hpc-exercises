#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>

/* prototypes */
int * init_matrix(int, int);
int * init_zero_matrix(int, int);
int * init_one_matrix(int, int);

int * split_amount_between_processes(int, int);
int * split_blocks_of_matrix_between_processes(int, int, int);

int get_number_of_elements_of_process(int *, int);
int get_responsible_process_id_per_global_index(int *, int, int);

int get_index_on_process_of_real_index(int *, int, int, int);
int * get_indexes_of_process(int *, int, int);

int * get_number_of_elements_on_each_process(int *, int);
int * get_displacements_on_each_process(int *, int);

int * get_number_of_elements_of_each_process_on_result_matrix(int, int, int);
int * get_displacements_of_each_process_on_result_matrix(int, int, int);

void print_matrix(char *, int *, int, int);
void print_borderless_matrix(char *, int *, int, int);

int main(int argc, char **argv) {
    int process_id, n_processes, root_id = 0;
    int counter, _row, _column;
    int value, multiplications, rA, cA, rB, cB;
    int n_rows_of_A, n_rows_of_B;
    int *mA, *mB, *mC, *mResult = NULL;
    int *rows_of_A_per_process, *rows_of_B_per_process, *column_content;
    int *n_elements_per_process, *displacements_per_process, *indexes_of_rows_of_A;
    int *n_elements_per_process_on_result, *displacements_per_process_on_result;
    double initial_time, final_time, execution_time, max_execution_time;
    MPI_Datatype mpi_dt_column;

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

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);
    MPI_Comm_size(MPI_COMM_WORLD, &n_processes);

    /* alloc memory to result matrices */
    if (process_id == root_id) { mResult = (int *) malloc(sizeof(int) * (rA * cB)); }

    rows_of_A_per_process = (int *) malloc(sizeof(int) * (n_processes * 3));
    rows_of_B_per_process = (int *) malloc(sizeof(int) * (n_processes * 3));

    rows_of_A_per_process = split_amount_between_processes(rA, n_processes);
    rows_of_B_per_process = split_amount_between_processes(rB, n_processes);

    /* number of multiplications realized on each calculation */
    multiplications = cA;

    /* init rows and columns that are under responsibility of each process */
    n_rows_of_A = get_number_of_elements_of_process(rows_of_A_per_process, process_id);
    mA = (int *) malloc(sizeof(int) * (n_rows_of_A * cA));
    mA = init_matrix(n_rows_of_A, cA);

    n_rows_of_B = get_number_of_elements_of_process(rows_of_B_per_process, process_id);
    mB = (int *) malloc(sizeof(int) * (n_rows_of_B * cB));
    mB = init_matrix(n_rows_of_B, cB);

    /* result matrix on each process */
    mC = (int *) malloc(sizeof(int) * (n_rows_of_A * cB));

    MPI_Barrier(MPI_COMM_WORLD);
    initial_time = MPI_Wtime();

    column_content = (int *) malloc(sizeof(int) * multiplications);

    /* create type vector of each process */
    MPI_Type_vector(n_rows_of_B, 1, cB, MPI_INT, &mpi_dt_column);
    MPI_Type_commit(&mpi_dt_column);

    n_elements_per_process = (int *) malloc(sizeof(int) * n_processes);
    displacements_per_process = (int *) malloc(sizeof(int) * n_processes);

    /* will be used to call allgatherv and communicate the cols of B matrix between processes */
    n_elements_per_process = get_number_of_elements_on_each_process(rows_of_B_per_process, n_processes);
    displacements_per_process = get_displacements_on_each_process(rows_of_B_per_process, n_processes);

    indexes_of_rows_of_A = (int *) malloc(sizeof(int) * n_rows_of_A);
    indexes_of_rows_of_A = get_indexes_of_process(rows_of_A_per_process, n_processes * 3, process_id);

    for (counter = 0; counter < cB; counter++) {
        MPI_Allgatherv(&mB[counter], 1, mpi_dt_column, column_content,
                       n_elements_per_process, displacements_per_process, MPI_INT, MPI_COMM_WORLD);

        for (_row = 0; _row < n_rows_of_A; _row++) {
            value = 0;
            for (_column = 0; _column < multiplications; _column++) {
                value += mA[_row * cA + _column] * column_content[_column];
            }
            mC[_row * cB + counter] = value;
        }
    }

    /* will be used to call gatherv and to send the rows of C matrix to root process */
    if (process_id == root_id) {
        n_elements_per_process_on_result = (int *) malloc(sizeof(int) * n_processes);
        displacements_per_process_on_result = (int *) malloc(sizeof(int) * n_processes);

        n_elements_per_process_on_result = get_number_of_elements_of_each_process_on_result_matrix(rA, cB, n_processes);
        displacements_per_process_on_result = get_displacements_of_each_process_on_result_matrix(rA, cB, n_processes);
    }

    MPI_Gatherv(mC, (n_rows_of_A * cB), MPI_INT, mResult, n_elements_per_process_on_result,
                displacements_per_process_on_result, MPI_INT, root_id, MPI_COMM_WORLD);

    final_time = MPI_Wtime();
    execution_time = final_time - initial_time;

    MPI_Reduce(&execution_time, &max_execution_time, 1, MPI_DOUBLE, MPI_MAX, root_id, MPI_COMM_WORLD);

    if (process_id == root_id) {
        printf("threads:%d\n", n_processes);
        printf("time:%lf\n", max_execution_time);
        printf("work:[%d,%d]x[%d,%d]\n", rA, cA, rB, cB);
    }

    /* TODO: free malloc */
    MPI_Finalize();

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
 * split the number n_elements between the processes
 * @param n_elements
 * @param n_processes
 * @return a vector of size (3 * n_processes), that contains: the process id, the displacement to the initial element
 * and the number of elements per each process.
 *
 * example: 3 elements and 2 process return a vector with this values: [0, 0, 2, 1, 2, 1]. this represents that the
 * first process (id: 0) will have 2 elements, starting at position 0, and the second process (id: 1) will have 1
 * element, starting at position 2
 */
int * split_amount_between_processes(int n_elements, int n_processes) {
    int elements_per_process, max_elements_per_process, extra_elements, counter, process_id, displacement = 0;
    int *splitted = (int *) malloc(sizeof(int) * (3 * n_processes));

    elements_per_process = n_elements / n_processes;
    max_elements_per_process = elements_per_process + 1;
    extra_elements = n_elements % n_processes;

    for (counter = 0, process_id = 0; counter < (n_processes * 3), process_id < n_processes; process_id++) {
        splitted[counter++] = process_id;
        splitted[counter++] = displacement;

        if (process_id < extra_elements) {
            splitted[counter] = max_elements_per_process;
        } else {
            splitted[counter] = elements_per_process;
        }
        displacement += splitted[counter++];
    }

    return splitted;
}

/**
 * split the number n_elements of a matrix between the processes
 * @param n_elements
 * @param n_processes
 * @return a vector of size (3 * n_processes), that contains: the process id, the displacement to the initial element
 * and the number of elements per each process
 */
int * split_blocks_of_matrix_between_processes(int n_rows, int n_columns, int n_processes) {
    int rows_per_process, max_rows_per_process, extra_rows, counter, process_id, displacement = 0;
    int *splitted = (int *) malloc(sizeof(int) * (3 * n_processes));

    rows_per_process = n_rows / n_processes;
    max_rows_per_process = rows_per_process + 1;
    extra_rows = n_rows % n_processes;

    for (counter = 0, process_id = 0; counter < (n_processes * 3), process_id < n_processes; process_id++) {
        splitted[counter++] = process_id;
        splitted[counter++] = displacement;

        if (process_id < extra_rows) {
            splitted[counter] = max_rows_per_process * n_columns;
        } else {
            splitted[counter] = rows_per_process * n_columns;
        }
        displacement += splitted[counter++];
    }

    return splitted;
}

/**
 * get the number of elements presented in vector returned by split_amount_between_processes
 * @param vector
 * @param process_id
 * @return number of the elements to a specific process. it can be used after split_amount_between_processes function
 */
int get_number_of_elements_of_process(int *vector, int process_id) {
    return vector[process_id * 3 + 2];
}

/**
 * get process id responsible per a global index
 * @param vector
 * @param vector_len
 * @param index
 * @return
 */
int get_responsible_process_id_per_global_index(int *vector, int vector_len, int index) {
    int counter, sum = 0;

    for (counter = 0; counter < vector_len; counter+=3) {
        sum += vector[counter + 2];
        /* identify the number of elements to this process */
        if (sum > index){ return vector[counter]; }
    }

    return -1;
}

/***
 * get the index on a process of a real index
 * @param vector
 * @param vector_len
 * @param process_id
 * @param index_on_process_queue
 * @return
 */
int get_index_on_process_of_real_index(int *vector, int vector_len, int process_id, int real_index) {
    int _blocks, _index, sum = 0;

    /* the vector has blocks of three elements (look: split_amount_between_processes) */
    for (_blocks = 0; _blocks < vector_len; _blocks+=3) {
        /* identify the number of elements to this process */
        if (vector[_blocks] == process_id && (sum + vector[_blocks + 2]) > real_index) {
            for (_index = 0; _index < vector[_blocks + 2]; _index++) {
                if (sum + _index == real_index) { return _index; }
            }
        }
        sum += vector[_blocks + 2];
    }

    return -1;
}

int * get_number_of_elements_on_each_process(int *vector, int n_processes) {
    int *n_elements, _index;

    n_elements = (int *) malloc(sizeof(int) * n_processes);

    /* the vector has blocks of three elements (look: split_amount_between_processes) */
    for (_index = 0; _index < n_processes; _index++) {
        n_elements[_index] = vector[_index * 3 + 2];
    }

    return n_elements;
}

int * get_displacements_on_each_process(int *vector, int n_processes) {
    int *n_elements, _index;

    n_elements = (int *) malloc(sizeof(int) * n_processes);

    /* the vector has blocks of three elements (look: split_amount_between_processes) */
    for (_index = 0; _index < n_processes; _index++) {
        n_elements[_index] = vector[_index * 3 + 1];
    }

    return n_elements;
}

int * get_number_of_elements_of_each_process_on_result_matrix(int n_rows, int n_columns, int n_processes) {
    int *n_rows_of_each_process, *n_elements, _index;

    n_rows_of_each_process = (int *) malloc(sizeof(int) * (n_processes * 3));
    n_rows_of_each_process = split_amount_between_processes(n_rows, n_processes);

    n_elements = (int *) malloc(sizeof(int) * n_processes);

    /* the vector has blocks of three elements (look: split_amount_between_processes) */
    for (_index = 0; _index < n_processes; _index++) {
        n_elements[_index] = n_rows_of_each_process[_index * 3 + 2] * n_columns;
    }

    return n_elements;
}

int * get_displacements_of_each_process_on_result_matrix(int n_rows, int n_columns, int n_processes) {
    int *n_rows_of_each_process, *n_elements, _index;

    n_rows_of_each_process = (int *) malloc(sizeof(int) * (n_processes * 3));
    n_rows_of_each_process = split_amount_between_processes(n_rows, n_processes);

    n_elements = (int *) malloc(sizeof(int) * n_processes);

    /* the vector has blocks of three elements (look: split_amount_between_processes) */
    for (_index = 0; _index < n_processes; _index++) {
        n_elements[_index] = n_rows_of_each_process[_index * 3 + 1] * n_columns;
    }

    return n_elements;
}

/***
 * get global indexes to a specific process
 * @param vector
 * @param vector_len
 * @param process_id
 * @return
 */
int * get_indexes_of_process(int *vector, int vector_len, int process_id) {
    int *indexes, _blocks, _index, sum = 0;

    /* the vector has blocks of three elements (look: split_amount_between_processes) */
    for (_blocks = 0; _blocks < vector_len; _blocks+=3) {
        /* identify the number of elements to this process */
        if (vector[_blocks] == process_id) {
            indexes = (int *) malloc(sizeof(int) * vector[_blocks + 2]);
            for (_index = 0; _index < vector[_blocks + 2]; _index++)
                indexes[_index] = sum + _index;
        }
        sum += vector[_blocks + 2];
    }

    return indexes;
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