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
)

# fparse commands
declare -A FPARSE_CMD

# Compilaiton commands
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
FPARSE_CMD["pi_test1.c"]="./fparse pi_test1.c log.txt error.txt pi_test1_output.c"
COMP_SEQ["pi_test1.c"]="gcc pi_seq.c -o out"
RUN_SEQ["pi_test1.c"]="./out"
COMP_OMP["pi_test1.c"]="gcc -fopenmp pi_omp.c -o out"
RUN_OMP["pi_test1.c"]="./out"
COMP_MPI["pi_test1.c"]="mpicc pi_test1_output.c -o out"
RUN_MPI["pi_test1.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["pi_test1.c"]="mpicc -fopenmp pi_test1_output.c -o out"
RUN_MPO["pi_test1.c"]="mpirun -np {MPI_NP} ./out"

# pi_ompd.c
FPARSE_CMD["pi_ompd.c"]="./fparse pi_ompd.c log.txt error.txt pi_ompd_output.c"
COMP_SEQ["pi_ompd.c"]="gcc pi_seq.c -o out"
RUN_SEQ["pi_ompd.c"]="./out"
COMP_OMP["pi_ompd.c"]="gcc -fopenmp pi_omp.c -o out"
RUN_OMP["pi_ompd.c"]="./out"
COMP_MPI["pi_ompd.c"]="mpicc pi_ompd_output.c -o out"
RUN_MPI["pi_ompd.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["pi_ompd.c"]="mpicc -fopenmp pi_ompd_output.c -o out"
RUN_MPO["pi_ompd.c"]="mpirun -np {MPI_NP} ./out"

# prueba_julia.c
FPARSE_CMD["prueba_julia.c"]="./fparse prueba_julia.c log.txt error.txt prueba_julia_output.c"
COMP_SEQ["prueba_julia.c"]="gcc JuliaSEQ.c -o out -lm"
RUN_SEQ["prueba_julia.c"]="./out 1000 -0.85 0.0 2.0 200"
COMP_OMP["prueba_julia.c"]="gcc -fopenmp JuliaOMP.c -o out -lm"
RUN_OMP["prueba_julia.c"]="./out"
COMP_MPI["prueba_julia.c"]="mpicc prueba_julia_output.c -o out -lm"
RUN_MPI["prueba_julia.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"
COMP_MPO["prueba_julia.c"]="mpicc -fopenmp prueba_julia_output.c -o out -lm"
RUN_MPO["prueba_julia.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"

# JuliaOMPD.c
FPARSE_CMD["JuliaOMPD.c"]="./fparse JuliaOMPD.c log.txt error.txt JuliaOMPD_output.c"
COMP_SEQ["JuliaOMPD.c"]="gcc JuliaSEQ.c -o out -lm"
RUN_SEQ["JuliaOMPD.c"]="./out 1000 -0.85 0.0 2.0 200"
COMP_OMP["JuliaOMPD.c"]="gcc -fopenmp JuliaOMP.c -o out -lm"
RUN_OMP["JuliaOMPD.c"]="./out 1000 -0.85 0.0 2.0 200"
COMP_MPI["JuliaOMPD.c"]="mpicc JuliaOMPD_output.c -o out -lm"
RUN_MPI["JuliaOMPD.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"
COMP_MPO["JuliaOMPD.c"]="mpicc -fopenmp JuliaOMPD_output.c -o out -lm"
RUN_MPO["JuliaOMPD.c"]="mpirun -np {MPI_NP} ./out 1000 -0.85 0.0 2.0 200"

# prueba_mmat.c (ijk)
FPARSE_CMD["prueba_mmat.c (ijk)"]="./fparse prueba_mmat.c log.txt error.txt prueba_mmat_output.c"
COMP_SEQ["prueba_mmat.c (ijk)"]="gcc mmat_ijk.c -o out"
RUN_SEQ["prueba_mmat.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_OMP["prueba_mmat.c (ijk)"]="gcc -fopenmp mmat_ijk_omp.c -o out"
RUN_OMP["prueba_mmat.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_MPI["prueba_mmat.c (ijk)"]="mpicc prueba_mmat_output.c -o out"
RUN_MPI["prueba_mmat.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["prueba_mmat.c (ijk)"]="mpicc -fopenmp prueba_mmat_output.c -o out"
RUN_MPO["prueba_mmat.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# prueba_mmat.c (ikj)
FPARSE_CMD["prueba_mmat.c (ikj)"]="./fparse prueba_mmat.c log.txt error.txt prueba_mmat_output.c"
COMP_SEQ["prueba_mmat.c (ikj)"]="gcc mmat_ikj.c -o out"
RUN_SEQ["prueba_mmat.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_OMP["prueba_mmat.c (ikj)"]="gcc -fopenmp mmat_ikj_omp.c -o out"
RUN_OMP["prueba_mmat.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_MPI["prueba_mmat.c (ikj)"]="mpicc prueba_mmat_output.c -o out"
RUN_MPI["prueba_mmat.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["prueba_mmat.c (ikj)"]="mpicc -fopenmp prueba_mmat_output.c -o out"
RUN_MPO["prueba_mmat.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# mat_ompd.c (ijk)
FPARSE_CMD["mat_ompd.c (ijk)"]="./fparse mat_ompd.c log.txt error.txt mat_ompd_output.c"
COMP_SEQ["mat_ompd.c (ijk)"]="gcc mmat_ijk.c -o out"
RUN_SEQ["mat_ompd.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_OMP["mat_ompd.c (ijk)"]="gcc -fopenmp mmat_ijk_omp.c -o out"
RUN_OMP["mat_ompd.c (ijk)"]="./out 1536 1536 1536 1536"
COMP_MPI["mat_ompd.c (ijk)"]="mpicc mat_ompd_output.c -o out"
RUN_MPI["mat_ompd.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["mat_ompd.c (ijk)"]="mpicc -fopenmp mat_ompd_output.c -o out"
RUN_MPO["mat_ompd.c (ijk)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# mat_ompd.c (ikj)
FPARSE_CMD["mat_ompd.c (ikj)"]="./fparse mat_ompd.c log.txt error.txt mat_ompd_output.c"
COMP_SEQ["mat_ompd.c (ikj)"]="gcc mmat_ikj.c -o out"
RUN_SEQ["mat_ompd.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_OMP["mat_ompd.c (ikj)"]="gcc -fopenmp mmat_ikj_omp.c -o out"
RUN_OMP["mat_ompd.c (ikj)"]="./out 1536 1536 1536 1536"
COMP_MPI["mat_ompd.c (ikj)"]="mpicc mat_ompd_output.c -o out"
RUN_MPI["mat_ompd.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"
COMP_MPO["mat_ompd.c (ikj)"]="mpicc -fopenmp mat_ompd_output.c -o out"
RUN_MPO["mat_ompd.c (ikj)"]="mpirun -np {MPI_NP} ./out 1536 1536 1536 1536"

# prueba_mandel.c
FPARSE_CMD["prueba_mandel.c"]="./fparse prueba_mandel.c log.txt error.txt prueba_mandel_output.c"
COMP_SEQ["prueba_mandel.c"]="gcc Mandelbrot-SEQ.c -o out -lm"
RUN_SEQ["prueba_mandel.c"]="./out"
COMP_OMP["prueba_mandel.c"]="gcc -fopenmp Mandelbrot-OMP.c -o out -lm"
RUN_OMP["prueba_mandel.c"]="./out"
COMP_MPI["prueba_mandel.c"]="mpicc prueba_mandel_output.c -o out -lm"
RUN_MPI["prueba_mandel.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["prueba_mandel.c"]="mpicc -fopenmp prueba_mandel_output.c -o out -lm"
RUN_MPO["prueba_mandel.c"]="mpirun -np {MPI_NP} ./out"

# Mandelbrot-OMPD.c
FPARSE_CMD["Mandelbrot-OMPD.c"]="./fparse Mandelbrot-OMPD.c log.txt error.txt Mandelbrot-OMPD_output.c"
COMP_SEQ["Mandelbrot-OMPD.c"]="gcc Mandelbrot-SEQ.c -o out -lm"
RUN_SEQ["Mandelbrot-OMPD.c"]="./out"
COMP_OMP["Mandelbrot-OMPD.c"]="gcc -fopenmp Mandelbrot-OMP.c -o out -lm"
RUN_OMP["Mandelbrot-OMPD.c"]="./out"
COMP_MPI["Mandelbrot-OMPD.c"]="mpicc Mandelbrot-OMPD_output.c -o out -lm"
RUN_MPI["Mandelbrot-OMPD.c"]="mpirun -np {MPI_NP} ./out"
COMP_MPO["Mandelbrot-OMPD.c"]="mpicc -fopenmp Mandelbrot-OMPD_output.c -o out -lm"
RUN_MPO["Mandelbrot-OMPD.c"]="mpirun -np {MPI_NP} ./out"

# mdOMPD.c
FPARSE_CMD["mdOMPD.c"]="./fparse mdOMPD.c log.txt error.txt mdOMPD_output.c"
COMP_SEQ["mdOMPD.c"]="gcc mdSEQ.c -o out -lm"
RUN_SEQ["mdOMPD.c"]="./out 3 800 700 0.01"
COMP_OMP["mdOMPD.c"]="gcc -fopenmp mdOMP.c -o out -lm"
RUN_OMP["mdOMPD.c"]="./out 3 800 700 0.01"
COMP_MPI["mdOMPD.c"]="mpicc mdOMPD_output.c -o out -lm"
RUN_MPI["mdOMPD.c"]="mpirun -np {MPI_NP} ./out 3 800 700 0.01"
COMP_MPO["mdOMPD.c"]="mpicc -fopenmp mdOMPD_output.c -o out -lm"
RUN_MPO["mdOMPD.c"]="mpirun -np {MPI_NP} ./out 3 800 700 0.01"

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

main_menu(){
    echo
    echo "---------------------------------------"
    echo "Current program: $PROGRAM"
    echo "MPI ranks (-np): $MPI_NP | OpenMP threads: $OMP_THREADS"
    echo "---------------------------------------"
    echo "Choose action:"
    echo

    select opt in \
        "Run fparse" \
        "Run Sequential" \
        "Run OpenMP" \
        "Run MPI" \
        "Run MPI+OpenMP" \
        "Set MPI -np" \
        "Set OMP threads" \
        "Clean *_output.c" \
        "Change program" \
        "Exit"; do

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
            "Clean *_output.c")
                echo
                echo "Removing *_output.c"
                rm *_output.c
                echo "Clean done"
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
