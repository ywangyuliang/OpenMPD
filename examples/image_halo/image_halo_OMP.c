#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <omp.h>

typedef enum {
    FILTER_GAUSSIAN,
    FILTER_SOBEL
} filter_t;

/* Gaussian 3x3 kernel */
float gaussian3x3[3][3] = {
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
};

/* Gaussian 5x5 kernel */
float gaussian5x5[5][5] = {
    { 1,  4,  6,  4, 1},
    { 4, 16, 24, 16, 4},
    { 6, 24, 36, 24, 6},
    { 4, 16, 24, 16, 4},
    { 1,  4,  6,  4, 1}
};

/* Sobel 3x3 kernel, X */
float sobel3x3_x[3][3] = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1}
};

/* Sobel 3x3 kernel, Y */
float sobel3x3_y[3][3] = {
    {-1, -2, -1},
    { 0,  0,  0},
    { 1,  2,  1}
};

/* Sobel 5x5 kernel, X */
float sobel5x5_x[5][5] = {
    {-1, -2,  0,  2,  1},
    {-4, -8,  0,  8,  4},
    {-6,-12,  0, 12,  6},
    {-4, -8,  0,  8,  4},
    {-1, -2,  0,  2,  1}
};

/* Sobel 5x5 kernel, Y */
float sobel5x5_y[5][5] = {
    {-1, -4,  -6, -4, -1},
    {-2, -8, -12, -8, -2},
    { 0,  0,   0,  0,  0},
    { 2,  8,  12,  8,  2},
    { 1,  4,   6,  4,  1}
};

/* Clamp an integer to a closed range */
int clamp_int(int x, int min, int max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

/* Skip PGM header comments */
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

/* Read the PGM header */
void read_pgm_header(const char *filename, int *height, int *width) {
    FILE *f = fopen(filename, "rb");
    /* Validate file open */
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char magic[3];
    int maxval;

    /* Read the magic number */
    if (fscanf(f, "%2s", magic) != 1) {
        fprintf(stderr, "Error leyendo magic number\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    /* Validate binary PGM format */
    if (strcmp(magic, "P5") != 0) {
        fprintf(stderr, "Solo se soporta PGM binario (P5)\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    /* Read width, height, and maxval, skipping comments */
    fgetc(f);
    skip_comments(f);

    if (fscanf(f, "%d %d", width, height) != 2) {
        fprintf(stderr, "Error leyendo width/height\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    fgetc(f);
    skip_comments(f);

    /* Read maxval */
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

    /* The header is valid; the file pointer now points at the pixel data */
    fclose(f);
}

/* Read PGM pixel data */
void read_pgm_data(const char *filename, int height, int width, float img[height][width]) {
    FILE *f = fopen(filename, "rb");
    /* Validate file open */
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Skip the PGM header */
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

    /* Read pixel data */
    #pragma omp parallel for
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

/* Write a PGM image */
void write_pgm(const char *filename, int height, int width, float img[height][width]) {
    FILE *f = fopen(filename, "wb");
    /* Validate file open */
    if (!f) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Write the PGM header */
    fprintf(f, "P5\n%d %d\n255\n", width, height);

    /* Write clamped and rounded pixel data */
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
    int filter_dimension, halo_w, iterations;
    #ifdef _OPENMP
        double start_time, run_time;
    #else
        struct timeval tv_start, tv_end;
        double run_time;
    #endif
    filter_t filter_type;

    /* Validate argument count */
    if (argc != 5 && argc != 6) {
        fprintf(stderr,
            "Uso:\n"
            "  %s <filter_type> <filter_dimension> <input.pgm> <output.pgm> <iterations>\n\n"

            "Argumentos:\n"
            "  filter_type    Tipo de filtro a aplicar:\n"
            "                 gaussian | g   -> Suavizado Gaussiano\n"
            "                 sobel    | s   -> Detección de bordes Sobel\n\n"

            "  filter_dimension  Dimensión del kernel cuadrado (valores permitidos: 3 o 5)\n"
            "  input.pgm      Imagen de entrada en formato PGM (P5)\n"
            "  output.pgm     Imagen de salida procesada\n\n"
            "  iterations        Número de veces que se aplica el filtro\n"

            "Ejemplos:\n"
            "  %s gaussian 3 lenna.pgm output_OMP_g3x3.pgm\n"
            "  %s g 5 lenna.pgm output_OMP_g5x5.pgm\n"
            "  %s sobel 3 lenna.pgm output_OMP_s3x3.pgm\n"
            "  %s s 5 lenna.pgm output_OMP_s5x5.pgm\n",
            argv[0], argv[0], argv[0], argv[0], argv[0]
        );
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
    halo_w = filter_dimension/2;
    iterations = 1;  /* default */
    if (argc == 6) {
        iterations = atoi(argv[5]);
    }

    /* Validate filter dimension */
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

    /* Read the PGM header Validate the input PGM and read image dimensions */
    read_pgm_header(input_file, &height, &width);

    /* Allocate memory for the original and filtered images */
    float (*img)[width] = malloc(height * sizeof(*img));
    float (*filtered)[width] = malloc(height * sizeof(*filtered));

    /* Validate memory allocation */
    if (!img || !filtered) {
        perror("malloc");
        free(img);
        free(filtered);
        exit(EXIT_FAILURE);
    }

    /* Read image data */
    read_pgm_data(input_file, height, width, img);

    /* Compute norm for the Gaussian filter */
    float norm = 0.0f;
    if (filter_type == FILTER_GAUSSIAN) {
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
    /* Measure execution time */
    #ifdef _OPENMP
        start_time = omp_get_wtime();
    #else
        gettimeofday(&tv_start, NULL);
    #endif

    /* Apply the selected filter */
    for (int iter = 0; iter < iterations; iter++) {
        #pragma omp parallel for
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                /* Keep border pixels unchanged */
                if (i < halo_w || i >= height - halo_w ||
                    j < halo_w || j >= width  - halo_w) {
                    filtered[i][j] = img[i][j];
                } else {
                    /* GAUSSIAN FILTER */
                    if (filter_type == FILTER_GAUSSIAN) {
                        float sum = 0.0f;
                        for (int ki = -halo_w; ki <= halo_w; ki++) {
                            for (int kj = -halo_w; kj <= halo_w; kj++) {
                                if (filter_dimension == 3) {
                                    sum += img[i + ki][j + kj] *
                                        gaussian3x3[ki + 1][kj + 1];
                                } else { /* 5x5 */
                                    sum += img[i + ki][j + kj] *
                                        gaussian5x5[ki + 2][kj + 2];
                                }
                            }
                        }

                        filtered[i][j] = sum / norm;
                    }
                    /* SOBEL FILTER */
                    else if (filter_type == FILTER_SOBEL) {
                        float gx = 0.0f;
                        float gy = 0.0f;
                        for (int ki = -halo_w; ki <= halo_w; ki++) {
                            for (int kj = -halo_w; kj <= halo_w; kj++) {

                                float pixel = img[i + ki][j + kj];

                                if (filter_dimension == 3) {
                                    gx += pixel * sobel3x3_x[ki + 1][kj + 1];
                                    gy += pixel * sobel3x3_y[ki + 1][kj + 1];
                                } else { /* 5x5 */
                                    gx += pixel * sobel5x5_x[ki + 2][kj + 2];
                                    gy += pixel * sobel5x5_y[ki + 2][kj + 2];
                                }
                            }
                        }

                        /* Sobel gradient magnitude */
                        float mag = sqrtf(gx * gx + gy * gy);
                        if (mag > 255.0f) mag = 255.0f;
                        if (mag < 0.0f)   mag = 0.0f;
                        filtered[i][j] = mag;
                    }
                }
            }
        }

        if (iter < iterations - 1) {
            float (*tmp)[width] = img;
            img = filtered;
            filtered = tmp;
        }
    }

    /* Measure execution time */
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
        (filter_type == FILTER_GAUSSIAN) ? "Gaussiano" : "Sobel",
        filter_dimension,
        filter_dimension,
        run_time);

    write_pgm(output_file, height, width, filtered);

    char out_png[256];
    strncpy(out_png, output_file, sizeof(out_png) - 1);
    out_png[sizeof(out_png) - 1] = '\0';

    char *dot = strrchr(out_png, '.');
    if (dot != NULL && strcmp(dot, ".pgm") == 0) {
        *dot = '\0';
    }

    printf("Para visualizar:\n");
    printf("convert %s %s.png\n", output_file, out_png);

    free(img);
    free(filtered);

    return 0;
}