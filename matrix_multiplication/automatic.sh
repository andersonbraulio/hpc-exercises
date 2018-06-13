#!/bin/bash
mpicc main.c -o main
l_sizes='512 724 1024 1448 2048 2896'
l_tasks='2 4 8 16 32'
n_executions=3
for ((c = 1; c <= $n_executions; c++))
do
    for tasks in $l_tasks
    do
        for size in $l_sizes
        do
            sbatch --exclusive --tasks-per-node=$tasks script.sh $size 2896 2896 1
        done
    done
done