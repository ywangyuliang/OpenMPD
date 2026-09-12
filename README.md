# OpenMPD

- **Authors:** Yuliang Wang, Caijie Wu
- **Supervisor:** Antonio García Dopico
- **Department:** Departamento de Arquitectura y Tecnología de Sistemas Informáticos (DATSI), Universidad Politécnica de Madrid (UPM)

**English** · [Español](#openmpd-español)

OpenMPD is a source-to-source translator for C. It translates OpenMPD directives into MPI calls, producing a C program that can run across several processes. Its directive model is based on OpenMP and adapted to distributed-memory systems.

The programmer describes distributed-memory parallelism with directives instead of writing the MPI communication layer directly. OpenMPD supports data distribution, collective communication, reductions, halo exchange and asynchronous tasks managed by its runtime.

The translator is built as a single executable, `fparse`.

## Repository layout

```
OMPD/
├── OpenMpD/
│   ├── include/   translator headers
│   ├── src/       translator and runtime sources
│   ├── syntax/    lexer and parser sources
│   ├── scripts/   helper scripts
│   └── tests/     regression and performance tests
├── examples/      example programs (sequential, OpenMP, OpenMPD, MPI)
└── README.md      this file
```

## Build and run

OpenMPD is intended for Linux systems. It requires GNU Make, GCC/G++, Bison, Flex, an MPI implementation that provides `mpicc` and `mpirun`, and Python 3 for the test scripts. The performance tests also require Linux `perf`.

Clone the repository and enter the translator directory before running the commands in this README:

```sh
git clone https://github.com/Yul1ang/OMPD.git
cd OMPD/OpenMpD
```

### 1. Build the translator

```sh
make fparse
```

This command generates the parsers and scanners and links the `fparse` executable. Use a clean build after changing the parser or scanner sources:

```sh
make clean && make fparse
```

### 2. Translate an OpenMPD program

The `examples/` directory contains OpenMPD programs grouped by area, including `pi`, `halo` and `tasks`.

`fparse` accepts an input program, a log path, an error-log path and an optional path for the generated C/MPI program.

```sh
./fparse input.c log.txt error.txt name_output.c
```

The examples in this README use the `*_output.c` suffix for generated C files. Git ignores that suffix, as well as `log.txt` and `error.txt`:

```sh
./fparse ../examples/halo/image_filters/image_halo_OMPD.c log.txt error.txt image_halo_output.c
```

`image_halo_output.c` is the generated program; `log.txt` and `error.txt` contain the translation diagnostics. If the final path is omitted, the translator analyses the input but does not generate a C file.

### 3. Compile and run the generated program

Compile the generated program with `mpicc` and execute it with `mpirun`. Each example in `tests/regression/cases.tsv` records its arguments, libraries and process count:

```sh
mpicc -O2 image_halo_output.c -fopenmp -lm -o image_halo_out
mpirun -np 4 ./image_halo_out gaussian 3 ../examples/halo/image_filters/lenna.pgm result.pgm
```

Programs that use `task_async` or `taskwait` must be linked with the OpenMPD tasking runtime. For example:

```sh
./fparse ../examples/tasks/fibonacci/fibonacci-ompd.c log.txt error.txt fibonacci_ompd_output.c
mpicc -O2 fibonacci_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o fibonacci_ompd_out
mpirun -np 4 ./fibonacci_ompd_out 20 10
```

### 4. Complete example commands

The following commands are complete and can be run from `OpenMpD/`. Build `fparse` first with `make clean && make fparse`. For generated programs that support both forms, the commands show a build without OpenMP followed by a build with `-fopenmp`.

#### PI

```sh
gcc -O2 ../examples/pi/pi_seq.c -o pi_seq_out -lm
./pi_seq_out 1

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/pi/pi_omp.c -o pi_omp_out -lm
./pi_omp_out 1

./fparse ../examples/pi/pi_test1.c log.txt error.txt pi_test1_output.c
mpicc -O2 pi_test1_output.c -o pi_test1_out -lm
mpirun -np 4 ./pi_test1_out 1
mpicc -O2 -fopenmp pi_test1_output.c -o pi_test1_out -lm
mpirun -np 4 ./pi_test1_out 1

./fparse ../examples/pi/pi_ompd.c log.txt error.txt pi_ompd_output.c
mpicc -O2 pi_ompd_output.c -o pi_ompd_out -lm
mpirun -np 4 ./pi_ompd_out 1
mpicc -O2 -fopenmp pi_ompd_output.c -o pi_ompd_out -lm
mpirun -np 4 ./pi_ompd_out 1
```

#### Julia

```sh
gcc -O2 ../examples/julia/JuliaSEQ.c -o julia_seq_out -lm
./julia_seq_out 1000 -0.85 0.0 2.0 200

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/julia/JuliaOMP.c -o julia_omp_out -lm
./julia_omp_out 1000 -0.85 0.0 2.0 200

./fparse ../examples/julia/prueba_julia.c log.txt error.txt prueba_julia_output.c
mpicc -O2 prueba_julia_output.c -o prueba_julia_out -lm
mpirun -np 4 ./prueba_julia_out 1000 -0.85 0.0 2.0 200
mpicc -O2 -fopenmp prueba_julia_output.c -o prueba_julia_out -lm
mpirun -np 4 ./prueba_julia_out 1000 -0.85 0.0 2.0 200

./fparse ../examples/julia/JuliaOMPD.c log.txt error.txt julia_ompd_output.c
mpicc -O2 julia_ompd_output.c -o julia_ompd_out -lm
mpirun -np 4 ./julia_ompd_out 1000 -0.85 0.0 2.0 200
mpicc -O2 -fopenmp julia_ompd_output.c -o julia_ompd_out -lm
mpirun -np 4 ./julia_ompd_out 1000 -0.85 0.0 2.0 200
```

#### MMAT

```sh
gcc -O2 ../examples/mat_ompd/mat_seq.c -o mmat_seq_out
./mmat_seq_out 1536 1536 1536 1536

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/mat_ompd/mat_omp.c -o mmat_omp_out
./mmat_omp_out 1536 1536 1536 1536

./fparse ../examples/mat_ompd/prueba_mmat.c log.txt error.txt prueba_mmat_output.c
mpicc -O2 prueba_mmat_output.c -o prueba_mmat_out
mpirun -np 4 ./prueba_mmat_out 1536 1536 1536 1536
mpicc -O2 -fopenmp prueba_mmat_output.c -o prueba_mmat_out
mpirun -np 4 ./prueba_mmat_out 1536 1536 1536 1536

./fparse ../examples/mat_ompd/mat_ompd.c log.txt error.txt mat_ompd_output.c
mpicc -O2 mat_ompd_output.c -o mat_ompd_out
mpirun -np 4 ./mat_ompd_out 1536 1536 1536 1536
mpicc -O2 -fopenmp mat_ompd_output.c -o mat_ompd_out
mpirun -np 4 ./mat_ompd_out 1536 1536 1536 1536
```

#### Mandelbrot

```sh
gcc -O2 ../examples/mandelbrot/Mandelbrot-SEQ.c -o mandelbrot_seq_out -lm
./mandelbrot_seq_out

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/mandelbrot/Mandelbrot-OMP.c -o mandelbrot_omp_out -lm
./mandelbrot_omp_out

./fparse ../examples/mandelbrot/prueba_mandel.c log.txt error.txt prueba_mandel_output.c
mpicc -O2 prueba_mandel_output.c -o prueba_mandel_out -lm
mpirun -np 4 ./prueba_mandel_out
mpicc -O2 -fopenmp prueba_mandel_output.c -o prueba_mandel_out -lm
mpirun -np 4 ./prueba_mandel_out

./fparse ../examples/mandelbrot/Mandelbrot-OMPD.c log.txt error.txt mandelbrot_ompd_output.c
mpicc -O2 mandelbrot_ompd_output.c -o mandelbrot_ompd_out -lm
mpirun -np 4 ./mandelbrot_ompd_out
mpicc -O2 -fopenmp mandelbrot_ompd_output.c -o mandelbrot_ompd_out -lm
mpirun -np 4 ./mandelbrot_ompd_out
```

#### MD

```sh
gcc -O2 ../examples/MD/mdSEQ.c -o md_seq_out -lm
./md_seq_out 3 800 700 0.01

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/MD/mdOMP.c -o md_omp_out -lm
./md_omp_out 3 800 700 0.01

./fparse ../examples/MD/mdOMPD.c log.txt error.txt md_ompd_output.c
mpicc -O2 md_ompd_output.c -o md_ompd_out -lm
mpirun -np 4 ./md_ompd_out 3 800 700 0.01
mpicc -O2 -fopenmp md_ompd_output.c -o md_ompd_out -lm
mpirun -np 4 ./md_ompd_out 3 800 700 0.01
```

#### Heated plate

```sh
gcc -O2 ../examples/halo/heated_plate/heated_plate_SEQ.c -o heated_plate_seq_out -lm
./heated_plate_seq_out 0.001 heated_plate_seq_result.txt

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/halo/heated_plate/heated_plate_OMP.c -o heated_plate_omp_out -lm
./heated_plate_omp_out 0.001 heated_plate_omp_result.txt

./fparse ../examples/halo/heated_plate/heated_plate_OMPD.c log.txt error.txt heated_plate_ompd_output.c
mpicc -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
mpicc -fopenmp -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
```

#### Game of life

```sh
gcc -O2 ../examples/halo/game_of_life/game_of_life_SEQ.c -o game_of_life_seq_out -lm
./game_of_life_seq_out 1000 1000 8000 0

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/halo/game_of_life/game_of_life_OMP.c -o game_of_life_omp_out -lm
./game_of_life_omp_out 1000 1000 8000 0

./fparse ../examples/halo/game_of_life/game_of_life_OMPD.c log.txt error.txt game_of_life_ompd_output.c
mpicc -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out -lm
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
mpicc -fopenmp -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out -lm
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
```

#### Image halo

```sh
gcc -O2 ../examples/halo/image_filters/image_halo_SEQ.c -o image_halo_seq_out -lm
./image_halo_seq_out g 5 ../examples/halo/image_filters/lenna.pgm output_SEQ_g5x5.pgm 1000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/halo/image_filters/image_halo_OMP.c -o image_halo_omp_out -lm
./image_halo_omp_out g 5 ../examples/halo/image_filters/lenna.pgm output_OMP_g5x5.pgm 1000

./fparse ../examples/halo/image_filters/image_halo_OMPD.c log.txt error.txt image_halo_ompd_output.c
mpicc -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/halo/image_filters/lenna.pgm output_OMPD_g5x5.pgm 1000
mpicc -fopenmp -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/halo/image_filters/lenna.pgm output_OMPD_g5x5.pgm 1000
```

#### Tasking: Fibonacci

```sh
gcc -O2 ../examples/tasks/fibonacci/fibonacci-seq.c -o fibonacci_seq_out
./fibonacci_seq_out 50

export OMP_NUM_THREADS=8
gcc -O2 -fopenmp ../examples/tasks/fibonacci/fibonacci-omp.c -o fibonacci_omp_out
./fibonacci_omp_out 50 30

./fparse ../examples/tasks/fibonacci/fibonacci-ompd.c log.txt error.txt fibonacci_ompd_output.c
mpicc -O2 fibonacci_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o fibonacci_ompd_out
mpirun -np 8 ./fibonacci_ompd_out 50 30
mpirun --use-hwthread-cpus -np 16 ./fibonacci_ompd_out 50 30
```

#### Tasking: PI

```sh
gcc -O2 ../examples/tasks/pi_task/pi_task_seq.c -o pi_task_seq_out -lm
./pi_task_seq_out 1000000000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/pi_task/pi_task_omp.c -o pi_task_omp_out -lm
./pi_task_omp_out 1000000000

./fparse ../examples/tasks/pi_task/pi_task_ompd.c log.txt error.txt pi_task_ompd_output.c
mpicc -O2 pi_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -lm -o pi_task_ompd_out
mpirun -np 4 ./pi_task_ompd_out 1000000000
```

#### Tasking: Binomial

```sh
gcc -O2 ../examples/tasks/binomial/binomial_seq.c -o binomial_seq_out
./binomial_seq_out 35 17

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/binomial/binomial_omp.c -o binomial_omp_out
./binomial_omp_out 35 17 20

./fparse ../examples/tasks/binomial/binomial_task_ompd.c log.txt error.txt binomial_task_ompd_output.c
mpicc -O2 binomial_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o binomial_task_ompd_out
mpirun -np 4 ./binomial_task_ompd_out 35 17 20
```

#### Tasking: Pipeline

```sh
gcc -O2 ../examples/tasks/pipeline/pipeline_seq.c -o pipeline_seq_out
./pipeline_seq_out

export OMP_NUM_THREADS=5
gcc -O2 -fopenmp ../examples/tasks/pipeline/pipeline_omp.c -o pipeline_omp_out
./pipeline_omp_out

./fparse ../examples/tasks/pipeline/pipeline_ompd.c log.txt error.txt pipeline_ompd_output.c
mpicc -O2 pipeline_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o pipeline_ompd_out
mpirun -np 5 ./pipeline_ompd_out
```

#### Tasking: Inventory (`inout`)

```sh
gcc -O2 ../examples/tasks/inventory/inventory_seq.c -o inventory_seq_out
./inventory_seq_out

./fparse ../examples/tasks/inventory/inventory_ompd.c log.txt error.txt inventory_ompd_output.c
mpicc -O2 inventory_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o inventory_ompd_out
mpirun -np 4 ./inventory_ompd_out
```

#### Tasking: Simpson

```sh
gcc -O2 ../examples/tasks/simpson/simpson_seq.c -lm -o simpson_seq_out
./simpson_seq_out 1e-8 20

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/simpson/simpson_omp.c -lm -o simpson_omp_out
./simpson_omp_out 1e-8 20 10

./fparse ../examples/tasks/simpson/simpson_task_ompd.c log.txt error.txt simpson_task_ompd_output.c
mpicc -O2 simpson_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -lm -o simpson_task_ompd_out
mpirun -np 4 ./simpson_task_ompd_out 1e-8 20 10
```

## Architecture

The translator works in two passes. The first reads a preprocessed copy of the source and records declarations and types. The second reads the original source, copies its C code to the output and applies the OpenMPD transformations. There is no single intermediate representation for the complete C file.

```text
input.c
  -> first pass: mpicc -E -P -include mpi.h
       preprolexer.ll + preproparser.yy
       symbol_table.h
  -> second pass over input.c
       C99-scanner.lex + C99-parser.yacc
       omplexer.ll + ompparser.yy
       writer.cc + transform modules
  -> output.c
```

`main.cc` controls both passes and manages the input, diagnostic and output files. It starts `mpicc -E -P -include mpi.h` for the first pass. `preprolexer.ll` and `preproparser.yy` read its output and store declarations and type information in `symbol_table.h`.

For the second pass, `main.cc` returns to the original source and calls `yyparse()`. `C99-parser.yacc` defines the C grammar and `C99-scanner.lex` provides its tokens. The scanner reads preprocessor lines separately and sends OpenMPD pragmas to `parse_openmp_pragma()`. `omplexer.ll` tokenizes each directive and `ompparser.yy` parses its clauses.

The scanner sends the remaining C tokens to `writer.cc`. The writer constructs one line at a time and, according to the translator state, copies, buffers, rewrites or captures that line.

`pragma_args.h/.c` stores the arguments of the current directive. `translator_state.h/.c` and `cluster_stack.h/.c` store longer-lived state for constructs such as `cluster`, `distribute`, `master`, `halo` and `num_teams`. The transform modules use this state to generate MPI code.

`mpi_lifecycle` generates MPI initialization, finalization and process guards. `distribute_transform` rewrites loop bounds and generates work distribution. The scatter/gather, reduction, memory and datatype modules generate their corresponding MPI operations. `halo_transform` stores the bounds of the last distributed loop and uses them to generate the `MPI_Sendrecv` calls for a later `update halo` directive.

Tasking also uses the second pass, but task bodies are captured instead of copied directly. The tasking modules store each `task_async` body and generate a function for it. At the original position of the directive, they generate the code that creates the task, registers its dependencies and submits it to the runtime.

The scheduler normally selects the task that became ready most recently (LIFO). At `taskwait`, its worker only selects ready tasks created by the waiting task, directly or through other tasks. If several are ready, it selects the most recent one.

After parsing, `main.cc` flushes the writer and applies the deferred output slots. These slots contain headers, global declarations and task definitions that must appear earlier in the generated file.

## Supported directives

The translator supports three groups of constructs:

- **Data and work distribution.** A `cluster` region can allocate and broadcast data with `alloc` and `broad`, move arrays with `scatter`, `gather` and `allgather`, apply `reduction` and `allreduction`, and distribute loop iterations. `declare cluster` generates MPI datatype declarations.
- **Halo exchange.** A `halo(...)` clause declared on a `cluster` marks the array that needs ghost-border communication. A later `cluster distribute update halo(...)` uses the bounds of the last distributed loop and emits neighbour exchanges for the rows above and below the local block.
- **Tasking.** The translator supports `task_async` and `taskwait`. It captures each asynchronous task body and generates its function and input structure. `ompd_runtime.c` manages dependencies and selects ready tasks in LIFO order. At `taskwait`, it only selects work created by the waiting task, directly or through other tasks. `depend(inout: ...)` preserves the initial value and serializes read-modify-write tasks that use the same dependency.

## File map

### Entry point and flow

| File | Responsibility |
|---|---|
| `main.cc` | Drives the two passes, opens files, runs the preprocessor, applies the deferred slots at the end. |
| `symbol_table.h` | Scoped symbol table: types, variables and declarations collected in the preprocessor pass. |
| `preprolexer.ll`, `preproparser.yy` | Scanner and parser of the preprocessor pass. |
| `C99-scanner.lex`, `C99-parser.yacc` | Scanner and parser of the original C in the main pass. |
| `omplexer.ll`, `ompparser.yy` | Scanner and parser of the OpenMPD pragma text. |
| `writer.h/.cc` | Source reconstruction and output emission: token processing, line flushing, buffering, loop rewriting, task-body capture and directive-aware region closing. |
| `output_slots.h/.cc` | Deferred output insertion through named slots. |

### Translator state

| File | Responsibility |
|---|---|
| `translator_state.h/.c` | Global state grouped into structs (tasking, distribute, master, halo, num_teams) and their accessors. It is the Flex/Bison compatibility boundary. |
| `cluster_stack.h/.c` | Stack of active `cluster` regions and their close state. |
| `pragma_args.h/.c` | Arguments of the pragma being parsed and its clauses. |

### Transform modules

| File | Responsibility |
|---|---|
| `distribute_transform.h/.cc` | Loop distribution across processes: activation, extended modes, loop-header rewrite, MPI worksharing, `dist_schedule`, OpenMP clause forwarding. |
| `halo_transform.h/.c` | Declares halo variables, stores distributed-loop bounds and generates neighbour `MPI_Sendrecv` exchanges for `update halo`. |
| `scatter_gather_transform.h/.cc` | Scatter, gather and allgather generation. |
| `reduction_transform.h/.cc` | Reduce and allreduce generation. |
| `memory_transform.h/.cc` | Allocation and broadcast. |
| `mpi_type_transform.h/.cc` | MPI datatype declarations. |
| `mpi_lifecycle.h/.cc` | MPI init/finalize, runtime headers and global declarations, master/sequential guards, `num_teams` check. |
| `codegen_utils.h/.cc` | Shared generation utilities: type conversion, string formatting, indices. |

### Tasking (asynchronous tasks)

| File | Responsibility |
|---|---|
| `tasking_region.h/.c` | Model of a tasking region: ordered blocks inside a `cluster`. |
| `task_async_block.h/.c` | One asynchronous task block: dependencies, inputs and body text. |
| `task_body_transform.h/.c` | Intermediate representation of the task body and its generated code. |
| `tasking_emit.h/.cc` | Routes a finished region's generated output: the global definitions to their deferred slot and the body to the output. |
| `ompd_runtime.h/.c` | Tasking runtime compiled with the generated program: task creation, dependencies, LIFO selection, `taskwait` and synchronization. |
| `hash_map.h/.c`, `task_utils.h/.c` | Auxiliary tasking structures and utilities. |

### Generated files (do not edit by hand)

`ompparser.cc`, `ompparser.hh`, `omplexer.cc`, `preproparser.cc`, `preproparser.hh`, `preprolexer.cc`, `y.tab.cc`, `y.tab.hh`, `lex.yy.cc`.

They are produced by `bison`/`flex` from the `.yy`, `.yacc`, `.ll` and `.lex` files. To change the behaviour of a parser or scanner, edit its source, not the generated file; `make` regenerates them.

## Regression test

The regression test script is in `OpenMpD/tests/regression/`. For each case, it compiles and executes a sequential program and its OpenMPD equivalent, then compares their numeric or file output.

Run these commands from `OpenMpD/`. The `regression-test` target builds `fparse` when necessary. `tests/regression/cases.tsv` defines the source files, arguments, libraries, process counts and comparison method for every case.

The script writes temporary files and logs to `/tmp/ompd-regression` by default. Set `WORK_ROOT` to use another directory.

Normal test run:

```sh
make clean && make regression-test
```

Full test run, including the slow cases:

```sh
make fparse && RUN_SLOW=1 python3 tests/regression/run_regression.py
```

Run one family:

```sh
make fparse && FILTER_FAMILY=halo python3 tests/regression/run_regression.py
```

Expected results on a healthy tree:

```text
normal: summary: pass=18 skip=9 fail=0
full:   summary: pass=27 skip=0 fail=0
```

The script also accepts `CASES_FILE` and `CASE_TIMEOUT` as environment variables. To add a case, add one row to `tests/regression/cases.tsv`. The header documents its columns. `run_regression.py` defines the available path placeholders and comparison profiles.

## Performance tests

The performance tests use the same process and thread counts for every benchmark:

- `run_performance.py` defines 2, 4, 8, 12 and 24 as the single process/thread scale.
- `tests/performance/cases.tsv` lists the available variants for each benchmark. Individual cases cannot define a different scale.
- Fixed-topology examples such as `pipeline` (5 teams) and `inventory_inout` (4 teams) remain in the regression tests and are not part of the performance tests.

Each benchmark therefore has eleven core measurements: one sequential run, five OpenMP runs with 2, 4, 8, 12 and 24 threads, and five OpenMPD runs with the same number of MPI processes. Use `--include-ompd-openmp` to include the available OpenMPD+OpenMP configurations.

Preview the complete plan without compiling or running anything:

```sh
python3 tests/performance/run_performance.py --plan
```

List all command-line options:

```sh
python3 tests/performance/run_performance.py --help
```

Run every case in `tests/performance/cases.tsv`. By default, the script measures the sequential, OpenMP and OpenMPD variants and creates a timestamped CSV in `tests/performance/results/`, a directory of measured logs with the `.logs` suffix, and a separate directory with the `.warmup.logs` suffix:

```sh
python3 tests/performance/run_performance.py
```

The script builds `fparse`; no output path is required.

Run only the `base` cases:

```sh
python3 tests/performance/run_performance.py --family base
```

The `ompd` variant is the MPI program generated by OpenMPD. A separate `mpi` variant is a handwritten MPI implementation and is excluded by default. Add it with `--include-mpi`. Use `--include-ompd-openmp` for the available hybrid configurations. For a benchmark without a handwritten MPI implementation, set `mpi_source` to `-` and omit `mpi` and `mpi_omp` from `variants`.

Before measuring a configuration, the script runs it once without `perf` as a preliminary check. This detects an immediate execution failure and is not included in the measured average. Its standard output and error are saved separately in the `.warmup.logs` directory; if the warm-up fails, its diagnostics are also preserved in the corresponding `.error.log` file in the measured-log directory.

The script then executes the same configuration five times with `perf stat`. Each CSV row contains two means. `mean_seconds` includes the launcher and MPI lifecycle. `calculation_mean_seconds` uses the `OMPD_CALC_TIME_SECONDS=<seconds>` value printed by the example around its calculation. The script requires one such value from each measured execution.

## Development notes

To study the translator, build `fparse`, translate a small example and compare the generated MPI program with its OpenMPD source. The main path passes through `main.cc`, `C99-scanner.lex`, `C99-parser.yacc`, `omplexer.ll`, `ompparser.yy` and `writer.cc`. Each directive is then handled by its corresponding transform module.

`tests/regression/cases.tsv` provides a practical list of supported examples, required arguments, runtime dependencies and comparison methods.

Future work:

- Add more examples and regression cases with different input sizes, process counts and combinations of existing pragmas.
- Complete more of the OpenMP tasking model: add missing directives and clauses, improve dependency handling and support more complex task bodies.
- Define which OpenMPD constructs can be combined with tasking. The current parser keeps tasking clusters on a separate path and rejects some non-task pragmas at the same cluster level.
- Expand halo support beyond row-wise partitions, including two-dimensional distributions, columns and block-based decompositions.
- Improve translator memory management. Valgrind still reports memory left allocated at process exit, mainly around `symbol_info` objects in the symbol table; the code needs a clearer rule for which part creates those objects and which part releases them.
- Consider an intermediate representation instead of transforming the source line by line during parsing. This would separate analysis from code generation and could simplify task-body translation, semantic checks and MPI generation.

---

# OpenMPD (Español)

- **Autores:** Yuliang Wang, Caijie Wu
- **Tutor:** Antonio García Dopico
- **Departamento:** Departamento de Arquitectura y Tecnología de Sistemas Informáticos (DATSI), Universidad Politécnica de Madrid (UPM)

[English](#openmpd) · **Español**

OpenMPD es un traductor *source-to-source* para C. Traduce directivas OpenMPD a llamadas MPI y genera un programa en C que puede ejecutarse en varios procesos. Su modelo de directivas se basa en OpenMP y está adaptado a sistemas de memoria distribuida.

El programador describe el paralelismo de memoria distribuida mediante directivas, sin escribir directamente la capa de comunicación MPI. OpenMPD soporta distribución de datos, comunicación colectiva, reducciones, intercambio de halos y tareas asíncronas gestionadas por su runtime.

El traductor se construye como un único ejecutable, `fparse`.

## Estructura del repositorio

```
OMPD/
├── OpenMpD/
│   ├── include/   cabeceras del traductor
│   ├── src/       fuentes del traductor y del runtime
│   ├── syntax/    fuentes de lexer y parser
│   ├── scripts/   scripts auxiliares
│   └── tests/     pruebas de regresión y rendimiento
├── examples/      programas de ejemplo (secuencial, OpenMP, OpenMPD, MPI)
└── README.md      este fichero
```

## Construir y ejecutar

OpenMPD está pensado para sistemas Linux. Requiere GNU Make, GCC/G++, Bison, Flex, una implementación de MPI que proporcione `mpicc` y `mpirun`, y Python 3 para los scripts de pruebas. Las pruebas de rendimiento también requieren `perf` para Linux.

Clona el repositorio y entra en el directorio del traductor antes de ejecutar los comandos de este README:

```sh
git clone https://github.com/Yul1ang/OMPD.git
cd OMPD/OpenMpD
```

### 1. Construir el traductor

```sh
make fparse
```

Este comando genera los parsers y scanners y enlaza el ejecutable `fparse`. Usa una compilación limpia después de cambiar las fuentes de los parsers o scanners:

```sh
make clean && make fparse
```

### 2. Traducir un programa OpenMPD

El directorio `examples/` contiene programas OpenMPD agrupados por áreas, entre ellas `pi`, `halo` y `tasks`.

`fparse` recibe un programa de entrada, una ruta para el log, una ruta para el log de errores y una ruta opcional para el programa C/MPI generado.

```sh
./fparse entrada.c log.txt error.txt nombre_output.c
```

Los ejemplos de este README usan el sufijo `*_output.c` para los ficheros C generados. Git ignora ese sufijo, además de `log.txt` y `error.txt`:

```sh
./fparse ../examples/halo/image_filters/image_halo_OMPD.c log.txt error.txt image_halo_output.c
```

`image_halo_output.c` es el programa generado; `log.txt` y `error.txt` contienen los diagnósticos de la traducción. Si se omite la última ruta, el traductor analiza la entrada, pero no genera un fichero C.

### 3. Compilar y ejecutar el programa generado

El programa generado se compila con `mpicc` y se ejecuta con `mpirun`. Cada ejemplo de `tests/regression/cases.tsv` indica sus argumentos, librerías y número de procesos:

```sh
mpicc -O2 image_halo_output.c -fopenmp -lm -o image_halo_out
mpirun -np 4 ./image_halo_out gaussian 3 ../examples/halo/image_filters/lenna.pgm result.pgm
```

Los programas que usan `task_async` o `taskwait` deben enlazarse con el runtime de tasking de OpenMPD. Por ejemplo:

```sh
./fparse ../examples/tasks/fibonacci/fibonacci-ompd.c log.txt error.txt fibonacci_ompd_output.c
mpicc -O2 fibonacci_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o fibonacci_ompd_out
mpirun -np 4 ./fibonacci_ompd_out 20 10
```

### 4. Comandos completos de ejemplos

Los comandos siguientes están completos y se ejecutan desde `OpenMpD/`. Construye antes `fparse` con `make clean && make fparse`. Cuando un programa generado admite ambas formas, los comandos muestran primero la compilación sin OpenMP y después la compilación con `-fopenmp`.

#### PI

```sh
gcc -O2 ../examples/pi/pi_seq.c -o pi_seq_out -lm
./pi_seq_out 1

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/pi/pi_omp.c -o pi_omp_out -lm
./pi_omp_out 1

./fparse ../examples/pi/pi_test1.c log.txt error.txt pi_test1_output.c
mpicc -O2 pi_test1_output.c -o pi_test1_out -lm
mpirun -np 4 ./pi_test1_out 1
mpicc -O2 -fopenmp pi_test1_output.c -o pi_test1_out -lm
mpirun -np 4 ./pi_test1_out 1

./fparse ../examples/pi/pi_ompd.c log.txt error.txt pi_ompd_output.c
mpicc -O2 pi_ompd_output.c -o pi_ompd_out -lm
mpirun -np 4 ./pi_ompd_out 1
mpicc -O2 -fopenmp pi_ompd_output.c -o pi_ompd_out -lm
mpirun -np 4 ./pi_ompd_out 1
```

#### Julia

```sh
gcc -O2 ../examples/julia/JuliaSEQ.c -o julia_seq_out -lm
./julia_seq_out 1000 -0.85 0.0 2.0 200

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/julia/JuliaOMP.c -o julia_omp_out -lm
./julia_omp_out 1000 -0.85 0.0 2.0 200

./fparse ../examples/julia/prueba_julia.c log.txt error.txt prueba_julia_output.c
mpicc -O2 prueba_julia_output.c -o prueba_julia_out -lm
mpirun -np 4 ./prueba_julia_out 1000 -0.85 0.0 2.0 200
mpicc -O2 -fopenmp prueba_julia_output.c -o prueba_julia_out -lm
mpirun -np 4 ./prueba_julia_out 1000 -0.85 0.0 2.0 200

./fparse ../examples/julia/JuliaOMPD.c log.txt error.txt julia_ompd_output.c
mpicc -O2 julia_ompd_output.c -o julia_ompd_out -lm
mpirun -np 4 ./julia_ompd_out 1000 -0.85 0.0 2.0 200
mpicc -O2 -fopenmp julia_ompd_output.c -o julia_ompd_out -lm
mpirun -np 4 ./julia_ompd_out 1000 -0.85 0.0 2.0 200
```

#### MMAT

```sh
gcc -O2 ../examples/mat_ompd/mat_seq.c -o mmat_seq_out
./mmat_seq_out 1536 1536 1536 1536

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/mat_ompd/mat_omp.c -o mmat_omp_out
./mmat_omp_out 1536 1536 1536 1536

./fparse ../examples/mat_ompd/prueba_mmat.c log.txt error.txt prueba_mmat_output.c
mpicc -O2 prueba_mmat_output.c -o prueba_mmat_out
mpirun -np 4 ./prueba_mmat_out 1536 1536 1536 1536
mpicc -O2 -fopenmp prueba_mmat_output.c -o prueba_mmat_out
mpirun -np 4 ./prueba_mmat_out 1536 1536 1536 1536

./fparse ../examples/mat_ompd/mat_ompd.c log.txt error.txt mat_ompd_output.c
mpicc -O2 mat_ompd_output.c -o mat_ompd_out
mpirun -np 4 ./mat_ompd_out 1536 1536 1536 1536
mpicc -O2 -fopenmp mat_ompd_output.c -o mat_ompd_out
mpirun -np 4 ./mat_ompd_out 1536 1536 1536 1536
```

#### Mandelbrot

```sh
gcc -O2 ../examples/mandelbrot/Mandelbrot-SEQ.c -o mandelbrot_seq_out -lm
./mandelbrot_seq_out

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/mandelbrot/Mandelbrot-OMP.c -o mandelbrot_omp_out -lm
./mandelbrot_omp_out

./fparse ../examples/mandelbrot/prueba_mandel.c log.txt error.txt prueba_mandel_output.c
mpicc -O2 prueba_mandel_output.c -o prueba_mandel_out -lm
mpirun -np 4 ./prueba_mandel_out
mpicc -O2 -fopenmp prueba_mandel_output.c -o prueba_mandel_out -lm
mpirun -np 4 ./prueba_mandel_out

./fparse ../examples/mandelbrot/Mandelbrot-OMPD.c log.txt error.txt mandelbrot_ompd_output.c
mpicc -O2 mandelbrot_ompd_output.c -o mandelbrot_ompd_out -lm
mpirun -np 4 ./mandelbrot_ompd_out
mpicc -O2 -fopenmp mandelbrot_ompd_output.c -o mandelbrot_ompd_out -lm
mpirun -np 4 ./mandelbrot_ompd_out
```

#### MD

```sh
gcc -O2 ../examples/MD/mdSEQ.c -o md_seq_out -lm
./md_seq_out 3 800 700 0.01

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/MD/mdOMP.c -o md_omp_out -lm
./md_omp_out 3 800 700 0.01

./fparse ../examples/MD/mdOMPD.c log.txt error.txt md_ompd_output.c
mpicc -O2 md_ompd_output.c -o md_ompd_out -lm
mpirun -np 4 ./md_ompd_out 3 800 700 0.01
mpicc -O2 -fopenmp md_ompd_output.c -o md_ompd_out -lm
mpirun -np 4 ./md_ompd_out 3 800 700 0.01
```

#### Heated plate

```sh
gcc -O2 ../examples/halo/heated_plate/heated_plate_SEQ.c -o heated_plate_seq_out -lm
./heated_plate_seq_out 0.001 heated_plate_seq_result.txt

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/halo/heated_plate/heated_plate_OMP.c -o heated_plate_omp_out -lm
./heated_plate_omp_out 0.001 heated_plate_omp_result.txt

./fparse ../examples/halo/heated_plate/heated_plate_OMPD.c log.txt error.txt heated_plate_ompd_output.c
mpicc -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
mpicc -fopenmp -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
```

#### Game of life

```sh
gcc -O2 ../examples/halo/game_of_life/game_of_life_SEQ.c -o game_of_life_seq_out -lm
./game_of_life_seq_out 1000 1000 8000 0

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/halo/game_of_life/game_of_life_OMP.c -o game_of_life_omp_out -lm
./game_of_life_omp_out 1000 1000 8000 0

./fparse ../examples/halo/game_of_life/game_of_life_OMPD.c log.txt error.txt game_of_life_ompd_output.c
mpicc -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out -lm
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
mpicc -fopenmp -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out -lm
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
```

#### Image halo

```sh
gcc -O2 ../examples/halo/image_filters/image_halo_SEQ.c -o image_halo_seq_out -lm
./image_halo_seq_out g 5 ../examples/halo/image_filters/lenna.pgm output_SEQ_g5x5.pgm 1000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/halo/image_filters/image_halo_OMP.c -o image_halo_omp_out -lm
./image_halo_omp_out g 5 ../examples/halo/image_filters/lenna.pgm output_OMP_g5x5.pgm 1000

./fparse ../examples/halo/image_filters/image_halo_OMPD.c log.txt error.txt image_halo_ompd_output.c
mpicc -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/halo/image_filters/lenna.pgm output_OMPD_g5x5.pgm 1000
mpicc -fopenmp -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/halo/image_filters/lenna.pgm output_OMPD_g5x5.pgm 1000
```

#### Tasking: Fibonacci

```sh
gcc -O2 ../examples/tasks/fibonacci/fibonacci-seq.c -o fibonacci_seq_out
./fibonacci_seq_out 50

export OMP_NUM_THREADS=8
gcc -O2 -fopenmp ../examples/tasks/fibonacci/fibonacci-omp.c -o fibonacci_omp_out
./fibonacci_omp_out 50 30

./fparse ../examples/tasks/fibonacci/fibonacci-ompd.c log.txt error.txt fibonacci_ompd_output.c
mpicc -O2 fibonacci_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o fibonacci_ompd_out
mpirun -np 8 ./fibonacci_ompd_out 50 30
mpirun --use-hwthread-cpus -np 16 ./fibonacci_ompd_out 50 30
```

#### Tasking: PI

```sh
gcc -O2 ../examples/tasks/pi_task/pi_task_seq.c -o pi_task_seq_out -lm
./pi_task_seq_out 1000000000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/pi_task/pi_task_omp.c -o pi_task_omp_out -lm
./pi_task_omp_out 1000000000

./fparse ../examples/tasks/pi_task/pi_task_ompd.c log.txt error.txt pi_task_ompd_output.c
mpicc -O2 pi_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -lm -o pi_task_ompd_out
mpirun -np 4 ./pi_task_ompd_out 1000000000
```

#### Tasking: Binomial

```sh
gcc -O2 ../examples/tasks/binomial/binomial_seq.c -o binomial_seq_out
./binomial_seq_out 35 17

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/binomial/binomial_omp.c -o binomial_omp_out
./binomial_omp_out 35 17 20

./fparse ../examples/tasks/binomial/binomial_task_ompd.c log.txt error.txt binomial_task_ompd_output.c
mpicc -O2 binomial_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o binomial_task_ompd_out
mpirun -np 4 ./binomial_task_ompd_out 35 17 20
```

#### Tasking: Pipeline

```sh
gcc -O2 ../examples/tasks/pipeline/pipeline_seq.c -o pipeline_seq_out
./pipeline_seq_out

export OMP_NUM_THREADS=5
gcc -O2 -fopenmp ../examples/tasks/pipeline/pipeline_omp.c -o pipeline_omp_out
./pipeline_omp_out

./fparse ../examples/tasks/pipeline/pipeline_ompd.c log.txt error.txt pipeline_ompd_output.c
mpicc -O2 pipeline_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o pipeline_ompd_out
mpirun -np 5 ./pipeline_ompd_out
```

#### Tasking: Inventario (`inout`)

```sh
gcc -O2 ../examples/tasks/inventory/inventory_seq.c -o inventory_seq_out
./inventory_seq_out

./fparse ../examples/tasks/inventory/inventory_ompd.c log.txt error.txt inventory_ompd_output.c
mpicc -O2 inventory_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o inventory_ompd_out
mpirun -np 4 ./inventory_ompd_out
```

#### Tasking: Simpson

```sh
gcc -O2 ../examples/tasks/simpson/simpson_seq.c -lm -o simpson_seq_out
./simpson_seq_out 1e-8 20

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/simpson/simpson_omp.c -lm -o simpson_omp_out
./simpson_omp_out 1e-8 20 10

./fparse ../examples/tasks/simpson/simpson_task_ompd.c log.txt error.txt simpson_task_ompd_output.c
mpicc -O2 simpson_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -lm -o simpson_task_ompd_out
mpirun -np 4 ./simpson_task_ompd_out 1e-8 20 10
```

## Arquitectura

El traductor trabaja en dos pasadas. La primera lee una copia preprocesada del fuente y guarda declaraciones y tipos. La segunda lee el fuente original, copia su código C a la salida y aplica las transformaciones de OpenMPD. No hay una única representación intermedia para todo el fichero C.

```text
entrada.c
  -> primera pasada: mpicc -E -P -include mpi.h
       preprolexer.ll + preproparser.yy
       symbol_table.h
  -> segunda pasada sobre entrada.c
       C99-scanner.lex + C99-parser.yacc
       omplexer.ll + ompparser.yy
       writer.cc + módulos transform
  -> salida.c
```

`main.cc` controla las dos pasadas y gestiona los ficheros de entrada, diagnóstico y salida. Para la primera pasada ejecuta `mpicc -E -P -include mpi.h`. `preprolexer.ll` y `preproparser.yy` leen su salida y guardan las declaraciones y la información de tipos en `symbol_table.h`.

En la segunda pasada, `main.cc` vuelve al fuente original y llama a `yyparse()`. `C99-parser.yacc` define la gramática de C y `C99-scanner.lex` proporciona sus tokens. El scanner lee por separado las líneas del preprocesador y envía los pragmas OpenMPD a `parse_openmp_pragma()`. `omplexer.ll` tokeniza cada directiva y `ompparser.yy` analiza sus cláusulas.

El scanner envía los tokens C restantes a `writer.cc`. El writer construye una línea cada vez y, según el estado del traductor, la copia, la almacena, la reescribe o la captura.

`pragma_args.h/.c` guarda los argumentos de la directiva actual. `translator_state.h/.c` y `cluster_stack.h/.c` guardan el estado de construcciones como `cluster`, `distribute`, `master`, `halo` y `num_teams`. Los módulos transform usan este estado para generar código MPI.

`mpi_lifecycle` genera la inicialización y finalización de MPI y las condiciones por proceso. `distribute_transform` reescribe los límites de los bucles y genera el reparto de trabajo. Los módulos de scatter/gather, reducción, memoria y tipos generan las operaciones MPI correspondientes. `halo_transform` guarda los límites del último bucle distribuido y los usa para generar las llamadas a `MPI_Sendrecv` de una directiva `update halo` posterior.

Tasking también usa la segunda pasada, pero los cuerpos de tarea se capturan en vez de copiarse directamente. Los módulos de tasking guardan el cuerpo de cada `task_async` y generan una función para él. En la posición original de la directiva generan el código que crea la tarea, registra sus dependencias y la envía al runtime.

El planificador selecciona normalmente la tarea que ha pasado a estar lista más recientemente (LIFO). En `taskwait`, su worker solo selecciona tareas listas creadas por la tarea que espera, directamente o a través de otras tareas. Si hay varias, selecciona la más reciente.

Al terminar el análisis, `main.cc` vacía el writer y aplica los slots diferidos. Estos slots contienen cabeceras, declaraciones globales y definiciones de tareas que deben aparecer antes en el fichero generado.

## Directivas soportadas

El repositorio contiene tres grupos principales de construcciones traducidas:

- **Distribución de datos y trabajo.** Una región `cluster` puede reservar y difundir datos con `alloc` y `broad`, mover arrays con `scatter`, `gather` y `allgather`, aplicar `reduction` y `allreduction`, y distribuir las iteraciones de un bucle. `declare cluster` genera declaraciones de tipos MPI.
- **Intercambio de halo.** Una cláusula `halo(...)` declarada en un `cluster` marca el array que necesita comunicación de bordes fantasma. Después, `cluster distribute update halo(...)` usa los límites del último bucle distribuido y emite intercambios con los vecinos para las filas superior e inferior del bloque local.
- **Tasking.** El traductor soporta `task_async` y `taskwait`. Captura el cuerpo de cada tarea asíncrona y genera su función y su estructura de entrada. `ompd_runtime.c` gestiona las dependencias y selecciona las tareas listas en orden LIFO. En `taskwait`, solo selecciona trabajo creado por la tarea que espera, directamente o a través de otras tareas. `depend(inout: ...)` conserva el valor inicial y serializa las tareas de lectura-modificación-escritura sobre una misma dependencia.

## Mapa de ficheros

### Punto de entrada y flujo

| Fichero | Responsabilidad |
|---|---|
| `main.cc` | Orquesta las dos pasadas, abre ficheros, lanza el preprocesador, aplica los slots diferidos al final. |
| `symbol_table.h` | Tabla de símbolos con ámbitos: tipos, variables y declaraciones recogidas en la pasada de preprocesado. |
| `preprolexer.ll`, `preproparser.yy` | Scanner y parser de la pasada de preprocesado. |
| `C99-scanner.lex`, `C99-parser.yacc` | Scanner y parser del C original en la pasada principal. |
| `omplexer.ll`, `ompparser.yy` | Scanner y parser del texto de los pragmas OpenMPD. |
| `writer.h/.cc` | Reconstrucción del fuente y emisión de la salida: procesado de tokens, vaciado de líneas, bufferización, reescritura de bucles, captura de cuerpos de tarea y cierre de regiones según la directiva activa. |
| `output_slots.h/.cc` | Inserción de salida diferida mediante slots con nombre. |

### Estado del traductor

| Fichero | Responsabilidad |
|---|---|
| `translator_state.h/.c` | Estado global agrupado en structs (tasking, distribute, master, halo, num_teams) y sus accesores. Es el límite de compatibilidad con Flex/Bison. |
| `cluster_stack.h/.c` | Pila de regiones `cluster` activas y su estado de cierre. |
| `pragma_args.h/.c` | Argumentos del pragma que se está analizando y sus cláusulas. |

### Módulos transform

| Fichero | Responsabilidad |
|---|---|
| `distribute_transform.h/.cc` | Reparto de bucles entre procesos: activación, modos extendidos, reescritura de la cabecera del bucle, worksharing MPI, `dist_schedule`, forwarding de cláusulas OpenMP. |
| `halo_transform.h/.c` | Declara variables de halo, guarda límites de bucles distribuidos y genera intercambios `MPI_Sendrecv` con vecinos para `update halo`. |
| `scatter_gather_transform.h/.cc` | Generación de scatter, gather y allgather. |
| `reduction_transform.h/.cc` | Generación de reduce y allreduce. |
| `memory_transform.h/.cc` | Reserva de memoria y broadcast. |
| `mpi_type_transform.h/.cc` | Declaración de tipos de datos MPI. |
| `mpi_lifecycle.h/.cc` | Init/finalize de MPI, cabeceras y declaraciones globales del runtime, guardas master/secuencial, comprobación de `num_teams`. |
| `codegen_utils.h/.cc` | Utilidades compartidas de generación: conversión de tipos, formato de cadenas, índices. |

### Tasking (tareas asíncronas)

| Fichero | Responsabilidad |
|---|---|
| `tasking_region.h/.c` | Modelo de una región de tasking: bloques ordenados dentro de un `cluster`. |
| `task_async_block.h/.c` | Un bloque de tarea asíncrona: dependencias, entradas y texto del cuerpo. |
| `task_body_transform.h/.c` | Representación intermedia del cuerpo de la tarea y su código generado. |
| `tasking_emit.h/.cc` | Enruta la salida generada de una región terminada: las definiciones globales a su slot diferido y el cuerpo a la salida. |
| `ompd_runtime.h/.c` | Runtime de tasking que se compila con el programa generado: creación de tareas, dependencias, selección LIFO, `taskwait` y sincronización. |
| `hash_map.h/.c`, `task_utils.h/.c` | Estructuras y utilidades auxiliares de tasking. |

### Ficheros generados (no editar a mano)

`ompparser.cc`, `ompparser.hh`, `omplexer.cc`, `preproparser.cc`, `preproparser.hh`, `preprolexer.cc`, `y.tab.cc`, `y.tab.hh`, `lex.yy.cc`.

Los produce `bison`/`flex` a partir de los `.yy`, `.yacc`, `.ll` y `.lex`. Para cambiar el comportamiento de un parser o scanner se edita su fuente, no el fichero generado; `make` los vuelve a generar.

## Prueba de regresión

El script de prueba de regresión está en `OpenMpD/tests/regression/`. Para cada caso compila y ejecuta un programa secuencial y su equivalente OpenMPD, y después compara su salida numérica o sus ficheros.

Ejecuta estos comandos desde `OpenMpD/`. El target `regression-test` construye `fparse` cuando es necesario. `tests/regression/cases.tsv` define las fuentes, los argumentos, las librerías, el número de procesos y el método de comparación de cada caso. El script escribe los ficheros temporales y logs en `/tmp/ompd-regression` por defecto; define `WORK_ROOT` para usar otro directorio.

Ejecución normal:

```sh
make clean && make regression-test
```

Ejecución completa, incluyendo los casos lentos:

```sh
make fparse && RUN_SLOW=1 python3 tests/regression/run_regression.py
```

Ejecutar una familia concreta:

```sh
make fparse && FILTER_FAMILY=halo python3 tests/regression/run_regression.py
```

Resultados esperados con el árbol sano:

```text
normal:   summary: pass=18 skip=9 fail=0
completa: summary: pass=27 skip=0 fail=0
```

El script también acepta `CASES_FILE`, `WORK_ROOT` y `CASE_TIMEOUT` como variables de entorno. Para añadir un caso se agrega una fila a `tests/regression/cases.tsv`. La cabecera documenta sus columnas. Los placeholders de rutas y los perfiles de comparación disponibles están definidos en `run_regression.py`.

## Pruebas de rendimiento

Las pruebas de rendimiento usan los mismos números de procesos e hilos para todos los ejemplos:

- `run_performance.py` fija 2, 4, 8, 12 y 24 como única escala de procesos e hilos.
- `tests/performance/cases.tsv` enumera las variantes disponibles para cada ejemplo. Ningún caso puede definir una escala distinta.
- Los ejemplos de topología fija, como `pipeline` (5 equipos) e `inventory_inout` (4 equipos), permanecen en las pruebas de regresión y no forman parte de las pruebas de rendimiento.

Cada ejemplo tiene, por tanto, once medidas principales: una ejecución secuencial, cinco ejecuciones OpenMP con 2, 4, 8, 12 y 24 hilos, y cinco ejecuciones OpenMPD con el mismo número de procesos MPI. Usa `--include-ompd-openmp` para incluir las configuraciones OpenMPD+OpenMP disponibles.

El plan completo puede revisarse sin compilar ni ejecutar nada:

```sh
python3 tests/performance/run_performance.py --plan
```

Para ver todas las opciones de la línea de comandos:

```sh
python3 tests/performance/run_performance.py --help
```

Ejecuta todos los casos de `tests/performance/cases.tsv`. Por defecto, el script mide las variantes secuencial, OpenMP y OpenMPD y crea un CSV con fecha y hora en `tests/performance/results/`, un directorio de logs medidos con el sufijo `.logs` y otro directorio separado con el sufijo `.warmup.logs`:

```sh
python3 tests/performance/run_performance.py
```

El script construye `fparse`; no es necesario indicar una ruta de salida.

Ejecuta solo los casos de la familia `base`:

```sh
python3 tests/performance/run_performance.py --family base
```

La variante `ompd` es el programa MPI generado por OpenMPD. Una variante `mpi` distinta es una implementación MPI escrita a mano y se excluye por defecto. Se puede añadir con `--include-mpi`. Usa `--include-ompd-openmp` para las configuraciones híbridas disponibles. Si no existe una implementación MPI manual, indica `-` en `mpi_source` y omite `mpi` y `mpi_omp` de `variants`.

Antes de medir una configuración, el script la ejecuta una vez sin `perf` como comprobación previa. Así detecta un fallo inmediato de ejecución y esa ejecución no entra en la media medida. Su salida estándar y sus errores se guardan por separado en el directorio `.warmup.logs`; si el *warmup* falla, su diagnóstico también se conserva en el fichero `.error.log` correspondiente del directorio de logs medidos.

Después, el script ejecuta la misma configuración cinco veces con `perf stat`. Cada fila del CSV contiene dos medias. `mean_seconds` incluye el lanzador y el ciclo de vida de MPI. `calculation_mean_seconds` usa el valor `OMPD_CALC_TIME_SECONDS=<segundos>` que imprime el ejemplo alrededor del cálculo. El script exige un valor en cada ejecución medida.

## Notas de desarrollo

Para estudiar el traductor, construye `fparse`, traduce un ejemplo pequeño y compara el programa MPI generado con su fuente OpenMPD. El recorrido principal pasa por `main.cc`, `C99-scanner.lex`, `C99-parser.yacc`, `omplexer.ll`, `ompparser.yy` y `writer.cc`. Después, cada directiva se trata en su módulo transform correspondiente.

`tests/regression/cases.tsv` ofrece una lista práctica de los ejemplos soportados, sus argumentos, dependencias de runtime y métodos de comparación.

Trabajo futuro:

- Añadir más ejemplos y casos de regresión con distintos tamaños de entrada, números de procesos y combinaciones de pragmas ya existentes.
- Completar más partes del modelo de tasking de OpenMP: añadir directivas y cláusulas pendientes, mejorar el tratamiento de dependencias y soportar cuerpos de tarea más complejos.
- Definir qué construcciones OpenMPD pueden combinarse con tasking. El parser actual mantiene los clusters con tasking en un camino separado y rechaza algunos pragmas no-task al mismo nivel del cluster.
- Ampliar el soporte de halo más allá de repartos por filas, incluyendo distribuciones bidimensionales, columnas y descomposiciones por bloques.
- Mejorar la gestión de memoria del traductor. Valgrind sigue mostrando memoria sin liberar al terminar el proceso, sobre todo alrededor de los objetos `symbol_info` de la tabla de símbolos; el código necesita una regla más clara sobre qué parte crea esos objetos y qué parte debe liberarlos.
- Considerar una representación intermedia en vez de transformar el fuente línea a línea durante el parseo. Esto separaría el análisis de la generación de código y podría simplificar la traducción de cuerpos de tarea, las comprobaciones semánticas y la generación de MPI.
