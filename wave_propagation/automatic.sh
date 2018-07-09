#!/bin/bash
mpicc -fopenmp main.c -o main

l_sizes='10000 20000 40000 80000 160000 320000'
l_tasks='2 4 8 16 32 64'
n_executions=5
for tasks in $l_tasks
do
    for size in $l_sizes
    do
        for (( counter=1; counter<=$n_executions; counter++ ))
        do
            sbatch -p test --exclusive script.sh $size 10000 $tasks
        done
    done
done