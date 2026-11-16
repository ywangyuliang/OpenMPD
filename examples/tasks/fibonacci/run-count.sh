#!/bin/bash
set -e
cd "$(dirname "$0")/../../../OpenMpD"

make fparse

gcc -O2 -fopenmp ../examples/tasks/fibonacci/fibonacci-count-omp.c -o fibonacci_count_omp_out
OMP_NUM_THREADS=4 ./fibonacci_count_omp_out 50 30

./fparse ../examples/tasks/fibonacci/fibonacci-count-ompd.c log.txt error.txt fibonacci_count_ompd_output.c
mpicc -O2 fibonacci_count_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o fibonacci_count_ompd_out
mpirun -np 5 ./fibonacci_count_ompd_out 50 30
