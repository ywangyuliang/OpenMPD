#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <omp.h>

#define ALIVE 1
#define DEAD  0

int rows;
int cols;

void reset_directory(const char *dirname) {
    char command[256];

    snprintf(command, sizeof(command), "rm -rf %s", dirname);

    if (system(command) != 0) {
        fprintf(stderr, "Error ejecutando rm\n");
        exit(EXIT_FAILURE);
    }

    if (mkdir(dirname, 0700) != 0) {
        perror("mkdir");
        exit(EXIT_FAILURE);
    }
}

void initialize_random(int board[rows][cols], double alive_prob) {
    int i, j;

    for (i = 1; i < rows - 1; i++) {
        for (j = 1; j < cols - 1; j++) {
            double r = (double)rand() / (double)RAND_MAX;
            board[i][j] = (r < alive_prob) ? ALIVE : DEAD;
        }
    }
}

void initialize_glider(int board[rows][cols]) {
    int mid_i = rows / 2;
    int mid_j = cols / 2;

    if (rows < 6 || cols < 6) {
        return;
    }

    board[mid_i][mid_j + 1] = ALIVE;
    board[mid_i + 1][mid_j + 2] = ALIVE;
    board[mid_i + 2][mid_j]     = ALIVE;
    board[mid_i + 2][mid_j + 1] = ALIVE;
    board[mid_i + 2][mid_j + 2] = ALIVE;
}

void save_frame(const char *directory, int frame_id, int board[rows][cols]) {
    char filename[256];
    FILE *f;
    int i, j;

    snprintf(filename, sizeof(filename), "%s/frame_%04d.txt", directory, frame_id);

    f = fopen(filename, "w");
    if (f == NULL) {
        perror("fopen frame");
        exit(EXIT_FAILURE);
    }

    for (i = 1; i < rows - 1; i++) {
        for (j = 1; j < cols - 1; j++) {
            if (board[i][j] == ALIVE) {
                fprintf(f, "O ");
            } else {
                fprintf(f, ". ");
            }
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

void print_table(int rows, int cols, int board[rows][cols]) {
    int i, j;

    for (i = 1; i < rows - 1; i++) {
        for (j = 1; j < cols - 1; j++) {
            if (board[i][j] == ALIVE) {
                printf("O ");
            } else {
                printf(". ");
            }
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    struct timeval tv_start, tv_end;
    double run_time;

    int iterations;
    // show_Mode = 0 --> no se habilita impresión
    // show_Mode = 1 --> se imprime estado inicial y final
    // show_Mode = 2 --> se guarda el estado de cada iteracion en el directorio 'frames'
    int show_Mode;
    int iter;

    if (argc != 5) {
        fprintf(stderr,
                "Uso: mpirun [-np num_Proc] ./%s [rows] [cols] [iterations] [show_Mode]\n"
                "show_Mode = 0 --> no se habilita impresion\n"
                "show_Mode = 1 --> se imprime estado inicial y final\n"
                "show_Mode = 2 --> se guarda el estado de cada iteracion en el directorio 'frames'\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    rows = atoi(argv[1]);
    cols = atoi(argv[2]);
    iterations = atoi(argv[3]);
    show_Mode = atoi(argv[4]);

    if (rows <= 0) {
        fprintf(stderr, "Error: rows debe ser > 0\n");
        exit(EXIT_FAILURE);
    }
    if (cols <= 0) {
        fprintf(stderr, "Error: cols debe ser > 0\n");
        exit(EXIT_FAILURE);
    }
    if (iterations <= 0) {
        fprintf(stderr, "Error: iterations debe ser > 0\n");
        exit(EXIT_FAILURE);
    }
    if (show_Mode < 0 || show_Mode > 2) {
        fprintf(stderr, "Error: show_Mode debe ser 0, 1 o 2\n");
        exit(EXIT_FAILURE);
    }

    int current[rows][cols];
    int next[rows][cols];

    memset(current, 0, sizeof(current));
    memset(next, 0, sizeof(next));

    srand(42);

    if (show_Mode == 2) {
        reset_directory("frames");
    }

    // Elige una de estas inicializaciones
    initialize_glider(current);
    // initialize_random(current, 0.5);

    if (show_Mode == 1) {
        printf("Estado inicial:\n");
        print_table(rows, cols, current);
        printf("\n");
    }

    /*
    if (show_Mode == 2) {
        save_frame("frames", 0, current);
    }
    */

    // Copia inicial current -> next
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            next[i][j] = current[i][j];
        }
    }

    gettimeofday(&tv_start, NULL);

#pragma omp cluster broad(iterations) broad(next[rows][cols]) gather(next[rows][cols]) halo(next[rows][cols]:1*cols)
{
#pragma omp teams
    for (iter = 1; iter <= iterations; iter++) {
        int i, j;
        int neighbors;

        // Calcular next a partir de current
        #pragma omp cluster distribute update halo(next[rows][cols]:1*cols)
        #pragma omp parallel for private(j, neighbors)
        for (i = 1; i < rows - 1; i++) {
            for (j = 1; j < cols - 1; j++) {
                neighbors =
                    current[i - 1][j - 1] + current[i - 1][j] + current[i - 1][j + 1] +
                    current[i][j - 1]     +                     current[i][j + 1] +
                    current[i + 1][j - 1] + current[i + 1][j] + current[i + 1][j + 1];

                // Una célula viva muere si tiene menos de 2 o más de 3 vecinos vivos
                if (current[i][j] == ALIVE) {
                    if (neighbors < 2 || neighbors > 3) {
                        next[i][j] = DEAD;
                    } else {
                        next[i][j] = ALIVE;
                    }
                } else { // Una célula muerta se vuelve viva si tiene exactamente 3 vecinos vivos
                    if (neighbors == 3) {
                        next[i][j] = ALIVE;
                    } else {
                        next[i][j] = DEAD;
                    }
                }
            }
        }

        // Copia next -> current
        for (i = 1; i < rows - 1; i++) {
            for (j = 1; j < cols - 1; j++) {
                current[i][j] = next[i][j];
            }
        }

        /*
        #pragma omp cluster teams master
            if (show_Mode == 2) {
                save_frame("frames", iter, next);
            }
        */
    }
}

    gettimeofday(&tv_end, NULL);
    run_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000.0 +
               (tv_end.tv_usec - tv_start.tv_usec);
    run_time /= 1000000.0;

    printf("Execution time: %lg seconds\n", run_time);

    if (show_Mode == 1) {
        printf("Estado final:\n");
        print_table(rows, cols, next);
    }

    return 0;
}