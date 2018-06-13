#!/bin/bash
#SBATCH --nodes=1
#SBATCH --cpus-per-task=1
#SBATCH --time=0-0:30
#SBATCH --mem=15GB
#SBATCH --output=outputs/%j.out
#SBATCH --error=errors/%j.err
srun main $1 $2 $3 $4