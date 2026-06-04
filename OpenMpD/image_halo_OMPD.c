#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>

#define FILTER_GAUSSIAN 0
#define FILTER_SOBEL 1

// Kernel Gaussiano 3x3
float gaussian3x3[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
};

// Kernel Gaussiano 5x5
float gaussian5x5[5][5] = {
    { 1,  4,  6,  4, 1},
    { 4, 16, 24, 16, 4},
    { 6, 24, 36, 24, 6},
    { 4, 16, 24, 16, 4},
    { 1,  4,  6,  4, 1}
};

// Kernel Sobel 3x3 en X
float sobel3x3_x[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

// Kernel Sobel 3x3 en Y
float sobel3x3_y[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

// Kernel Sobel 5x5 en X
float sobel5x5_x[5][5] = {
    {-1, -2,  0,  2,  1},
    {-4, -8,  0,  8,  4},
    {-6,-12,  0, 12,  6},
    {-4, -8,  0,  8,  4},
    {-1, -2,  0,  2,  1}
};

// Kernel Sobel 5x5 en Y
float sobel5x5_y[5][5] = {
    {-1, -4,  -6, -4, -1},
    {-2, -8, -12, -8, -2},
    { 0,  0,   0,  0,  0},
    { 2,  8,  12,  8,  2},
    { 1,  4,   6,  4,  1}
};

// Función para clamping de enteros
int clamp_int(int x, int min, int max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

// Función para saltar comentarios en el archivo PGM
void skip_comments(FILE *f) {
    int c;
    while ((c = fgetc(f)) == '#') {
        while ((c = fgetc(f)) != '\n' && c != EOF) {
        }
    }
    if (c != EOF) {
        ungetc(c, f);
    }
}

// Función para leer el header de un archivo PGM
void read_pgm_header(const char *filename, int *height, int *width) {
    FILE *f = fopen(filename, "rb");
    // Validación de apertura del archivo
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char magic[3];
    int maxval;

    // Lectura del magic number
    if (fscanf(f, "%2s", magic) != 1) {
        fprintf(stderr, "Error leyendo magic number\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // Validación del formato PGM binario
    if (strcmp(magic, "P5") != 0) {
        fprintf(stderr, "Solo se soporta PGM binario (P5)\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // Lectura de width, height y maxval, saltando comentarios
    fgetc(f);
    skip_comments(f);

    if (fscanf(f, "%d %d", width, height) != 2) {
        fprintf(stderr, "Error leyendo width/height\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fgetc(f);
    skip_comments(f);


    // Lectura de maxval
    if (fscanf(f, "%d", &maxval) != 1) {
        fprintf(stderr, "Error leyendo maxval\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    if (maxval != 255) {
        fprintf(stderr, "Solo se soporta maxval = 255\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // El header se ha leído correctamente, el puntero del archivo ahora apunta al inicio de los datos de píxeles
    fclose(f);
}

// Función para leer los datos de un archivo PGM
void read_pgm_data(const char *filename, int height, int width, float img[height][width]) {
    FILE *f = fopen(filename, "rb");
    // Validación de apertura del archivo
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Saltar el header del archivo PGM
    char magic[3];
    int maxval;
    int tmp_w, tmp_h;

    fscanf(f, "%2s", magic);
    fgetc(f);
    skip_comments(f);
    fscanf(f, "%d %d", &tmp_w, &tmp_h);
    fgetc(f);
    skip_comments(f);
    fscanf(f, "%d", &maxval);
    fgetc(f);

    // Lectura de los datos de píxeles
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            unsigned char pixel;
            if (fread(&pixel, sizeof(unsigned char), 1, f) != 1) {
                fprintf(stderr, "Error leyendo píxeles\n");
                fclose(f);
                exit(EXIT_FAILURE);
            }
            img[i][j] = (float)pixel;
        }
    }

    fclose(f);
}

// Función para escribir un archivo PGM
void write_pgm(const char *filename, int height, int width, float img[height][width]) {
    FILE *f = fopen(filename, "wb");
    // Validación de apertura del archivo
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Escribir el header del archivo PGM
    fprintf(f, "P5\n%d %d\n255\n", width, height);

    // Escribir los datos de píxeles, aplicando clamping y redondeo
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int v = (int)(img[i][j] + 0.5);
            v = clamp_int(v, 0, 255);
            unsigned char pixel = (unsigned char)v;
            fwrite(&pixel, sizeof(unsigned char), 1, f);
        }
    }

    fclose(f);
}

int main(int argc, char *argv[]) 
{
    int height, width;
    int filter_dimension, halo_rows, iterations;
    #ifdef _OPENMP
        double start_time, run_time;
    #else
        struct timeval tv_start, tv_end;
        double run_time;
    #endif
    int filter_type;
    MPI_Request reqs;
    MPI_Status status;
    
    // Validación de número de argumentos
    if (argc != 5 && argc != 6) {
        fprintf(stderr, "Uso:\n  %s <filter_type> <filter_dimension> <input.pgm> <output.pgm> [iterations]\n",argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "gaussian") == 0 || strcmp(argv[1], "g") == 0) {
        filter_type = FILTER_GAUSSIAN;
    } else if (strcmp(argv[1], "sobel") == 0 || strcmp(argv[1], "s") == 0) {
        filter_type = FILTER_SOBEL;
    } else {
        fprintf(stderr, "Filtro no válido. Usa gaussian/g o sobel/s\n");
        exit(EXIT_FAILURE);
    }

    filter_dimension = atoi(argv[2]);
    iterations = 1;
    if (argc == 6) {
        iterations = atoi(argv[5]);
    }
    halo_rows = filter_dimension / 2;

    #pragma omp cluster broad(filter_dimension,halo_rows,filter_type,iterations)

    // Validación de dimensión de filtro
    if (filter_dimension != 3 && filter_dimension != 5) {
        fprintf(stderr, "Solo se permiten filtros 3x3 o 5x5\n");
        exit(EXIT_FAILURE);
    }
    if (iterations < 1) {
        fprintf(stderr, "iterations debe ser mayor o igual que 1\n");
        exit(EXIT_FAILURE);
    }

    const char *input_file = argv[3];
    const char *output_file = argv[4];

    // Lectura del header del archivo PGM
    // Esto valida que el archivo sea un PGM válido y obtiene las dimensiones de la imagen
    read_pgm_header(input_file, &height, &width);

    #pragma omp cluster broad(height, width)

    // Reserva en pila
    float img[height][width];
    float filtered[height][width];

    // Lectura de los datos de la imagen
    read_pgm_data(input_file, height, width, img);

    // Calcular norm si es gaussian
    float norm = 0.0f;
    if (filter_type==FILTER_GAUSSIAN) {
        if (filter_dimension == 3) {
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    norm += gaussian3x3[i][j];
                }
            }
        } else if (filter_dimension == 5) {
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 5; j++) {
                    norm += gaussian5x5[i][j];
                }
            }
        }
    }

    // Medición del tiempo de ejecución
    #ifdef _OPENMP
        start_time = omp_get_wtime();
    #else
        gettimeofday(&tv_start, NULL);
    #endif

    #pragma omp cluster broad(img[height][width]) broad(norm) gather(img[height][width]) halo(img[height][width]:halo_rows*width)
    {
        // Aplicación del filtro según filter_type
        for (int iter = 0; iter < iterations; iter++) {
            #pragma omp cluster distribute
            #pragma omp parallel for
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    // Bordes: se copia el píxel original
                    if (i < halo_rows || i >= height - halo_rows ||
                        j < halo_rows || j >= width  - halo_rows) {
                        filtered[i][j] = img[i][j];
                    } else {
                        // FILTRO GAUSSIANO
                        if (filter_type==FILTER_GAUSSIAN) {
                            float sum = 0.0f;

                            for (int ki = -halo_rows; ki <= halo_rows; ki++) {
                                for (int kj = -halo_rows; kj <= halo_rows; kj++) {
                                    if (filter_dimension == 3) {
                                        sum += img[i + ki][j + kj] *
                                            gaussian3x3[ki + 1][kj + 1];
                                    } else { // 5x5
                                        sum += img[i + ki][j + kj] *
                                            gaussian5x5[ki + 2][kj + 2];
                                    }
                                }
                            }

                            filtered[i][j] = sum / norm;
                        }
                        // FILTRO SOBEL
                        if (filter_type==FILTER_SOBEL) {
                            float gx = 0.0f;
                            float gy = 0.0f;

                            for (int ki = -halo_rows; ki <= halo_rows; ki++) {
                                for (int kj = -halo_rows; kj <= halo_rows; kj++) {

                                    float pixel = img[i + ki][j + kj];

                                    if (filter_dimension == 3) {
                                        gx += pixel * sobel3x3_x[ki + 1][kj + 1];
                                        gy += pixel * sobel3x3_y[ki + 1][kj + 1];
                                    } else { // 5x5
                                        gx += pixel * sobel5x5_x[ki + 2][kj + 2];
                                        gy += pixel * sobel5x5_y[ki + 2][kj + 2];
                                    }
                                }
                            }

                            // Magnitud del gradiente Sobel
                            float mag = sqrtf(gx * gx + gy * gy);
                            if (mag > 255.0f) mag = 255.0f;
                            if (mag < 0.0f)   mag = 0.0f;
                            filtered[i][j] = mag;
                        }
                    }
                }
            }

            #pragma omp cluster distribute update halo(img[height][width]:halo_rows*width)
            #pragma omp parallel for
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j++) {
                    img[i][j] = filtered[i][j];
                }
            }
        }
    }

    // Medición del tiempo de ejecución
    #ifdef _OPENMP
        run_time = omp_get_wtime() - start_time;
    #else
        gettimeofday(&tv_end, NULL);
        run_time = (tv_end.tv_sec - tv_start.tv_sec) * 1000000
                + (tv_end.tv_usec - tv_start.tv_usec);
        run_time = run_time / 1000000.0;
    #endif

    printf("Imagen leida: %dx%d\n", width, height);
    printf("Filtro %s %dx%d completado en %f s\n",
        filter_type==FILTER_GAUSSIAN?"gaussian":"sobel",
        filter_dimension,
        filter_dimension,
        run_time);

    write_pgm(output_file, height, width, img);

    // Sugerencia para visualizar la imagen resultante
    char out_png[256];
    // Copia el nombre del archivo de salida sin la extensión .pgm
    strncpy(out_png, output_file, sizeof(out_png) - 1);
    out_png[sizeof(out_png) - 1] = '\0';

    char *dot;
    dot= strrchr(out_png, '.');
    if (dot != NULL && strcmp(dot, ".pgm") == 0) {
        *dot = '\0';
    }

    printf("Para visualizar:\n");
    printf("convert %s %s.png\n", output_file, out_png);

    return 0;
}
