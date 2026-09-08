#!/bin/bash

cmp output_SEQ_g3x3.pgm output_OMP_g3x3.pgm && echo "Gaussian 3x3: OK"
cmp output_SEQ_g5x5.pgm output_OMP_g5x5.pgm && echo "Gaussian 5x5: OK"
cmp output_SEQ_s3x3.pgm output_OMP_s3x3.pgm && echo "Sobel 3x3: OK"
cmp output_SEQ_s5x5.pgm output_OMP_s5x5.pgm && echo "Sobel 5x5: OK"