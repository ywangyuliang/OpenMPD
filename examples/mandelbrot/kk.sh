echo time ./Mandelbrot-OMP
time ./Mandelbrot-OMP
echo time mpirun -np 40 ./Mandelbrot-MPI
time mpirun -np 40 ./Mandelbrot-MPI
echo time mpirun -np 40 ./Mandelbrot-OMPD_mpi
time mpirun -np 40 ./Mandelbrot-OMPD_mpi
echo time mpirun -np 40 ./Mandelbrot-OMPD_mpi-contiguo
time mpirun -np 40 ./Mandelbrot-OMPD_mpi-contiguo
echo time ./Mandelbrot-SEQ
time ./Mandelbrot-SEQ
