#!/bin/bash
mpicc main.c -o main

l_sizes='x1 x2 x3'
l_tasks='2 4 8 16 32'
n_executions=10
for tasks in $l_tasks
do
    for size in $l_sizes
    do
        for (( counter=1; counter<=$n_executions; counter++ ))
	do
            sbatch --exclusive --tasks-per-node=$tasks script.sh $size
        done
    done
done