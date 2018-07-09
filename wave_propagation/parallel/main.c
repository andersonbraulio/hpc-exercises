#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <omp.h>

/*
 * C  = velocity
 * dT = time
 * dX =
 *
 */
#define C  0.7
#define E  1
#define dT 0.04
#define PI 3.14159265358979323846

/* prototypes */
double * get_initial_conditions(int, double);
double * get_previous_conditions(double *, int, double);

int main(int argc, char **argv) {
    int counter, c_pos, c_time, M, N, n_threads;
    double *v_prev, *v_curr, *v_next, p_prev, p_curr, p_next, dX, start, exec_time;
    double fixed_part;

    /**
     * argv[1] = M (gridpoints)
     * argv[2] = N (timesteps)
     */
    if (argc == 4 && argv[1] != NULL && argv[2] != NULL && argv[3] != NULL) {
        M = atoi(argv[1]);
        N = atoi(argv[2]);
        n_threads = atoi(argv[3]);

        if (n_threads > 0) {
            omp_set_num_threads(n_threads);
        }
    } else {
        exit(0);
    }

    /* start dX */
    dX = (double) 1. / M;

    /* alloc memory to vectors */
    v_prev = (double *) malloc(sizeof(double) * (M + 1));
    v_curr = (double *) malloc(sizeof(double) * (M + 1));
    v_next = (double *) malloc(sizeof(double) * (M + 1));

    /* start initial conditions */
    v_curr = get_initial_conditions(M, dX);
    v_prev = get_previous_conditions(v_curr, M, dX);

    fixed_part = (C * C) * ((dT * dT) / (dX * dX));

    start = omp_get_wtime();

    /* calculate all points */
    for (c_time = 0; c_time < N; c_time++) {
        #pragma omp parallel for private(p_prev, p_curr, p_next)
        for (c_pos = 0; c_pos <= M; c_pos++) {
            p_prev = v_curr[c_pos - 1];
            p_curr = v_curr[c_pos];
            p_next = v_curr[c_pos + 1];

            if (c_pos == 0) {
                p_prev = v_curr[M - 1];
            }

            if (c_pos == M) {
                p_next = v_curr[1];
            }

            v_next[c_pos] = fixed_part * (p_next - (2. * p_curr) + p_prev) + (2. * p_curr) - v_prev[c_pos];
        }

        v_prev = v_curr;
        v_curr = v_next;

    }

    exec_time = omp_get_wtime() - start;

    /* print values */
    /*for (counter = 1; counter <= M; counter++) {
        printf("%.5lf\t", v_curr[counter]);
    }*/

    printf("threads:%d\n", n_threads);
    printf("time:%lf\n", exec_time);
    printf("work:%dx%d\n", M,N);

    return 0;
}

double * get_initial_conditions(int M, double dX) {
    int counter;
    double *vector;

    vector = (double *) malloc(sizeof(double) * (M + 1));
    for (counter = 0; counter <= M; counter++) {
        vector[counter] = sin(2 * PI * (counter * dX));
    }

    return vector;
}

double * get_previous_conditions(double *v_initial_conditions, int M, double dX) {
    int counter;
    double *vector;

    vector = (double *) malloc(sizeof(double) * (M + 1));
    for (counter = 0; counter <= M; counter++) {
        vector[counter] = (double) (2. * PI * C * cos(2. * PI * (counter * dX)) * dT + v_initial_conditions[counter]);
    }

    return vector;
}
