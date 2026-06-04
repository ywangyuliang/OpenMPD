# include <math.h>
# include <stdio.h>
# include <stdlib.h>
# include <time.h>
# include <omp.h>
#include <mpi.h>

int numprocs;
int myid;
int offset;
int cantidad;
int step;

int main ( int argc, char *argv[] );
void compute ( int np, int nd, double pos[], double vel[], 
  double mass, double f[], double *pot, double *kin );
double cpu_time ( );
double dist ( int nd, double r1[], double r2[], double dr[] );
void initialize ( int np, int nd, double pos[], double vel[], double acc[] );
void r8mat_uniform_ab ( int m, int n, double a, double b, int *seed, double r[] );
void timestamp ( );
void update ( int np, int nd, double pos[], double vel[], double f[], 
  double acc[], double mass, double dt );

/******************************************************************************/
int main ( int argc, char *argv[] )
/******************************************************************************/
{
  double *acc;
  double ctime;
  double dt;
  double e0;
  double *force;
  double kinetic;
  double mass = 1.0;
  int nd;
  int np;
  double *pos;
  double potential;

  int step_num;
  int step_print;
  int step_print_index;
  int step_print_num;
  double *vel;

  // 1.Inicializar  MPI
  MPI_Init(&argc, &argv);                      /* Initializing mpi       */

  MPI_Comm_size(MPI_COMM_WORLD, &numprocs); /* Getting # of processes */

  MPI_Comm_rank(MPI_COMM_WORLD, &myid);     /* Getting my rank        */

  timestamp ( );

  if(myid == 0){
    printf ( "\n" );
    printf ( "MD\n" );
    printf ( "  C version\n" );
    printf ( "  A molecular dynamics program.\n" );
  }
/*
  Get the spatial dimension.
*/
  if ( 1 < argc )
    nd = atoi ( argv[1] );
  else
  {
    if(myid == 0){
      printf ( "\n" );
      printf ( "  Enter ND, the spatial dimension (2 or 3).\n" );
    }
    if (scanf ( "%d", &nd ) <= 0)
      printf("scanf error en nd\n");
  }
//
//  Get the number of particles.
//
  if ( 2 < argc )
    np = atoi ( argv[2] );
  else
  { if(myid == 0){
      printf ( "\n" );
      printf ( "  Enter NP, the number of particles (500, for instance).\n" );
    }
    if (scanf ( "%d", &np ) <= 0)
      printf("scanf error en np\n");
  }
//
//  Get the number of time steps.
//
  if ( 3 < argc )
    step_num = atoi ( argv[3] );
  else
  { if(myid == 0){
      printf ( "\n" );
      printf ( "  Enter ND, the number of time steps (500 or 1000, for instance).\n" );
    }
    if (scanf ( "%d", &step_num ) <= 0)
      printf("scanf error en step_num\n");
  }
//
//  Get the time steps.
//
  if ( 4 < argc )
    dt = atof ( argv[4] );
  else
  { if(myid == 0){
      printf ( "\n" );
      printf ( "  Enter DT, the size of the time step (0.1, for instance).\n" );
    }
    if (scanf ( "%lf", &dt ) <= 0)
      printf("scanf error en dt\n");
  }
/*
  Report.
*/if(myid == 0){
    printf ( "\n" );
    printf ( "  ND, the spatial dimension, is %d\n", nd );
    printf ( "  NP, the number of particles in the simulation, is %d\n", np );
    printf ( "  STEP_NUM, the number of time steps, is %d\n", step_num );
    printf ( "  DT, the size of each time step, is %f\n", dt );
  }
/*
  Allocate memory.
*/


  acc = ( double * ) malloc ( nd * np * sizeof ( double ) );
  force = ( double * ) malloc ( nd * np * sizeof ( double ) );
  pos = ( double * ) malloc ( nd * np * sizeof ( double ) );
  vel = ( double * ) malloc ( nd * np * sizeof ( double ) );
  double * posCopia = ( double * ) malloc ( nd * np * sizeof ( double ) );
  double * direccion;
/*
  This is the main time stepping loop:
    Compute forces and energies,
    Update positions, velocities, accelerations.
*/
  if(myid == 0){
    printf ( "\n" );
    printf ( "  At each step, we report the potential and kinetic energies.\n" );
    printf ( "  The sum of these energies should be a constant.\n" );
    printf ( "  As an accuracy check, we also print the relative error\n" );
    printf ( "  in the total energy.\n" );
    printf ( "\n" );
    printf ( "      Step      Potential       Kinetic        (P+K-E0)/E0\n" );
    printf ( "                Energy P        Energy K       Relative Energy Error\n" );
    printf ( "\n" );
  }

  step_print = 0;
  step_print_index = 0;
  step_print_num = 10;
  double pot = 0.0;
  double kin = 0.0;
  cantidad = np/numprocs;
  offset = myid * np/numprocs; 
  
  double start_time = 0.0; //omp_get_wtime();

  for ( step = 0; step <= step_num; step++ )
  {
    if ( step == 0 ){
      initialize ( np, nd, pos, vel, acc );
    }
    else{
    
      update ( np, nd, pos, vel, force, acc, mass, dt );
      
      MPI_Allgather(pos+offset*nd, nd * cantidad, MPI_DOUBLE, posCopia, nd * cantidad, MPI_DOUBLE,
      MPI_COMM_WORLD);
  
      direccion = pos;
      pos = posCopia;
      posCopia = direccion;
      
    }
    compute ( np, nd, pos, vel, mass, force, &potential, &kinetic );

    if ( step == step_print )
    {
      MPI_Reduce(&potential, &pot, 1, MPI_DOUBLE, MPI_SUM, 0,  MPI_COMM_WORLD);
      MPI_Reduce(&kinetic, &kin, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

       if ( step == 0 )
        e0 = pot + kin;

      if(myid == 0){
        printf ( "  %8d  %14f  %14f  %14e\n", step, pot, kin,( pot + kin - e0 ) / e0 );
      }

      step_print_index = step_print_index + 1;
      step_print = ( step_print_index * step_num ) / step_print_num;
    }
  }
/*
  Report timing.
*/

  double run_time = 0.0;   // omp_get_wtime() - start_time;
  if(myid == 0) printf("\n Tiempo =  %f segundos\n",run_time);

/*
  Free memory.
*/
  free ( acc );
  free ( force );
  free ( pos );
  free ( vel );
/*
  Terminate.
*/
  if(myid == 0){
    printf ("\n");
    printf ("MD\n");
    printf ("  Normal end of execution.\n");
    printf ("\n");
    timestamp ();
  }
  MPI_Finalize();

  return 0;
}
/******************************************************************************/
void compute ( int np, int nd, double pos[], double vel[], double mass, 
  double f[], double *pot, double *kin )
/******************************************************************************/

{
  double d;
  double d2;
  int i,j,k,r;
  double ke;
  double pe;
  double PI2 = 3.141592653589793 / 2.0;
  double rij[3];

  pe = 0.0;
  ke = 0.0;


  #pragma omp parallel for simd schedule(static) private(i,k,j,d2,d,rij) reduction(+:pe) reduction(+:ke)
  for ( k = offset; k < (offset + cantidad); k++ )
  {

// Compute the potential energy and forces.
    for ( i = 0; i < nd; i++ )
      f[i+k*nd] = 0.0;

    for ( j = 0; j < np; j++ )
    {
      if ( k != j )
      {
        d = dist ( nd, pos+k*nd, pos+j*nd, rij );
       

//  Attribute half of the potential energy to particle J.
        if ( d < PI2 )
          d2 = d;
        else
          d2 = PI2;

        pe = pe + 0.5 * pow ( sin ( d2 ), 2 );

       
        for ( i = 0; i < nd; i++ )
          f[i+k*nd] = f[i+k*nd] - rij[i] * sin ( 2.0 * d2 ) / d;
      }
    }
//  Compute the kinetic energy.
    for ( i = 0; i < nd; i++ )
      ke = ke + vel[i+k*nd] * vel[i+k*nd];
  }

  ke = ke * 0.5 * mass;
  
  *pot = pe;
  *kin = ke;

  return;
}
/*******************************************************************************/
double cpu_time ( )
/*******************************************************************************/

{
  double value;
  value = ( double ) clock ( ) / ( double ) CLOCKS_PER_SEC;

  return value;
}
/******************************************************************************/
double dist ( int nd, double r1[], double r2[], double dr[] )
/******************************************************************************/

{
  double d;
  int i;

  d = 0.0;
  
  for ( i = 0; i < nd; i++ )
  {
    dr[i] = r1[i] - r2[i];
    d = d + dr[i] * dr[i];
  }
  d = sqrt ( d );

  return d;
}
/******************************************************************************/
void initialize ( int np, int nd, double pos[], double vel[], double acc[] )
/******************************************************************************/

{
  int i;
  int j;
  int seed;

//  Set positions.
  seed = 123456789;
  r8mat_uniform_ab ( nd, np, 0.0, 10.0, &seed, pos );

//  Set velocities.
  for ( j = 0; j < np; j++ )
  {
    for ( i = 0; i < nd; i++ )
    {
      vel[i+j*nd] = 0.0;
    }
  }
/*
  Set accelerations.
*/
  for ( j = 0; j < np; j++ )
  {
    for ( i = 0; i < nd; i++ )
      acc[i+j*nd] = 0.0;
  }

  return;
}
/******************************************************************************/
void r8mat_uniform_ab ( int m, int n, double a, double b, int *seed, double r[] )
/******************************************************************************/

{
  int i;
  const int i4_huge = 2147483647;
  int j;
  int k;

  if ( *seed == 0 )
  {
    fprintf ( stderr, "\n" );
    fprintf ( stderr, "R8MAT_UNIFORM_AB - Fatal error!\n" );
    fprintf ( stderr, "  Input value of SEED = 0.\n" );
    exit ( 1 );
  }

  for ( j = 0; j < n; j++ )
  {
    for ( i = 0; i < m; i++ )
    {
      k = *seed / 127773;

      *seed = 16807 * ( *seed - k * 127773 ) - k * 2836;

      if ( *seed < 0 )
        *seed = *seed + i4_huge;
      r[i+j*m] = a + ( b - a ) * ( double ) ( *seed ) * 4.656612875E-10;
    }
  }

  return;
}
/******************************************************************************/

void timestamp ( )

/******************************************************************************/

{
# define TIME_SIZE 40

  static char time_buffer[TIME_SIZE];
  const struct tm *tm;
  time_t now;

  now = time ( NULL );
  tm = localtime ( &now );

  strftime ( time_buffer, TIME_SIZE, "%d %B %Y %I:%M:%S %p", tm );

  printf ( "%s\n", time_buffer );

  return;
# undef TIME_SIZE
}
/******************************************************************************/

void update ( int np, int nd, double pos[], double vel[], double f[], 
  double acc[], double mass, double dt )

/******************************************************************************/

{
  int i;
  int j;
  double rmass;

  rmass = 1.0 / mass;

#pragma omp parallel for simd private(i,j) schedule(static)
  for ( j = offset; j < offset + cantidad; j++ )
  {
    for ( i = 0; i < nd; i++ )
    {
      pos[i+j*nd] = pos[i+j*nd] + vel[i+j*nd] * dt + 0.5 * acc[i+j*nd] * dt * dt;
      vel[i+j*nd] = vel[i+j*nd] + 0.5 * dt * ( f[i+j*nd] * rmass + acc[i+j*nd] );
      acc[i+j*nd] = f[i+j*nd] * rmass;
    }
  }

  return;
}
