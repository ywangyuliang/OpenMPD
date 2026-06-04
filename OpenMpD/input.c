# include <stdlib.h>
# include <stdio.h>
# include <math.h>
# include <time.h>
# include <sys/time.h>
# include <omp.h>
# include <assert.h>
# include <mpi.h>


int main ( int argc, char *argv[] ){
	# define MASTER 0
	#ifdef _OPENMP
  		double start_time, run_time;
	#else
		struct timeval tv_start, tv_end;
		double run_time;
	#endif

	int M = 500;
	int N = 500;
	double diff;
	double aux_diff;
	double epsilon;
	int i;
	int iterations;
	int iterations_print;
	int j;
	double mean;
	FILE *output;
	char output_filename[80];
	int success;
	double u[M][N];
	double w[M][N];

	printf ( "\n" );
	printf ( "HEATED_PLATE <epsilon> <fichero-salida>\n" );
	printf ( "  C/serie version\n" );
	printf ( "  A program to solve for the steady state temperature distribution\n" );
	printf ( "  over a rectangular plate.\n" );
	printf ( "\n" );
	printf ( "  Spatial grid of %d by %d points.\n", M, N );


	epsilon = atof(argv[1]);
	printf("The iteration will be repeated until the change is <= %lf\n", epsilon);
	diff = epsilon;

	success = sscanf ( argv[2], "%s", output_filename );
	if ( success != 1 ) {
		printf ( "\n" );
		printf ( "HEATED_PLATE\n" );
		printf ( " Error en la lectura del nombre del fichero de salida\n");
		exit(1);
	}

 	printf("  The steady state solution will be written to %s\n", output_filename);

	mean = 0.0;
	for ( i = 1; i < M - 1; i++ ) {
		w[i][0] = 100.0;
		mean += w[i][0];
	}
	for ( i = 1; i < M - 1; i++ ) {
			w[i][N-1] = 100.0;
		mean += w[i][N-1];
	}

	for ( j = 0; j < N; j++ ) {
			w[M-1][j] = 100.0;
		mean += w[M-1][j]; 
	}

	for ( j = 0; j < N; j++ ) {
		w[0][j] = 0.0;
		mean += w[0][j];
	}

  	mean = mean / ( double ) ( 2 * M + 2 * N - 4 );

	printf ( "\n" );
	printf ( "  MEAN = %lf\n", mean );


	for ( i = 1; i < M - 1; i++ ){
		for ( j = 1; j < N - 1; j++ ){
				w[i][j] = mean;
		}
	}

	iterations = 0;
	iterations_print = 1;
	printf ( "\n" );
	printf ( " Iteration  Change\n" );
	printf ( "\n" );

	#ifdef _OPENMP
  		start_time = omp_get_wtime();
	#else
  		gettimeofday(&tv_start, NULL);
	#endif



	#pragma omp cluster broad(epsilon, diff, M,N) alloc(u[M][N]) scatter(w[M][N]) halo(w[M][N]:chunk(N))

	//#pragma omp teams
	while ( epsilon <= diff ){
		//secuencial
		//#pragma omp parallel for private(j)
		#pragma omp parallel for private(j)
		for ( i = 0; i < M; i++ )
			for ( j = 0; j < N; j++ )
				__u[i][j] = __w[i][j];
		
		
		diff = 0.0;
		//fin secuencial

		//#pragma omp cluster update allreduction(max:diff)
		{
			double __diff = 0.0;
			

			// Considero el halo, por encima y debajo, para que coincidan las filas y 
			// las iteraciones
			int first = 1 - 1; // first - halo
			int last = M - 1 + 1;  // last - halo
			int __iter = -1;
			int __start = -1;
			int __end = -1;

			__iter = ((last - first)  / __numprocs);
			if (__taskid < ((last -first) % __numprocs))
				__iter++;
			__start =  first + __iter * __taskid ;
			if (__taskid >= ((last - first) % __numprocs))
				__start += ((last - first) % __numprocs);
			__end = __start + __iter ;
			if (__taskid == (__numprocs-1)) assert (__end == last);
			if (__taskid == 0) __start += 1; // halo
			if (__taskid == (__numprocs-1)) __end -= 1; // halo

				#pragma omp parallel for private(aux_diff, j) reduction(max:__diff)
				for ( i = __start; i < __end; i++ ){
				for ( j = 1; j < N - 1; j++ )
					{
						__w[i][j] = ( __u[i-1][j] + __u[i+1][j] + __u[i][j-1] + __u[i][j+1] ) * 0.25;
						aux_diff = fabs ( __w[i][j] - __u[i][j] );
						__diff = __diff < aux_diff ? aux_diff : __diff;
					}
				}
			MPI_Allreduce(&__diff, &diff, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
		}
		
		iterations++;

		#pragma omp cluster update halo(w[M][N]:chunk(N))
    	// #pragma omp cluster teams master
		if (__taskid == 0) {
			if ( iterations == iterations_print )
			{
				printf ( "  %8d  %lg\n", iterations, diff );
				iterations_print = 2 * iterations_print;
			}
		}
  	} //fin while epsilon

	// Gather de w
	int __chunk_w;
	int *displs_w = (int *)malloc(__numprocs*sizeof(int));
	int *counts_w = (int *)malloc(__numprocs*sizeof(int));

	__chunk_w = (M / __numprocs);
	if (__taskid < (M % __numprocs))
		counts_w[__taskid] = (__chunk_w+1) * N;
	else
		counts_w[__taskid] = __chunk_w * N;
	displs_w[__taskid] = __chunk_w * N * __taskid;
	if (__taskid < (M % __numprocs))
		displs_w[__taskid] += __taskid * N;
	else  displs_w[__taskid] += (M % __numprocs) * N;

	if (__taskid == 0) {
		for (i=0; i < __numprocs; i++)
			if (i < (M % __numprocs))
				counts_w[i] = (__chunk_w+1) * N;
			else
				counts_w[i] = __chunk_w * N;
		displs_w[0] = 0;
		for (i=1; i < __numprocs; i++)
			if (i <= (M % __numprocs))
				displs_w[i] = displs_w[i-1] + (__chunk_w+1) * N;
			else
				displs_w[i] = displs_w[i-1] + __chunk_w * N;
		assert ((displs_w[__numprocs-1] + counts_w[__numprocs-1]) == M*N);
	}

	MPI_Gatherv((&__w[0][0])+displs_w[__taskid], counts_w[__taskid], MPI_DOUBLE, &w[0][0], counts_w, displs_w, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	#ifdef _OPENMP
		run_time = omp_get_wtime() - start_time;
	#else
		gettimeofday(&tv_end, NULL);
		run_time=(tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
				(tv_end.tv_usec - tv_start.tv_usec); //en us
		run_time = run_time/1000000; // en s
	#endif


	printf ( "\n" );
	printf ( "  %8d  %lg\n", iterations, diff );
	printf ( "\n" );
	printf ( "  Error tolerance achieved.\n" );
	printf("\n Tiempo version Secuencial = %lg s\n", run_time);

	//  Write the solution to the output file.
	//
	output = fopen(output_filename, "wt");

	fprintf(output, "%d\n", M);
	fprintf(output, "%d\n", N);

	for ( i = 0; i < M; i++ )
	{
		for ( j = 0; j < N; j++)
		{
		fprintf(output, "%lg ", w[i][j]);
		}
		fprintf(output, "\n");
	}
	fclose(output);

	printf ( "\n" );
	printf ( " Solucion escrita en el fichero %s\n", output_filename );
	// 
	//  Terminate.
	//
	printf ( "\n" );
	printf ( "HEATED_PLATE_Serie:\n" );
	printf ( "  Normal end of execution.\n" );

	return 0;
}
