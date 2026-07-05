#!/bin/bash

# Program list
PROGRAMS=(
    "pi_test1.c"
    "pi_ompd.c"
    "prueba_julia.c"
    "JuliaOMPD.c"
    "prueba_mmat.c (ijk)"
    "prueba_mmat.c (ikj)"
    "mat_ompd.c (ijk)"
    "mat_ompd.c (ikj)"
    "prueba_mandel.c"
    "Mandelbrot-OMPD.c"
    "mdOMPD.c"
    "fibonacci-ompd.c"
    "binomial_task_ompd.c"
    "pi_task_ompd.c"
    "simpson_task_ompd.c"
    "pipeline_ompd.c"
)

# fparse commands
declare -A FPARSE_CMD

# Compilation commands
declare -A COMP_SEQ
declare -A COMP_OMP
declare -A COMP_MPI
declare -A COMP_MPO

# Execution commands
declare -A RUN_SEQ
declare -A RUN_OMP
declare -A RUN_MPI
declare -A RUN_MPO

# Default settings
MPI_NP=4
OMP_THREADS=4

# pi_test1.c
FPARSE_CMD["pi_test1.c"]="./fparse ../examples/pi/pi_test1.c log.txt error.txt pi_test1_output.c"
COMP_SEQ["pi_test1.c"]="gcc -O2 ../examples/pi/pi_seq.c -o out"
RUN_SEQ["pi_test1.c"]="./out"
COMP_OMP["pi_test1.c"]="gcc -O2 -fopenmp ../examples/pi/pi_omp.c -o out"
RUN_OMP["pi_test1.c"]="./out"
COMP_MPI["pi_test1.c"]="mpicc -O2 pi_test1_output.c -o out"
RUN_MPI["pi_test1.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["pi_test1.c"]="mpicc -O2 -fopenmp pi_test1_output.c -o out"
RUN_MPO["pi_test1.c"]="mpirun -np {MPI_NP} ./out"

# pi_ompd.c
FPARSE_CMD["pi_ompd.c"]="./fparse ../examples/pi/pi_ompd.c log.txt error.txt pi_ompd_output.c"
COMP_SEQ["pi_ompd.c"]="gcc -O2 ../examples/pi/pi_seq.c -o out"
RUN_SEQ["pi_ompd.c"]="./out"
COMP_OMP["pi_ompd.c"]="gcc -O2 -fopenmp ../examples/pi/pi_omp.c -o out"
RUN_OMP["pi_ompd.c"]="./out"
COMP_MPI["pi_ompd.c"]="mpicc -O2 pi_ompd_output.c -o out"
RUN_MPI["pi_ompd.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["pi_ompd.c"]="mpicc -O2 -fopenmp pi_ompd_output.c -o out"
RUN_MPO["pi_ompd.c"]="mpirun -np {MPI_NP} ./out"

# prueba_julia.c
FPARSE_CMD["prueba_julia.c"]="./fparse ../examples/julia/prueba_julia.c log.txt error.txt prueba_julia_output.c"
COMP_SEQ["prueba_julia.c"]="gcc -O2 ../examples/julia/JuliaSEQ.c -o out -lm"
RUN_SEQ["prueba_julia.c"]="./out 1000 -0.85 0.0 2.0 200"
COMP_OMP["prueba_julia.c"]="gcc -O2 -fopenmp ../examples/julia/JuliaOMP.c -o out -lm"
RUN_OMP["prueba_julia.c"]="./out"
COMP_MPI["prueba_julia.c"]="mpicc -O2 prueba_julia_output.c -o out -lm"
RUN_MPI["prueba_julia.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"
COMP_MPO["prueba_julia.c"]="mpicc -O2 -fopenmp prueba_julia_output.c -o out -lm"
RUN_MPO["prueba_julia.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"

# JuliaOMPD.c
FPARSE_CMD["JuliaOMPD.c"]="./fparse ../examples/julia/JuliaOMPD.c log.txt error.txt JuliaOMPD_output.c"
COMP_SEQ["JuliaOMPD.c"]="gcc -O2 ../examples/julia/JuliaSEQ.c -o out -lm"
RUN_SEQ["JuliaOMPD.c"]="./out 1000 -0.85 0.0 2.0 200"
COMP_OMP["JuliaOMPD.c"]="gcc -O2 -fopenmp ../examples/julia/JuliaOMP.c -o out -lm"
RUN_OMP["JuliaOMPD.c"]="./out 1000 -0.85 0.0 2.0 200"
COMP_MPI["JuliaOMPD.c"]="mpicc -O2 JuliaOMPD_output.c -o out -lm"
RUN_MPI["JuliaOMPD.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"
COMP_MPO["JuliaOMPD.c"]="mpicc -O2 -fopenmp JuliaOMPD_output.c -o out -lm"
RUN_MPO["JuliaOMPD.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"

# prueba_mmat.c (ijk)
FPARSE_CMD["prueba_mmat.c (ijk)"]="./fparse ../examples/mat_ompd/prueba_mmat.c log.txt error.txt prueba_mmat_output.c"
COMP_SEQ["prueba_mmat.c (ijk)"]="gcc -O2 ../examples/mmat/IJK/mmat_ijk.c -o out"
RUN_SEQ["prueba_mmat.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_OMP["prueba_mmat.c (ijk)"]="gcc -O2 -fopenmp ../examples/mmat/IJK/mmat_ijk_omp.c -o out"
RUN_OMP["prueba_mmat.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_MPI["prueba_mmat.c (ijk)"]="mpicc -O2 prueba_mmat_output.c -o out"
RUN_MPI["prueba_mmat.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["prueba_mmat.c (ijk)"]="mpicc -O2 -fopenmp prueba_mmat_output.c -o out"
RUN_MPO["prueba_mmat.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# prueba_mmat.c (ikj)
FPARSE_CMD["prueba_mmat.c (ikj)"]="./fparse ../examples/mat_ompd/prueba_mmat.c log.txt error.txt prueba_mmat_output.c"
COMP_SEQ["prueba_mmat.c (ikj)"]="gcc -O2 ../examples/mmat/IKJ/mmat_ikj.c -o out"
RUN_SEQ["prueba_mmat.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_OMP["prueba_mmat.c (ikj)"]="gcc -O2 -fopenmp ../examples/mmat/IKJ/mmat_ikj_omp.c -o out"
RUN_OMP["prueba_mmat.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_MPI["prueba_mmat.c (ikj)"]="mpicc -O2 prueba_mmat_output.c -o out"
RUN_MPI["prueba_mmat.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["prueba_mmat.c (ikj)"]="mpicc -O2 -fopenmp prueba_mmat_output.c -o out"
RUN_MPO["prueba_mmat.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# mat_ompd.c (ijk)
FPARSE_CMD["mat_ompd.c (ijk)"]="./fparse ../examples/mat_ompd/mat_ompd.c log.txt error.txt mat_ompd_output.c"
COMP_SEQ["mat_ompd.c (ijk)"]="gcc -O2 ../examples/mmat/IJK/mmat_ijk.c -o out"
RUN_SEQ["mat_ompd.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_OMP["mat_ompd.c (ijk)"]="gcc -O2 -fopenmp ../examples/mmat/IJK/mmat_ijk_omp.c -o out"
RUN_OMP["mat_ompd.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_MPI["mat_ompd.c (ijk)"]="mpicc -O2 mat_ompd_output.c -o out"
RUN_MPI["mat_ompd.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["mat_ompd.c (ijk)"]="mpicc -O2 -fopenmp mat_ompd_output.c -o out"
RUN_MPO["mat_ompd.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# mat_ompd.c (ikj)
FPARSE_CMD["mat_ompd.c (ikj)"]="./fparse ../examples/mat_ompd/mat_ompd.c log.txt error.txt mat_ompd_output.c"
COMP_SEQ["mat_ompd.c (ikj)"]="gcc -O2 ../examples/mmat/IKJ/mmat_ikj.c -o out"
RUN_SEQ["mat_ompd.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_OMP["mat_ompd.c (ikj)"]="gcc -O2 -fopenmp ../examples/mmat/IKJ/mmat_ikj_omp.c -o out"
RUN_OMP["mat_ompd.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_MPI["mat_ompd.c (ikj)"]="mpicc -O2 mat_ompd_output.c -o out"
RUN_MPI["mat_ompd.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["mat_ompd.c (ikj)"]="mpicc -O2 -fopenmp mat_ompd_output.c -o out"
RUN_MPO["mat_ompd.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# prueba_mandel.c
FPARSE_CMD["prueba_mandel.c"]="./fparse ../examples/mandelbrot/prueba_mandel.c log.txt error.txt prueba_mandel_output.c"
COMP_SEQ["prueba_mandel.c"]="gcc -O2 ../examples/mandelbrot/Mandelbrot-SEQ.c -o out -lm"
RUN_SEQ["prueba_mandel.c"]="./out"
COMP_OMP["prueba_mandel.c"]="gcc -O2 -fopenmp ../examples/mandelbrot/Mandelbrot-OMP.c -o out -lm"
RUN_OMP["prueba_mandel.c"]="./out"
COMP_MPI["prueba_mandel.c"]="mpicc -O2 prueba_mandel_output.c -o out -lm"
RUN_MPI["prueba_mandel.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["prueba_mandel.c"]="mpicc -O2 -fopenmp prueba_mandel_output.c -o out -lm"
RUN_MPO["prueba_mandel.c"]="mpirun -np {MPI_NP} ./out"

# Mandelbrot-OMPD.c
FPARSE_CMD["Mandelbrot-OMPD.c"]="./fparse ../examples/mandelbrot/Mandelbrot-OMPD.c log.txt error.txt Mandelbrot-OMPD_output.c"
COMP_SEQ["Mandelbrot-OMPD.c"]="gcc -O2 ../examples/mandelbrot/Mandelbrot-SEQ.c -o out -lm"
RUN_SEQ["Mandelbrot-OMPD.c"]="./out"
COMP_OMP["Mandelbrot-OMPD.c"]="gcc -O2 -fopenmp ../examples/mandelbrot/Mandelbrot-OMP.c -o out -lm"
RUN_OMP["Mandelbrot-OMPD.c"]="./out"
COMP_MPI["Mandelbrot-OMPD.c"]="mpicc -O2 Mandelbrot-OMPD_output.c -o out -lm"
RUN_MPI["Mandelbrot-OMPD.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["Mandelbrot-OMPD.c"]="mpicc -O2 -fopenmp Mandelbrot-OMPD_output.c -o out -lm"
RUN_MPO["Mandelbrot-OMPD.c"]="mpirun -np {MPI_NP} ./out"

# mdOMPD.c
FPARSE_CMD["mdOMPD.c"]="./fparse ../examples/MD/mdOMPD.c log.txt error.txt mdOMPD_output.c"
COMP_SEQ["mdOMPD.c"]="gcc -O2 ../examples/MD/mdSEQ.c -o out -lm"
RUN_SEQ["mdOMPD.c"]="./out 3 800 700 0.01"
COMP_OMP["mdOMPD.c"]="gcc -O2 -fopenmp ../examples/MD/mdOMP.c -o out -lm"
RUN_OMP["mdOMPD.c"]="./out 3 800 700 0.01"
COMP_MPI["mdOMPD.c"]="mpicc -O2 mdOMPD_output.c -o out -lm"
RUN_MPI["mdOMPD.c"]="mpirun -np {MPI_NP} ./out 3 800 700 0.01"
COMP_MPO["mdOMPD.c"]="mpicc -O2 -fopenmp mdOMPD_output.c -o out -lm"
RUN_MPO["mdOMPD.c"]="mpirun -np {MPI_NP} ./out 3 800 700 0.01"

# fibonacci-ompd.c
FPARSE_CMD["fibonacci-ompd.c"]="./fparse ../examples/tasks/fibonacci/fibonacci-ompd.c log.txt error.txt fibonacci-ompd_output.c"
COMP_SEQ["fibonacci-ompd.c"]="mpicc -O2 ../examples/tasks/fibonacci/fibonacci-seq.c -o out"
RUN_SEQ["fibonacci-ompd.c"]="./out 20 10"
COMP_OMP["fibonacci-ompd.c"]="gcc -O2 -fopenmp ../examples/tasks/fibonacci/fibonacci-omp.c -o out"
RUN_OMP["fibonacci-ompd.c"]="./out 20 10"
COMP_MPI["fibonacci-ompd.c"]="mpicc -O2 fibonacci-ompd_output.c -o out"
RUN_MPI["fibonacci-ompd.c"]="mpirun -np {MPI_NP} ./out 20 10"
COMP_MPO["fibonacci-ompd.c"]="mpicc -O2 -fopenmp fibonacci-ompd_output.c -o out"
RUN_MPO["fibonacci-ompd.c"]="mpirun -np {MPI_NP} ./out 20 10"

# binomial_task_ompd.c
FPARSE_CMD["binomial_task_ompd.c"]="./fparse ../examples/tasks/binomial/binomial_task_ompd.c log.txt error.txt binomial_task_ompd_output.c"
COMP_SEQ["binomial_task_ompd.c"]="mpicc -O2 ../examples/tasks/binomial/binomial_seq.c -o out"
RUN_SEQ["binomial_task_ompd.c"]="./out 20 10 10"
COMP_OMP["binomial_task_ompd.c"]="gcc -O2 -fopenmp ../examples/tasks/binomial/binomial_omp.c -o out"
RUN_OMP["binomial_task_ompd.c"]="./out 20 10 10"
COMP_MPI["binomial_task_ompd.c"]="mpicc -O2 binomial_task_ompd_output.c -o out"
RUN_MPI["binomial_task_ompd.c"]="mpirun -np {MPI_NP} ./out 20 10 10"
COMP_MPO["binomial_task_ompd.c"]="mpicc -O2 -fopenmp binomial_task_ompd_output.c -o out"
RUN_MPO["binomial_task_ompd.c"]="mpirun -np {MPI_NP} ./out 20 10 10"

# pi_task_ompd.c
FPARSE_CMD["pi_task_ompd.c"]="./fparse ../examples/tasks/pi_task/pi_task_ompd.c log.txt error.txt pi_task_ompd_output.c"
COMP_SEQ["pi_task_ompd.c"]="mpicc -O2 ../examples/tasks/pi_task/pi_task_seq.c -o out -lm"
RUN_SEQ["pi_task_ompd.c"]="./out 100000"
COMP_OMP["pi_task_ompd.c"]="gcc -O2 -fopenmp ../examples/tasks/pi_task/pi_task_omp.c -o out -lm"
RUN_OMP["pi_task_ompd.c"]="./out 100000"
COMP_MPI["pi_task_ompd.c"]="mpicc -O2 pi_task_ompd_output.c -o out -lm"
RUN_MPI["pi_task_ompd.c"]="mpirun -np {MPI_NP} ./out 100000"
COMP_MPO["pi_task_ompd.c"]="mpicc -O2 -fopenmp pi_task_ompd_output.c -o out -lm"
RUN_MPO["pi_task_ompd.c"]="mpirun -np {MPI_NP} ./out 100000"

# simpson_task_ompd.c
FPARSE_CMD["simpson_task_ompd.c"]="./fparse ../examples/tasks/simpson/simpson_task_ompd.c log.txt error.txt simpson_task_ompd_output.c"
COMP_SEQ["simpson_task_ompd.c"]="mpicc -O2 ../examples/tasks/simpson/simpson_seq.c -o out -lm"
RUN_SEQ["simpson_task_ompd.c"]="./out 1e-6 12 6"
COMP_OMP["simpson_task_ompd.c"]="gcc -O2 -fopenmp ../examples/tasks/simpson/simpson_omp.c -o out -lm"
RUN_OMP["simpson_task_ompd.c"]="./out 1e-6 12 6"
COMP_MPI["simpson_task_ompd.c"]="mpicc -O2 simpson_task_ompd_output.c -o out -lm"
RUN_MPI["simpson_task_ompd.c"]="mpirun -np {MPI_NP} ./out 1e-6 12 6"
COMP_MPO["simpson_task_ompd.c"]="mpicc -O2 -fopenmp simpson_task_ompd_output.c -o out -lm"
RUN_MPO["simpson_task_ompd.c"]="mpirun -np {MPI_NP} ./out 1e-6 12 6"

# pipeline_ompd.c
FPARSE_CMD["pipeline_ompd.c"]="./fparse ../examples/tasks/pipeline/pipeline_ompd.c log.txt error.txt pipeline_ompd_output.c"
COMP_SEQ["pipeline_ompd.c"]="mpicc -O2 ../examples/tasks/pipeline/pipeline_seq.c -o out"
RUN_SEQ["pipeline_ompd.c"]="./out"
COMP_OMP["pipeline_ompd.c"]="gcc -O2 -fopenmp ../examples/tasks/pipeline/pipeline_omp.c -o out"
RUN_OMP["pipeline_ompd.c"]="./out"
COMP_MPI["pipeline_ompd.c"]="mpicc -O2 pipeline_ompd_output.c -o out"
RUN_MPI["pipeline_ompd.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["pipeline_ompd.c"]="mpicc -O2 -fopenmp pipeline_ompd_output.c -o out"
RUN_MPO["pipeline_ompd.c"]="mpirun -np {MPI_NP} ./out"

# Functions
pause_enter(){
    echo
    echo "---------------------------------------"
    read -r -p "Press ENTER to continue..." _
    echo
}

render_run_cmd(){
    local cmd="$1"
    cmd="${cmd//\{MPI_NP\}/$MPI_NP}"
    echo "$cmd"
}

run_fparse(){
    local cmd="${FPARSE_CMD[$PROGRAM]}"
    if [[ -z "$cmd" ]]; then
        echo
        echo "No parse command defined for program: $PROGRAM"
        echo "Fill FPARSE_CMD[\"$PROGRAM\"] in the script"
        return
    fi

    echo
    echo "Running fparse"
    echo "$cmd"
    echo "---------------------------------------"
    echo
    eval "$cmd"
    echo
    echo "---------------------------------------"
    echo "Execution of fparse finished"
}

select_program(){
    echo
    echo "Select program:"
    select p in "${PROGRAMS[@]}" "Exit"; do
        if [[ "$p" == "Exit" ]]; then
            exit 0
        elif [[ -n "$p" ]]; then
            PROGRAM="$p"
            return
        else
            echo "Invalid option"
        fi
    done
}


configure_np(){
    echo
    read -r -p "Enter MPI ranks (-np) [current: $MPI_NP]: " val
    if [[ -z "${val// }" ]]; then
        echo "Keeping MPI_NP=$MPI_NP"
        return
    fi
    if [[ "$val" =~ ^[0-9]+$ ]] && ((val >= 1)); then
        MPI_NP="$val"
        echo "MPI_NP set to $MPI_NP"
    else
        echo "Invalid number. MPI_NP unchanges ($MPI_NP)"
    fi
}

configure_threads(){
    echo
    read -r -p "Enter OpenMP threads (OMP_NUM_THREADS) [current: $OMP_THREADS]: " val
    if [[ -z "${val// }" ]]; then
        echo "Keeping OMP_THREADS=$OMP_THREADS"
        return
    fi
    if [[ "$val" =~ ^[0-9]+$ ]] && ((val >= 1)); then
        OMP_THREADS="$val"
        echo "OMP_THREADS set to $OMP_THREADS"
    else
        echo "Invalid number. OMP_THREADS unchanges ($OMP_THREADS)"
    fi
}

run_program(){
    local comp="" run="" need_omp=0

    case "$MODE" in
    "Sequential")
        comp="${COMP_SEQ[$PROGRAM]}"
        run="${RUN_SEQ[$PROGRAM]}"
        need_omp=0
        ;;
    "OpenMP")
        comp="${COMP_OMP[$PROGRAM]}"
        run="${RUN_OMP[$PROGRAM]}"
        need_omp=1
        ;;
    "MPI")
        comp="${COMP_MPI[$PROGRAM]}"
        run="${RUN_MPI[$PROGRAM]}"
        need_omp=0
        ;;
    "MPI+OpenMP")
        comp="${COMP_MPO[$PROGRAM]}"
        run="${RUN_MPO[$PROGRAM]}"
        need_omp=1
        ;;
    *)
        echo "Invalid MODE: $MODE"
        return 1
        ;;
    esac

    if [[ -z "$comp" || -z "$run" ]]; then
        echo
        echo "Missing commands for $PROGRAM ($MODE)"
        echo "Fill COMP_* and RUN_* arrays for this program/mode"
        return
    fi

    if (( need_omp == 1)); then
        export OMP_NUM_THREADS="$OMP_THREADS"
        echo
        echo "Exporting: OMP_NUM_THREADS=$OMP_NUM_THREADS"
    fi

    echo
    echo "Compiling:"
    echo "$comp"
    echo "---------------------------------------"
    echo
    eval "$comp"
    echo
    echo "---------------------------------------"
    echo "Compilation finished"

    local run_rendered
    run_rendered="$(render_run_cmd "$run")"

    echo
    echo "Running:"
    echo "$run_rendered"
    echo "---------------------------------------"
    echo
    eval "$run_rendered"
    echo
    echo "---------------------------------------"
    echo "Execution finished"
}

clean_generated_files(){
    local files=(
        *_output.c
        *_output_reference.c
        output.c
        log.txt
        error.txt
        sym_tables.txt
        game_of_life_output_version_out.txt
        out
        out_*
        mandelbrot.ppm
        julia_set.tga
        g.pgm
        g.png
        s.pgm
        s.png
        lenna.pgm
        lenna.png
        original.bmp
        kind
    )

    echo
    echo "Removing generated files"
    rm -f -- "${files[@]}"
    rm -rf -- tests/regression/__pycache__
    echo "Clean done"
}

available_versions(){
    AVAILABLE_VERSIONS=()
    [[ -n "${COMP_SEQ[$PROGRAM]}" ]] && AVAILABLE_VERSIONS+=("Run Sequential")
    [[ -n "${COMP_OMP[$PROGRAM]}" ]] && AVAILABLE_VERSIONS+=("Run OpenMP")
    [[ -n "${COMP_MPI[$PROGRAM]}" ]] && AVAILABLE_VERSIONS+=("Run MPI")
    [[ -n "${COMP_MPO[$PROGRAM]}" ]] && AVAILABLE_VERSIONS+=("Run MPI+OpenMP")
}

main_menu(){
    available_versions

    echo
    echo "---------------------------------------"
    echo "Current program: $PROGRAM"
    echo "Available versions: ${AVAILABLE_VERSIONS[*]#Run }"
    echo "MPI ranks (-np): $MPI_NP | OpenMP threads: $OMP_THREADS"
    echo "---------------------------------------"
    echo "Choose action:"
    echo

    local options=(
        "Run fparse"
        "${AVAILABLE_VERSIONS[@]}"
        "Set MPI -np"
        "Set OMP threads"
        "Clean generated files"
        "Change program"
        "Exit"
    )

    select opt in "${options[@]}"; do
        case "$opt" in
            "Run fparse")
                run_fparse
                return
                ;;
            "Run Sequential")
                MODE="Sequential"
                run_program
                return
                ;;
            "Run OpenMP")
                MODE="OpenMP"
                run_program
                return
                ;;
            "Run MPI")
                MODE="MPI"
                run_program
                return
                ;;
            "Run MPI+OpenMP")
                MODE="MPI+OpenMP"
                run_program
                return
                ;;
            "Set MPI -np")
                configure_np
                return
                ;;
            "Set OMP threads")
                configure_threads
                return
                ;;
            "Clean generated files")
                clean_generated_files
                NO_PAUSE=1
                return
                ;;
            "Change program")
                select_program
                NO_PAUSE=1
                return
                ;;
            "Exit")
                exit 0
                ;;
            *)
                echo "Invalid option"
                ;;
            esac
        done
}

select_program

while true; do
    main_menu
    [[ "${NO_PAUSE:-0}" == "1" ]] || pause_enter
    NO_PAUSE=1
done
