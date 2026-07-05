/* Compute forces and accumulate the virial and the potential */
#include <mpi.h>
#include <stdio.h>
extern double epot, vir;

  void
  forces(int npart, double x[], double f[], double side, double rcoff,int num_op,int id, int nprocs){

    int   i, j;
    vir   = 0.0;
    epot  = 0.0;
    double xx,yy,zz,rd,xi,yi,zi,fzi,fxi,fyi,rrd,rrd3,rrd4,r148;
    double inf = -0.5*side;
    double sup = 0.5*side;
    double rcof2 = rcoff*rcoff;
    int npart3 = npart*3;

    double f_aux[npart3];
    double epotc, virc;
    epotc = 0.0;
    virc  = 0.0;

    for(i = 0; i<npart3; i++)
      f_aux[i] = 0.0;

#pragma omp parallel for simd shared(npart3,inf,sup,rcof2,side,f_aux) private(i,j,xi,yi,zi,xx,yy,zz,rd,rrd,rrd3,rrd4,r148,fxi,fyi,fzi) reduction(+:epotc) reduction(-:virc) schedule(dynamic)
	for (i=id*3; i<npart3; i+=3*nprocs) {
	  /* zero force components on particle i */
	  fxi = 0.0;
	  fyi = 0.0;
	  fzi = 0.0;
	  xi=x[i];
	  yi=x[i+1];
	  zi=x[i+2];

	  /* loop over all particles with index > i */
	  for (j=i+3; j<npart3; j+=3) {

	    /* compute distance between particles i and j allowing for wraparound  */
	    xx = xi-x[j];
	    yy = yi-x[j+1];
	    zz = zi-x[j+2];

	    if (xx< inf) xx += side;
	    if (xx> sup) xx -= side;
	    if (yy< inf) yy += side;
	    if (yy> sup) yy -= side;
	    if (zz< inf) zz += side;
	    if (zz> sup) zz -= side;

	    rd = xx*xx+yy*yy+zz*zz;

	    /* if distance is inside cutoff radius compute forces and contributions to pot. energy and virial */
	    if (rd<=rcof2) {
	      rrd      = 1.0/rd;
	      rrd3     = rrd*rrd*rrd;
	      rrd4     = rrd3*rrd;
	      r148     = rrd4*(rrd3-0.5);

	      epotc    += rrd3*(rrd3-1.0);
	      virc     -= rd*r148;

	      fxi     += xx*r148;
	      fyi     += yy*r148;
	      fzi     += zz*r148;
#pragma omp atomic
	      f_aux[j]  -= xx*r148;
#pragma omp atomic
	      f_aux[j+1]  -= yy*r148;
#pragma omp atomic
	      f_aux[j+2]  -= zz*r148;
	    }
	  }
	  /* update forces on particle  */
#pragma omp atomic
	  f_aux[i]     += fxi;
#pragma omp atomic
	  f_aux[i+1]   += fyi;
#pragma omp atomic
	  f_aux[i+2]   += fzi;
	}

    MPI_Reduce(&epotc, &epot, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&virc, &vir, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(f_aux, f, npart3, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  }
