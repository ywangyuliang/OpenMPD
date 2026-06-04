#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <omp.h> 
#include <sys/time.h>
#include <mpi.h>

#define mm 15
#define npart 4*mm*mm*mm

// Function declarations
//
  void dfill(int,double,double[],int);
  void domove(int,double[],double[],double[],double);
  void dscal(int,double,double[],int);
  void fcc(double[],int,int,double);
  void forces(int,double[],double[],double,double,int,int,int);
  double mkekin(int,double[],double[],double,double);
  void mxwell(double[],int,double,double);
  void prnout(int,double,double,double,double,double,double,int,double);
  double velavg(int,double[],double,double);

//  Variable declarations

  double epot;
  double vir;
  double count;

// Main program : Molecular Dynamics simulation.
int main( int argc, char *argv[] ){
    int move;
    double x[npart*3], vh[npart*3], f[npart*3];
    double ekin;
    double vel;
    double sc;
    //Inicializamos MPI
    int myid,numprocs;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
#ifdef _OPENMP
    double start, time;
#else
    struct timeval tv_start, tv_end;
    float tiempo_trans;
#endif

  //  Parameter definitions
    double den    = 0.83134;
    double side   = pow((double)npart/den,0.3333333);
    double tref   = 0.722;
    double rcoff  = (double)mm/4.0;
    double h      = 0.064;
    int    irep   = 10;
    int    istop  = 20;
    int    iprint = 5;
    int    movemx = 50;

    double a      = side/(double)mm;
    double hsq    = h*h;
    double hsq2   = hsq*0.5;
    double tscale = 16.0/((double)npart-1.0);
    double vaver  = 1.13*sqrt(tref/24.0);

    if ( 1 < argc )
        movemx = atoi ( argv[1] );

  //  Initial output
    if(myid == 0){
    printf(" Molecular Dynamics Simulation example program\n");
    printf(" ---------------------------------------------\n");
    printf(" number of particles is ............ %6d\n",npart);
    printf(" side length of the box is ......... %13.6f\n",side);
    printf(" cut off is ........................ %13.6f\n",rcoff);
    printf(" reduced temperature is ............ %13.6f\n",tref);
    printf(" basic timestep is ................. %13.6f\n",h);
    printf(" temperature scale interval ........ %6d\n",irep);
    printf(" stop scaling at move .............. %6d\n",istop);
    printf(" print interval .................... %6d\n",iprint);
    printf(" total no. of steps ................ %6d\n",movemx);
    

  // Generate fcc lattice for atoms inside box
    fcc(x, npart, mm, a);

   //  Initialise velocities and forces (which are zero in fcc positions)
    mxwell(vh, 3*npart, h, tref);
    dfill(3*npart, 0.0, f, 1);

  //  Start of md
    printf("\n    i       ke         pe            e         temp   "
           "   pres      vel      rp\n  -----  ----------  ----------"
           "  ----------  --------  --------  --------  ----\n");
    }
#ifdef _OPENMP
    start = MPI_Wtime(); 
#else
    gettimeofday(&tv_start, NULL);
#endif
    
    for (move=1; move<=movemx; move++) {

    //  Move the particles and partially update velocities
      if(myid == 0)
	domove(3*npart, x, vh, f, side);

    //  Compute forces in the new positions and accumulate the virial
    //  and potential energy.
      MPI_Bcast(x, npart*3, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      int num_op=(npart)/numprocs;
      forces(npart, x, f, side, rcoff,num_op,myid,numprocs);
     
      if(myid == 0){
	//  Scale forces, complete update of velocities and compute k.e.
	ekin=mkekin(npart, f, vh, hsq2, hsq);

	//  Average the velocity and temperature scale if desired
	vel=velavg(npart, vh, vaver, h);
	if (move<istop && fmod(move, irep)==0) {
	  sc=sqrt(tref/(tscale*ekin));
	  dscal(3*npart, sc, vh, 1);
	  ekin=tref/tscale;
	}

	//  Sum to get full potential energy and virial
	if (fmod(move, iprint)==0)
	  prnout(move, ekin, epot, tscale, vir, vel, count, npart, den);
      }
    }
  
#ifdef _OPENMP
    time = MPI_Wtime() - start;
    MPI_Finalize();
    if(myid == 0)
      printf("Time =  %f\n",(float) time);  
#else
    gettimeofday(&tv_end, NULL);
    tiempo_trans=(tv_end.tv_sec - tv_start.tv_sec) * 1000000 +
      (tv_end.tv_usec - tv_start.tv_usec); //en us
    if(myid == 0)
      printf("Time = %f\n", tiempo_trans/1000000);
#endif
}
