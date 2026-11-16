#!/bin/bash
set -eo pipefail
cd "$(dirname "$0")/.."

make fparse

gcc -O2 -fopenmp ../examples/tasks/fibonacci/fibonacci-count-omp.c -o fibonacci_count_omp_out

./fparse ../examples/tasks/fibonacci/fibonacci-count-ompd.c log.txt error.txt fibonacci_count_ompd_output.c
mpicc -O2 fibonacci_count_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o fibonacci_count_ompd_out

{
    echo "OpenMP: 4 hilos, n=50, umbral=30"
    OMP_NUM_THREADS=4 ./fibonacci_count_omp_out 50 30
    echo
    echo "OpenMPD: 5 procesos MPI, n=50, umbral=30"
    mpirun -np 5 ./fibonacci_count_ompd_out 50 30
} 2>&1 | tee tests/resultados.txt
