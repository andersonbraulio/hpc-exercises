#!/bin/bash
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=32
#SBATCH --time=0-0:30
#SBATCH --mem=25GB
#SBATCH --output=outputs/%j.out
#SBATCH --error=errors/%j.err

if [ $2 -gt 32 ]
then
    srun -n2 main image.png $1 32
else
    srun -n1 main image.png $1 $2
fi