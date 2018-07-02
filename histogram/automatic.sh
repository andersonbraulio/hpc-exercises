#!/bin/bash
mpicc -I/usr/include/libpng12 -L/usr/lib64 -lpng12 -fopenmp main.c -o main

l_sizes='x0 x1 x2 x3 x4 x5 x6'
l_tasks='1 2 4 8 16 32 64'
n_executions=10
for tasks in $l_tasks
do
    for size in $l_sizes
    do
        for (( counter=1; counter<=$n_executions; counter++ ))
	do
            sbatch --exclusive script.sh $size $tasks
        done
    done
done