/* ran4 cryptographic random number generator */
/* (C) Copr. 1986-92 Numerical Recipes Software #.,. */
/* Modified by Thomas M. Bartol, Ph.D. */
/* hoc interface at end added by Michale Hines */

#define NITER 2
static unsigned int idums = 0;

void mcell_ran4_init(idum)
  unsigned int *idum;
{
	idums = *idum;
	*idum=1;
	return;
}

double mcell_ran4_64(idum, ilow, ran_vec,n,range)
  unsigned int *idum,n;
  unsigned int ilow;
  double *ran_vec,range;
{
	unsigned int irword,lword;
        unsigned int i,j;
        unsigned int ia,ib,iswap,itmph=0,itmpl=0;
        static unsigned int c1[4]={
                0xbaa96887L, 0x1e17d32cL, 0x03bcdc3cL, 0x0f33d1b2L};
        static unsigned int c2[4]={
                0x4b0f3b58L, 0xe874f0c3L, 0x6955c5a6L, 0x55a7ca46L};

        range=range*2.3283064365386963e-10;
	lword=ilow;

	for (i=0;i<n;i++) {
	  irword=(*idum);

/*
          printf("before: %x %x\n",lword,irword);
*/

          for (j=0;j<NITER;j++) {
                ia=(iswap=(irword)) ^ c1[j];
                itmpl = ia & 0xffff;
                itmph = ia >> 16;
                ib=itmpl*itmpl+ ~(itmph*itmph);
                irword=(lword) ^ (((ia = (ib >> 16) |
                        ((ib & 0xffff) << 16)) ^ c2[j])+itmpl*itmph);
                lword=iswap;
          }

/*
          printf("after: %x %x\n",lword,irword);
*/
	  ran_vec[i]=range*irword;

	  ++(*idum);
        }
        return(ran_vec[0]);
}

double mcell_ran4(idum, ran_vec,n,range)
  unsigned int *idum,n;
  double *ran_vec,range;
{
	return mcell_ran4_64(idum, idums, ran_vec, n, range);
}

unsigned int mcell_iran4_64(idum, ilow, iran_vec,n)
  unsigned int *idum, ilow, *iran_vec,n;
{
	unsigned int irword,lword;
        unsigned int i,j,ia,ib,iswap,itmph=0,itmpl=0;
        static unsigned int c1[4]={
                0xbaa96887L, 0x1e17d32cL, 0x03bcdc3cL, 0x0f33d1b2L};
        static unsigned int c2[4]={
                0x4b0f3b58L, 0xe874f0c3L, 0x6955c5a6L, 0x55a7ca46L};

	lword=ilow;

	for (i=0;i<n;i++) {
	  irword=(*idum);

/*
          printf("before: %x %x\n",lword,irword);
*/

          for (j=0;j<NITER;j++) {
                ia=(iswap=(irword)) ^ c1[j];
                itmpl = ia & 0xffff;
                itmph = ia >> 16;
                ib=itmpl*itmpl+ ~(itmph*itmph);
                irword=(lword) ^ (((ia = (ib >> 16) |
                        ((ib & 0xffff) << 16)) ^ c2[j])+itmpl*itmph);
                lword=iswap;
          }

/*
          printf("after: %x %x\n",lword,irword);
*/
	  iran_vec[i]=irword;

	  ++(*idum);
        }
        return(iran_vec[0]);
}

unsigned int mcell_iran4(idum,iran_vec,n)
  unsigned int *idum,*iran_vec,n;
{
	return mcell_iran4_64( idum, idums, iran_vec, n);
}

#undef NITER

/* Hoc interface */
extern double chkarg(), *hoc_pgetarg();
extern int use_mcell_ran4_;

void hoc_mcran4() {
	unsigned int idum;
	double* xdum;
	double x;
	xdum = hoc_pgetarg(1);
	idum = (unsigned int)(*xdum);
	mcell_ran4(&idum, &x, 1, 1.);
	*xdum = idum;
	hoc_ret();
	hoc_pushx(x);
}
void hoc_mcran4init() {
	double prev = (double)idums;
	if (ifarg(1)) {
		unsigned int idum = (unsigned int) chkarg(1, 0., 4294967295.);
		mcell_ran4_init(&idum);
	}
	hoc_ret();
	hoc_pushx(prev);
	
}
void hoc_usemcran4() {
	double prev = (double)use_mcell_ran4_;
	if (ifarg(1)) {
		use_mcell_ran4_ = (int)chkarg(1, 0., 1.);
	}
	hoc_ret();
	hoc_pushx(prev);
}
