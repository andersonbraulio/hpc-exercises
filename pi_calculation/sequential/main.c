#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* time parameters */
#define x0 5
#define x1 10
#define x2 40
#define x3 160

/* prototypes */
int rand_montecarlo(void);
long get_length_problem(char *);
void abort_execution(char *);

int main(int argc, char **argv) {
    unsigned long long in_circle = 0, points = 0;
    double result;
    time_t start, end;
    int execution_time;

    /* get how many seconds will be used to calculation */
    if (argc >= 2 && argv[1] != NULL) {
        execution_time = get_length_problem(argv[1]);
    } else {
        abort_execution("error: parameter dont received");
    }

    srand(time(NULL));
    start = time(NULL);

    do {
        if (rand_montecarlo()) {
            in_circle++;
        }
        points++;
        end = time(NULL);
    } while ((end - start) < execution_time);

    /* monte carlo calculation */
    result = 4 * ((long double) in_circle / points);

    printf("threads: %d\n", 1);
    printf("time: %d\n", (int) execution_time);
    printf("pi: %.12lf\n", result);

    return 0;
}

/***
 *
 * @return 1 if the two numbers generated are according with monte carlo algorithm or 0 if the opposite
 */
int rand_montecarlo(void) {
    float x = (float) rand() / RAND_MAX;
    float y = (float) rand() / RAND_MAX;

    if ((x * x) + (y * y) < 1)
        return 1;
    else
        return 0;
}

/***
 *
 * @param problem
 * @return how many seconds will be used to generate the points of calculation
 */
long get_length_problem(char * problem) {
    if (!strcmp(problem, "x1")) {
        return x1;
    } else if (!strcmp(problem, "x2")) {
        return x2;
    } else if (!strcmp(problem, "x3")) {
        return x3;
    } else {
        return x0;
    }
}

/**
 * shutdown the code and print the message
 * @param message
 */
void abort_execution(char *message) {
    printf("%s\n", message);
    exit(0);
}