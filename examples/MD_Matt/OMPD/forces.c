/* Compute forces and accumulate the virial and the potential */

  extern double epot, vir;

  void
  forces(int npart, double x[], double f[], double side, double rcoff){

    int   i, j;
    vir   = 0.0;
    epot  = 0.0;
    double xx,yy,zz,rd,xi,yi,zi,fzi,fxi,fyi,rrd,rrd3,rrd4,r148;
    double inf = -0.5*side;
    double sup = 0.5*side;
    double rcof2 = rcoff*rcoff;
    int npart3 = npart*3;

#pragma omp cluster broad(x[npart*3]) reduction(+:epot,vir) reduction(+:f[npart*3]) (static,3)
#pragma omp distribute dist_schedule(static,3)
#pragma omp parallel for simd shared(npart3,inf,sup,rcof2,side) private(i,j,xi,yi,zi,xx,yy,zz,rd,rrd,rrd3,rrd4,r148,fxi,fyi,fzi)
    /* schedule(dynamic) reduction(+:epot) reduction(-:vir) */
      for (i=0; i<npart3; i+=3) {
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

	    epot    += rrd3*(rrd3-1.0);
	    vir     -= rd*r148;

	    fxi     += xx*r148;
	    fyi     += yy*r148;
	    fzi     += zz*r148;
#pragma omp atomic
	    f[j]    -= xx*r148;
#pragma omp atomic
	    f[j+1]  -= yy*r148;
#pragma omp atomic
	    f[j+2]  -= zz*r148;
	  }
        }
	/* update forces on particle  */
#pragma omp atomic
	f[i]   += fxi;
#pragma omp atomic
	f[i+1]   += fyi;
#pragma omp atomic
	f[i+2]   += fzi;
      }
  }
  }
