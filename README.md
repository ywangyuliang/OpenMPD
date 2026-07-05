# OpenMPD

- **Authors:** Yuliang Wang, Caijie Wu
- **Tutor:** Antonio García Dopico
- **Department:** Departamento de Arquitectura y Tecnología de Sistemas Informáticos (DATSI), Universidad Politécnica de Madrid (UPM)

**English** · [Español](#openmpd-español)

OpenMPD is a source-to-source translator. It takes a C program annotated with OpenMPD pragmas and generates a C program with MPI calls, ready to compile and run across several processes. The directive model is close to OpenMP, but designed for clusters.

The goal is to let the programmer describe distributed-memory parallelism with directives instead of writing the MPI communication layer manually. OpenMPD supports data distribution, collective data movement, reductions, halo exchange for stencil-style computations and runtime-managed asynchronous tasks.

The translator is built as a single executable, `fparse`.

## Repository layout

```
OMPD/
├── OpenMpD/
│   ├── include/   translator headers
│   ├── src/       translator and runtime sources
│   ├── syntax/    lexer and parser sources
│   ├── scripts/   helper scripts
│   └── tests/     regression tests
├── examples/      example programs (sequential, OpenMP, OpenMPD, MPI)
└── README.md      this file
```

## Build and run

All commands below are run from the `OpenMpD/` directory.

### 1. Build the translator

```sh
make fparse
```

This generates the parsers and scanners from their sources and links the `fparse` executable. Build from a clean tree before trusting a result, since a stale `fparse` binary can hide failures:

```sh
make clean && make fparse
```

### 2. Translate an OpenMPD program

The `examples/` directory holds ready-to-use OpenMPD programs grouped by area (`pi`, `halo`, `image_halo`, `game_of_life`, `tasks`, ...), so picking one as the input is the easiest way to try the translator.

`fparse` takes the input program followed by three output paths: two log files and the generated C/MPI program.

```sh
./fparse input.c log.txt error.txt name_output.c
```

Use the `*_output.c` suffix for generated C files. The repository ignores that pattern, along with the default `log.txt` and `error.txt` files:

```sh
./fparse ../examples/image_halo/image_halo_OMPD.c log.txt error.txt image_halo_output.c
```

`image_halo_output.c` is the generated program; `log.txt` and `error.txt` are the translation logs. If the output path is omitted, the translator only analyses the input without writing anything.

### 3. Compile and run the generated program

Compile the generated program with `mpicc` and run it across several processes with `mpirun`. Add the libraries and arguments the example expects; both are listed for every example in `tests/regression/cases.tsv`:

```sh
mpicc -O2 image_halo_output.c -fopenmp -lm -o image_halo_out
mpirun -np 4 ./image_halo_out gaussian 3 ../examples/image_halo/lenna.pgm result.pgm
```

If the program uses tasks (`task_async`, `taskwait`, ...), also compile the tasking runtime next to it:

```sh
mpicc -O2 task_example_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o task_example_out
```

### 4. Complete example commands

The following commands are meant to be run from `OpenMpD/`. Build `fparse` first with `make clean && make fparse`.

#### PI

```sh
gcc -O2 ../examples/pi/pi_seq.c -o pi_seq_out
./pi_seq_out

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/pi/pi_omp.c -o pi_omp_out
./pi_omp_out

./fparse ../examples/pi/pi_test1.c log.txt error.txt pi_test1_output.c
mpicc -O2 pi_test1_output.c -o pi_test1_out
mpirun -np 4 ./pi_test1_out
mpicc -O2 -fopenmp pi_test1_output.c -o pi_test1_out
mpirun -np 4 ./pi_test1_out

./fparse ../examples/pi/pi_ompd.c log.txt error.txt pi_ompd_output.c
mpicc -O2 pi_ompd_output.c -o pi_ompd_out
mpirun -np 4 ./pi_ompd_out
mpicc -O2 -fopenmp pi_ompd_output.c -o pi_ompd_out
mpirun -np 4 ./pi_ompd_out
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
gcc -O2 ../examples/mmat/IJK/mmat_ijk.c -o mmat_seq_out
./mmat_seq_out 1536 1536 1536 1536

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/mmat/IJK/mmat_ijk_omp.c -o mmat_omp_out
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
gcc -O2 ../examples/Heated/heated_plate_SEQ.c -o heated_plate_seq_out -lm
./heated_plate_seq_out 0.001 heated_plate_seq_result.txt

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/Heated/heated_plate_OMP.c -o heated_plate_omp_out -lm
./heated_plate_omp_out 0.001 heated_plate_omp_result.txt

./fparse ../examples/Heated/heated_plate_OMPD.c log.txt error.txt heated_plate_ompd_output.c
mpicc -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
mpicc -fopenmp -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
```

#### Game of life

```sh
gcc -O2 ../examples/game_of_life/game_of_life_SEQ.c -o game_of_life_seq_out
./game_of_life_seq_out 1000 1000 8000 0

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/game_of_life/game_of_life_OMP.c -o game_of_life_omp_out
./game_of_life_omp_out 1000 1000 8000 0

./fparse ../examples/game_of_life/game_of_life_OMPD.c log.txt error.txt game_of_life_ompd_output.c
mpicc -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
mpicc -fopenmp -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
```

#### Image halo

```sh
gcc -O2 ../examples/image_halo/image_halo_SEQ.c -o image_halo_seq_out -lm
./image_halo_seq_out g 5 ../examples/image_halo/lenna.pgm output_SEQ_g5x5.pgm 1000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/image_halo/image_halo_OMP.c -o image_halo_omp_out -lm
./image_halo_omp_out g 5 ../examples/image_halo/lenna.pgm output_OMP_g5x5.pgm 1000

./fparse ../examples/image_halo/image_halo_OMPD.c log.txt error.txt image_halo_ompd_output.c
mpicc -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/image_halo/lenna.pgm output_OMPD_g5x5.pgm 1000
mpicc -fopenmp -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/image_halo/lenna.pgm output_OMPD_g5x5.pgm 1000
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
gcc -O2 ../examples/tasks/pi_task/pi_task_seq.c -o pi_task_seq_out
./pi_task_seq_out 1000000000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/pi_task/pi_task_omp.c -o pi_task_omp_out
./pi_task_omp_out 1000000000

./fparse ../examples/tasks/pi_task/pi_task_ompd.c log.txt error.txt pi_task_ompd_output.c
mpicc -O2 pi_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o pi_task_ompd_out
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

The translator reads the input program twice. The first pass works on a preprocessed version of the source and only fills semantic information. The second pass goes back to the original file, reconstructs its output text and applies the OpenMPD transformations while it reads. This keeps the generated program close to the input program, without building a global intermediate representation for the whole C file.

```text
input.c
  -> main.cc
  -> preprocessor pass
       mpicc -E -P -include mpi.h
       preprolexer.ll + preproparser.yy
       symbol_table.h
  -> main pass over the original source
       C99-scanner.lex + C99-parser.yacc
       omplexer.ll + ompparser.yy
       writer.cc + output_slots.h/.cc
       *_transform modules + mpi_lifecycle
       tasking model + tasking runtime hooks
  -> output.c
```

`main.cc` is the entry point. It opens the input, log, error and optional output files, reserves the deferred output slots, and launches `mpicc -E -P -include mpi.h`. The temporary preprocessed file is read by `preprolexer.ll` and `preproparser.yy`, which fill `symbol_table.h` with declarations and type information that may not appear explicitly in the original file.

After the preprocessor pass, `main.cc` rewinds the original source and calls `yyparse()`. In this pass, `C99-parser.yacc` drives the C grammar and `C99-scanner.lex` provides the tokens. When the scanner finds a source line that starts with `#`, it reads the whole line separately instead of processing it as ordinary C tokens. If that line is an OpenMPD pragma, it is sent to `parseOpenMP()`, where `omplexer.ll` tokenizes the directive and `ompparser.yy` interprets its clauses.

The rest of the C source goes through the writer. For each normal token, the scanner passes its text to `writer.cc` with `writer_set_token_text()` and then calls `writer_process_current_token()`. The writer keeps the current source line under construction. When a newline arrives, it closes that line and checks the active translator state to decide whether the line is copied unchanged, buffered for a pending transformation, rewritten or captured for tasking.

The pragma parser stores the current clause arguments in `pragma_args.h/.c`. State that must survive beyond a single clause, such as active `cluster`, `distribute`, `master`, `halo` or `num_teams` information, is kept in `translator_state.h/.c` and `cluster_stack.h/.c`. That state is the bridge between the parsed directive and the transform modules that generate MPI code.

The transform modules are responsible for the concrete MPI fragments: `mpi_lifecycle` emits initialization/finalization and process guards, `distribute_transform` rewrites loop bounds and emits worksharing, `scatter_gather_transform`, `reduction_transform`, `memory_transform` and `mpi_type_transform` generate data movement, reductions, allocation/broadcast and datatype declarations. `halo_transform` is connected to distributed loops: the distributed-loop bounds are stored when the loop is rewritten, and a later `update halo` emits the neighbour `MPI_Sendrecv` exchanges when the region is closed.

Tasking follows the same main pass but does not copy task bodies directly. `tasking_region.h/.c` and `task_async_block.h/.c` record the tasking region and its async blocks, while `task_body_transform.h/.c` builds a small representation of each task body. `tasking_emit.h/.cc` then emits the generated task functions and `ompd_execute_generated_task` through `output_slots.h/.cc`, and leaves at the original `task_async` position the code that builds the task, registers its dependencies and submits it to `ompd_runtime.h/.c`.

At the end of the parse, `main.cc` flushes the writer and applies the deferred slots. This final step inserts code that was discovered during the traversal but must appear earlier in the generated file, such as headers, global declarations or generated task definitions.

## Supported Directives

The repository contains three main groups of translated constructs:

- **Data and work distribution.** `cluster`, `distribute`, `scatter`, `gather`, `allgather`, reductions, broadcasts, memory allocation and MPI datatype declarations are translated directly into MPI fragments by the transform modules.
- **Halo exchange.** A `halo(...)` clause declared on a `cluster` marks the array that needs ghost-border communication. A later `cluster distribute update halo(...)` uses the bounds of the last distributed loop and emits neighbour exchanges for the rows above and below the local block.
- **Tasking.** `task_async`, `taskwait`, `taskgroup` and `taskyield` are not lowered into one fixed MPI sequence. The translator captures task bodies, generates task functions and input structures, and relies on `ompd_runtime.c` to create, schedule and synchronize tasks between MPI processes.

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
| `ompd_runtime.h/.c` | Tasking runtime compiled *into* the generated program (task creation, dependencies, scheduling, synchronization). |
| `hash_map.h/.c`, `task_utils.h/.c` | Auxiliary tasking structures and utilities. |

### Generated files (do not edit by hand)

`ompparser.cc`, `ompparser.hh`, `omplexer.cc`, `preproparser.cc`, `preproparser.hh`, `preprolexer.cc`, `y.tab.cc`, `y.tab.hh`, `lex.yy.cc`.

They are produced by `bison`/`flex` from the `.yy`, `.yacc`, `.ll` and `.lex` files. To change the behaviour of a parser or scanner, edit its source, not the generated file; `make` regenerates them.

## Regression tests

The regression tests live in `OpenMpD/tests/regression/`. They are integration tests: for each case, the runner compiles and runs a sequential version and the OpenMPD version of the same program, extracts the computed result and compares both. It does not check success messages, but the numeric result.

Build `fparse` first and run the tests from the `OpenMpD/` directory. `cases.tsv` lists the examples and `run_regression.py` compiles, runs and compares each row. Generated files and logs are written outside the repository by default, under `WORK_ROOT`.

Normal test run:

```sh
python3 tests/regression/run_regression.py
```

Full test run, including the slow cases:

```sh
RUN_SLOW=1 python3 tests/regression/run_regression.py
```

Run one family:

```sh
FILTER_FAMILY=halo python3 tests/regression/run_regression.py
```

Expected results on a healthy tree:

```text
normal: summary: pass=15 skip=9 fail=0
full:   summary: pass=24 skip=0 fail=0
```

To add a case, append a row to `cases.tsv`. Its header documents the columns, path placeholders and comparison profiles.

## Continuing the project

The current repository contains a working source-to-source translator for C programs annotated with OpenMPD pragmas. It covers the main translation flow: preprocessing and symbol collection, pragma parsing, source reconstruction, MPI lifecycle generation, distributed loops, collective movement, reductions, memory management, MPI datatype declarations, halo exchange and runtime-based asynchronous tasking.

The recommended way to start is to build `fparse`, translate a small example and compare the generated MPI program with the original OpenMPD source. After that, read the architecture section and follow the execution path through `main.cc`, `C99-scanner.lex`, `C99-parser.yacc`, `omplexer.ll`, `ompparser.yy` and `writer.h/.cc`. For a concrete directive, continue in the matching transform module: for example `distribute_transform`, `halo_transform`, `scatter_gather_transform`, `reduction_transform`, `memory_transform`, `mpi_type_transform` or `task_body_transform`.

For behaviour-oriented work, `tests/regression/cases.tsv` is the practical map of the project: it shows which examples are currently exercised, which arguments they need, which cases require the tasking runtime and which result profile is used for comparison.

Future work:

- Add more examples and regression cases with different input sizes, process counts and combinations of existing pragmas.
- Complete more of the OpenMP tasking model: add missing directives and clauses, improve dependency handling and support more complex task bodies.
- Define which OpenMPD constructs can be combined with tasking. The current parser keeps tasking clusters on a separate path and rejects some non-task pragmas at the same cluster level.
- Expand halo support beyond row-wise partitions, including two-dimensional distributions, columns and block-based decompositions.
- Improve translator memory management. Valgrind still reports memory left allocated at process exit, mainly around `symbol_info` objects in the symbol table; the code needs a clearer rule for which part creates those objects and which part releases them.
- Study a larger refactor towards an intermediate representation instead of transforming the source line by line during parsing. This is a costly architectural change, but it is closer to how modern compilers separate analysis from code generation and could make task-body translation, semantic checks and MPI generation more robust.

---

# OpenMPD (Español)

- **Autores:** Yuliang Wang, Caijie Wu
- **Tutor:** Antonio García Dopico
- **Departamento:** Departamento de Arquitectura y Tecnología de Sistemas Informáticos (DATSI), Universidad Politécnica de Madrid (UPM)

[English](#openmpd) · **Español**

OpenMPD es un traductor *source-to-source*. Toma un programa en C anotado con pragmas propios de OpenMPD y genera un programa en C con llamadas MPI listo para compilar y ejecutar en varios procesos. El modelo de directivas se parece a OpenMP, pero está pensado para clústeres.

El objetivo es permitir que el programador describa paralelismo de memoria distribuida mediante directivas, sin escribir manualmente toda la capa de comunicación MPI. OpenMPD soporta distribución de datos, movimiento colectivo de datos, reducciones, intercambio de halos para cálculos tipo stencil y tareas asíncronas gestionadas por runtime.

El traductor se construye como un único ejecutable, `fparse`.

## Estructura del repositorio

```
OMPD/
├── OpenMpD/
│   ├── include/   cabeceras del traductor
│   ├── src/       fuentes del traductor y del runtime
│   ├── syntax/    fuentes de lexer y parser
│   ├── scripts/   scripts auxiliares
│   └── tests/     pruebas de regresión
├── examples/      programas de ejemplo (secuencial, OpenMP, OpenMPD, MPI)
└── README.md      este fichero
```

## Construir y ejecutar

Todos los comandos de abajo se ejecutan desde el directorio `OpenMpD/`.

### 1. Construir el traductor

```sh
make fparse
```

Esto genera los parsers y scanners a partir de sus fuentes y enlaza el ejecutable `fparse`. Conviene construir desde un árbol limpio antes de fiarse de un resultado, porque un binario `fparse` antiguo puede ocultar fallos:

```sh
make clean && make fparse
```

### 2. Traducir un programa OpenMPD

El directorio `examples/` contiene programas OpenMPD listos para usar, agrupados por área (`pi`, `halo`, `image_halo`, `game_of_life`, `tasks`, ...), así que tomar uno como entrada es la forma más sencilla de probar el traductor.

`fparse` recibe el programa de entrada seguido de tres rutas de salida: dos ficheros de log y el programa C/MPI generado.

```sh
./fparse entrada.c log.txt error.txt nombre_output.c
```

Usa el sufijo `*_output.c` para los C generados. El repositorio ignora ese patrón, además de los ficheros por defecto `log.txt` y `error.txt`:

```sh
./fparse ../examples/image_halo/image_halo_OMPD.c log.txt error.txt image_halo_output.c
```

`image_halo_output.c` es el programa generado; `log.txt` y `error.txt` son los registros de la traducción. Si se omite la ruta de salida, el traductor solo analiza la entrada sin escribir nada.

### 3. Compilar y ejecutar el programa generado

Se compila el programa generado con `mpicc` y se ejecuta en varios procesos con `mpirun`. Añade las librerías y los argumentos que pide el ejemplo; ambos están listados para cada ejemplo en `tests/regression/cases.tsv`:

```sh
mpicc -O2 image_halo_output.c -fopenmp -lm -o image_halo_out
mpirun -np 4 ./image_halo_out gaussian 3 ../examples/image_halo/lenna.pgm result.pgm
```

Si el programa usa tareas (`task_async`, `taskwait`, ...), hay que compilar también el runtime de tasking junto a él:

```sh
mpicc -O2 task_example_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o task_example_out
```

### 4. Comandos completos de ejemplos

Los comandos siguientes se ejecutan desde `OpenMpD/`. Construye antes `fparse` con `make clean && make fparse`.

#### PI

```sh
gcc -O2 ../examples/pi/pi_seq.c -o pi_seq_out
./pi_seq_out

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/pi/pi_omp.c -o pi_omp_out
./pi_omp_out

./fparse ../examples/pi/pi_test1.c log.txt error.txt pi_test1_output.c
mpicc -O2 pi_test1_output.c -o pi_test1_out
mpirun -np 4 ./pi_test1_out
mpicc -O2 -fopenmp pi_test1_output.c -o pi_test1_out
mpirun -np 4 ./pi_test1_out

./fparse ../examples/pi/pi_ompd.c log.txt error.txt pi_ompd_output.c
mpicc -O2 pi_ompd_output.c -o pi_ompd_out
mpirun -np 4 ./pi_ompd_out
mpicc -O2 -fopenmp pi_ompd_output.c -o pi_ompd_out
mpirun -np 4 ./pi_ompd_out
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
gcc -O2 ../examples/mmat/IJK/mmat_ijk.c -o mmat_seq_out
./mmat_seq_out 1536 1536 1536 1536

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/mmat/IJK/mmat_ijk_omp.c -o mmat_omp_out
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
gcc -O2 ../examples/Heated/heated_plate_SEQ.c -o heated_plate_seq_out -lm
./heated_plate_seq_out 0.001 heated_plate_seq_result.txt

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/Heated/heated_plate_OMP.c -o heated_plate_omp_out -lm
./heated_plate_omp_out 0.001 heated_plate_omp_result.txt

./fparse ../examples/Heated/heated_plate_OMPD.c log.txt error.txt heated_plate_ompd_output.c
mpicc -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
mpicc -fopenmp -O2 heated_plate_ompd_output.c -o heated_plate_ompd_out -lm
mpirun -np 4 ./heated_plate_ompd_out 0.001 heated_plate_ompd_result.txt
```

#### Game of life

```sh
gcc -O2 ../examples/game_of_life/game_of_life_SEQ.c -o game_of_life_seq_out
./game_of_life_seq_out 1000 1000 8000 0

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/game_of_life/game_of_life_OMP.c -o game_of_life_omp_out
./game_of_life_omp_out 1000 1000 8000 0

./fparse ../examples/game_of_life/game_of_life_OMPD.c log.txt error.txt game_of_life_ompd_output.c
mpicc -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
mpicc -fopenmp -O2 game_of_life_ompd_output.c -o game_of_life_ompd_out
mpirun -np 4 ./game_of_life_ompd_out 1000 1000 8000 0
```

#### Image halo

```sh
gcc -O2 ../examples/image_halo/image_halo_SEQ.c -o image_halo_seq_out -lm
./image_halo_seq_out g 5 ../examples/image_halo/lenna.pgm output_SEQ_g5x5.pgm 1000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/image_halo/image_halo_OMP.c -o image_halo_omp_out -lm
./image_halo_omp_out g 5 ../examples/image_halo/lenna.pgm output_OMP_g5x5.pgm 1000

./fparse ../examples/image_halo/image_halo_OMPD.c log.txt error.txt image_halo_ompd_output.c
mpicc -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/image_halo/lenna.pgm output_OMPD_g5x5.pgm 1000
mpicc -fopenmp -O2 image_halo_ompd_output.c -o image_halo_ompd_out -lm
mpirun -np 4 ./image_halo_ompd_out g 5 ../examples/image_halo/lenna.pgm output_OMPD_g5x5.pgm 1000
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
gcc -O2 ../examples/tasks/pi_task/pi_task_seq.c -o pi_task_seq_out
./pi_task_seq_out 1000000000

export OMP_NUM_THREADS=4
gcc -O2 -fopenmp ../examples/tasks/pi_task/pi_task_omp.c -o pi_task_omp_out
./pi_task_omp_out 1000000000

./fparse ../examples/tasks/pi_task/pi_task_ompd.c log.txt error.txt pi_task_ompd_output.c
mpicc -O2 pi_task_ompd_output.c src/ompd_runtime.c src/hash_map.c -Iinclude -o pi_task_ompd_out
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

El traductor lee el programa de entrada dos veces. La primera pasada trabaja sobre una versión preprocesada del fuente y solo recopila información semántica. La segunda vuelve al fichero original, reconstruye el texto de salida y aplica las transformaciones de OpenMPD mientras lo recorre. Así el programa generado se mantiene cercano al original, sin construir una representación intermedia global de todo el fichero C.

```text
entrada.c
  -> main.cc
  -> pasada de preprocesado
       mpicc -E -P -include mpi.h
       preprolexer.ll + preproparser.yy
       symbol_table.h
  -> pasada principal sobre el fuente original
       C99-scanner.lex + C99-parser.yacc
       omplexer.ll + ompparser.yy
       writer.cc + output_slots.h/.cc
       módulos *_transform + mpi_lifecycle
       modelo de tasking + llamadas al runtime
  -> salida.c
```

`main.cc` es el punto de entrada. Abre los ficheros de entrada, log, errores y salida opcional, reserva los slots diferidos y lanza `mpicc -E -P -include mpi.h`. El fichero temporal preprocesado lo leen `preprolexer.ll` y `preproparser.yy`, que rellenan `symbol_table.h` con declaraciones e información de tipos que pueden no aparecer explícitamente en el fichero original.

Después de la pasada de preprocesado, `main.cc` vuelve al fuente original y llama a `yyparse()`. En esta pasada, `C99-parser.yacc` dirige la gramática de C y `C99-scanner.lex` proporciona los tokens. Cuando el scanner encuentra una línea del fuente que empieza por `#`, lee la línea completa por separado en lugar de procesarla como tokens normales de C. Si esa línea es un pragma OpenMPD, se envía a `parseOpenMP()`, donde `omplexer.ll` tokeniza la directiva y `ompparser.yy` interpreta sus cláusulas.

El resto del código C pasa por el writer. Para cada token normal, el scanner pasa su texto a `writer.cc` con `writer_set_token_text()` y después llama a `writer_process_current_token()`. El writer mantiene en construcción la línea actual. Cuando llega un salto de línea, cierra esa línea y consulta el estado activo del traductor para decidir si se copia sin cambios, se bufferiza para una transformación pendiente, se reescribe o se captura para tasking.

El parser de pragmas guarda los argumentos de la cláusula activa en `pragma_args.h/.c`. El estado que debe durar más que una cláusula, como la información de `cluster`, `distribute`, `master`, `halo` o `num_teams`, se mantiene en `translator_state.h/.c` y `cluster_stack.h/.c`. Ese estado conecta la directiva parseada con los módulos transform que generan código MPI.

Los módulos transform generan los fragmentos MPI concretos: `mpi_lifecycle` emite inicialización/finalización y guardas por proceso, `distribute_transform` reescribe límites de bucles y genera el reparto de trabajo, `scatter_gather_transform`, `reduction_transform`, `memory_transform` y `mpi_type_transform` generan movimiento de datos, reducciones, reserva/broadcast y declaraciones de tipos. `halo_transform` se conecta con los bucles distribuidos: guarda los límites del bucle cuando se reescribe y, si aparece un `update halo`, emite los `MPI_Sendrecv` con los vecinos al cerrar la región.

Tasking sigue la misma pasada principal, pero no copia directamente los cuerpos de tarea. `tasking_region.h/.c` y `task_async_block.h/.c` registran la región y sus bloques asíncronos, mientras que `task_body_transform.h/.c` construye una representación pequeña de cada cuerpo. Después, `tasking_emit.h/.cc` emite las funciones de tarea generadas y `ompd_execute_generated_task` mediante `output_slots.h/.cc`, y deja en el punto original de `task_async` el código que crea la tarea, registra sus dependencias y la envía a `ompd_runtime.h/.c`.

Al terminar el parse, `main.cc` vacía el writer y aplica los slots diferidos. Ese último paso inserta el código que se descubrió durante el recorrido pero debe aparecer antes en el fichero generado, como cabeceras, declaraciones globales o definiciones generadas de tareas.

## Directivas soportadas

El repositorio contiene tres grupos principales de construcciones traducidas:

- **Distribución de datos y trabajo.** `cluster`, `distribute`, `scatter`, `gather`, `allgather`, reducciones, broadcasts, reserva de memoria y declaraciones de tipos MPI se traducen directamente a fragmentos MPI desde los módulos transform.
- **Intercambio de halo.** Una cláusula `halo(...)` declarada en un `cluster` marca el array que necesita comunicación de bordes fantasma. Después, `cluster distribute update halo(...)` usa los límites del último bucle distribuido y emite intercambios con los vecinos para las filas superior e inferior del bloque local.
- **Tasking.** `task_async`, `taskwait`, `taskgroup` y `taskyield` no se reducen a una secuencia MPI fija. El traductor captura cuerpos de tarea, genera funciones e inputs auxiliares, y delega en `ompd_runtime.c` la creación, planificación y sincronización de tareas entre procesos MPI.

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
| `ompd_runtime.h/.c` | Runtime de tasking que se compila *dentro* del programa generado (creación de tareas, dependencias, planificación, sincronización). |
| `hash_map.h/.c`, `task_utils.h/.c` | Estructuras y utilidades auxiliares de tasking. |

### Ficheros generados (no editar a mano)

`ompparser.cc`, `ompparser.hh`, `omplexer.cc`, `preproparser.cc`, `preproparser.hh`, `preprolexer.cc`, `y.tab.cc`, `y.tab.hh`, `lex.yy.cc`.

Los produce `bison`/`flex` a partir de los `.yy`, `.yacc`, `.ll` y `.lex`. Para cambiar el comportamiento de un parser o scanner se edita su fuente, no el fichero generado; `make` los vuelve a generar.

## Tests de regresión

Las pruebas de regresión están en `OpenMpD/tests/regression/`. Son pruebas de integración: para cada caso, el runner compila y ejecuta una versión secuencial y la versión OpenMPD del mismo programa, extrae el resultado calculado y compara ambos. No comprueba mensajes de éxito, sino el resultado numérico.

Hay que construir `fparse` antes y ejecutar las pruebas desde el directorio `OpenMpD/`. `cases.tsv` lista los ejemplos y `run_regression.py` compila, ejecuta y compara cada fila. Los ficheros generados y logs se escriben fuera del repositorio por defecto, bajo `WORK_ROOT`.

Ejecución normal:

```sh
python3 tests/regression/run_regression.py
```

Ejecución completa, incluyendo los casos lentos:

```sh
RUN_SLOW=1 python3 tests/regression/run_regression.py
```

Ejecutar una familia concreta:

```sh
FILTER_FAMILY=halo python3 tests/regression/run_regression.py
```

Resultados esperados con el árbol sano:

```text
normal:   summary: pass=15 skip=9 fail=0
completa: summary: pass=24 skip=0 fail=0
```

Para añadir un caso se agrega una fila a `cases.tsv`. Su cabecera documenta las columnas, los placeholders de rutas y los perfiles de comparación.

## Continuar el proyecto

El repositorio contiene actualmente un traductor *source-to-source* funcional para programas C anotados con pragmas OpenMPD. Cubre el flujo principal de traducción: preprocesado y recogida de símbolos, parsing de pragmas, reconstrucción del fuente, generación del ciclo de vida MPI, bucles distribuidos, movimiento colectivo de datos, reducciones, gestión de memoria, declaraciones de tipos MPI, intercambio de halos y tasking asíncrono basado en runtime.

La forma recomendada de empezar es construir `fparse`, traducir un ejemplo pequeño y comparar el programa MPI generado con el fuente OpenMPD original. Después conviene leer la sección de arquitectura y seguir el recorrido de ejecución por `main.cc`, `C99-scanner.lex`, `C99-parser.yacc`, `omplexer.ll`, `ompparser.yy` y `writer.h/.cc`. Para una directiva concreta, el siguiente paso es el módulo transform correspondiente: por ejemplo `distribute_transform`, `halo_transform`, `scatter_gather_transform`, `reduction_transform`, `memory_transform`, `mpi_type_transform` o `task_body_transform`.

Para trabajo orientado al comportamiento, `tests/regression/cases.tsv` es el mapa práctico del proyecto: muestra qué ejemplos se ejercitan ahora, qué argumentos necesitan, qué casos requieren el runtime de tasking y qué perfil de resultado se usa para comparar.

Trabajo futuro:

- Añadir más ejemplos y casos de regresión con distintos tamaños de entrada, números de procesos y combinaciones de pragmas ya existentes.
- Completar más partes del modelo de tasking de OpenMP: añadir directivas y cláusulas pendientes, mejorar el tratamiento de dependencias y soportar cuerpos de tarea más complejos.
- Definir qué construcciones OpenMPD pueden combinarse con tasking. El parser actual mantiene los clusters con tasking en un camino separado y rechaza algunos pragmas no-task al mismo nivel del cluster.
- Ampliar el soporte de halo más allá de repartos por filas, incluyendo distribuciones bidimensionales, columnas y descomposiciones por bloques.
- Mejorar la gestión de memoria del traductor. Valgrind sigue mostrando memoria sin liberar al terminar el proceso, sobre todo alrededor de los objetos `symbol_info` de la tabla de símbolos; el código necesita una regla más clara sobre qué parte crea esos objetos y qué parte debe liberarlos.
- Estudiar una refactorización mayor hacia una representación intermedia en vez de transformar el fuente línea a línea durante el parseo. Es un cambio arquitectónico costoso, pero se acerca más a cómo los compiladores modernos separan el análisis de la generación de código, y podría hacer más robusta la traducción de cuerpos de tarea, las comprobaciones semánticas y la generación de MPI.
