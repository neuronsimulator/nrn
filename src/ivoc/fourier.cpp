#include <../../nrnconf.h>
/* (C) Copr. 1986-92 Numerical Recipes Software #.,. */

/* Algorithms for Fourier Transform Spectral Methods */

#include <math.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#undef myfabs
#if MAC
#if __GNUC__ < 4
#define myfabs std::fabs
#else
#define myfabs ::fabs
#endif
#else
#define myfabs fabs
#endif

#define NR_END 1
#define FREE_ARG char*

static double sqrarg;
#define SQR(a) ((sqrarg=(a)) == 0.0 ? 0.0 : sqrarg*sqrarg)

extern "C" {
void hoc_execerror(const char*, const char*);
extern void nrn_exit(int);
}

static void nrerror(char error_text[])
/* Numerical Recipes standard error handler */
{
#if 1
	hoc_execerror("Numerical Recipes run-time error...\n", error_text);
#else
	fprintf(stderr,"Numerical Recipes run-time error...\n");
	fprintf(stderr,"%s\n",error_text);
	fprintf(stderr,"...now exiting to system...\n");
	nrn_exit(1);
#endif
}


static double *vector(long nl, long nh)
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;

	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	if (!v) nrerror("allocation failure in vector()");
	return v-nl+NR_END;
}

static void free_vector(double *v, long nl, long nh)
/* free a double vector allocated with vector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}



/* 
  four1()  -- complex discrete FFT and inverse FFT
  N.R.C  p. 411
*/

#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr

void four1(double data[], unsigned long nn, int isign)
{
	unsigned long n,mmax,m,j,istep,i;
	double wtemp,wr,wpr,wpi,wi,theta;
	double tempr,tempi;

	n=nn << 1;
	j=1;
	for (i=1;i<n;i+=2) {
		if (j > i) {
			SWAP(data[j],data[i]);
			SWAP(data[j+1],data[i+1]);
		}
		m=n >> 1;
		while (m >= 2 && j > m) {
			j -= m;
			m >>= 1;
		}
		j += m;
	}
	mmax=2;
	while (n > mmax) {
		istep=mmax << 1;
		theta=isign*(6.28318530717959/mmax);
		wtemp=sin(0.5*theta);
		wpr = -2.0*wtemp*wtemp;
		wpi=sin(theta);
		wr=1.0;
		wi=0.0;
		for (m=1;m<mmax;m+=2) {
			for (i=m;i<=n;i+=istep) {
				j=i+mmax;
				tempr=wr*data[j]-wi*data[j+1];
				tempi=wr*data[j+1]+wi*data[j];
				data[j]=data[i]-tempr;
				data[j+1]=data[i+1]-tempi;
				data[i] += tempr;
				data[i+1] += tempi;
			}
			wr=(wtemp=wr)*wpr-wi*wpi+wr;
			wi=wi*wpr+wtemp*wpi+wi;
		}
		mmax=istep;
	}
}
#undef SWAP

/* 
  twofft()  -- discrete FFT of two real functions simultaneously
  N.R.C  p. 414
*/

void twofft(double data1[], double data2[], double fft1[], double fft2[],
	unsigned long n)
{
	void four1(double data[], unsigned long nn, int isign);
	unsigned long nn3,nn2,jj,j;
	double rep,rem,aip,aim;

	nn3=1+(nn2=2+n+n);
	for (j=1,jj=2;j<=n;j++,jj+=2) {
		fft1[jj-1]=data1[j];
		fft1[jj]=data2[j];
	}
	four1(fft1,n,1);
	fft2[1]=fft1[2];
	fft1[2]=fft2[2]=0.0;
	for (j=3;j<=n+1;j+=2) {
		rep=0.5*(fft1[j]+fft1[nn2-j]);
		rem=0.5*(fft1[j]-fft1[nn2-j]);
		aip=0.5*(fft1[j+1]+fft1[nn3-j]);
		aim=0.5*(fft1[j+1]-fft1[nn3-j]);
		fft1[j]=rep;
		fft1[j+1]=aim;
		fft1[nn2-j]=rep;
		fft1[nn3-j] = -aim;
		fft2[j]=aip;
		fft2[j+1] = -rem;
		fft2[nn2-j]=aip;
		fft2[nn3-j]=rem;
	}
}


/* 
  realft()  -- discrete FFT of a real function with 2n data pts
  N.R.C  p. 417
*/

void realft(double data[], unsigned long n, int isign)
{
	void four1(double data[], unsigned long nn, int isign);
	unsigned long i,i1,i2,i3,i4,np3;
	double c1=0.5,c2,h1r,h1i,h2r,h2i;
	double wr,wi,wpr,wpi,wtemp,theta;

	theta=3.141592653589793/(double) (n>>1);
	if (isign == 1) {
		c2 = -0.5;
		four1(data,n>>1,1);
	} else {
		c2=0.5;
		theta = -theta;
	}
	wtemp=sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi=sin(theta);
	wr=1.0+wpr;
	wi=wpi;
	np3=n+3;
	for (i=2;i<=(n>>2);i++) {
		i4=1+(i3=np3-(i2=1+(i1=i+i-1)));
		h1r=c1*(data[i1]+data[i3]);
		h1i=c1*(data[i2]-data[i4]);
		h2r = -c2*(data[i2]+data[i4]);
		h2i=c2*(data[i1]-data[i3]);
		data[i1]=h1r+wr*h2r-wi*h2i;
		data[i2]=h1i+wr*h2i+wi*h2r;
		data[i3]=h1r-wr*h2r+wi*h2i;
		data[i4] = -h1i+wr*h2i+wi*h2r;
		wr=(wtemp=wr)*wpr-wi*wpi+wr;
		wi=wi*wpr+wtemp*wpi+wi;
	}
	if (isign == 1) {
		data[1] = (h1r=data[1])+data[2];
		data[2] = h1r-data[2];
	} else {
		data[1]=c1*((h1r=data[1])+data[2]);
		data[2]=c1*(h1r-data[2]);
		four1(data,n>>1,-1);
	}
}


/* 
  convlv()  -- convolution of a read data set with a response function

  the respns function must be stored in wrap-around order 
 
  N.R.C  p. 430
*/



void convlv(double data[], unsigned long n, double respns[], unsigned long m,
	int isign, double ans[])
{
	void realft(double data[], unsigned long n, int isign);
	void twofft(double data1[], double data2[], double fft1[], double fft2[],
		unsigned long n);
	unsigned long i,no2;
	double dum,mag2,*fft;

	fft=vector(1,n<<1);
	for (i=1;i<=(m-1)/2;i++)
		respns[n+1-i]=respns[m+1-i];
	for (i=(m+3)/2;i<=n-(m-1)/2;i++)
		respns[i]=0.0;
	twofft(data,respns,fft,ans,n);
	no2=n>>1;
	for (i=2;i<=n+2;i+=2) {
		if (isign == 1) {
			ans[i-1]=(fft[i-1]*(dum=ans[i-1])-fft[i]*ans[i])/no2;
			ans[i]=(fft[i]*dum+fft[i-1]*ans[i])/no2;
		} else if (isign == -1) {
			if ((mag2=SQR(ans[i-1])+SQR(ans[i])) == 0.0)
				nrerror("Deconvolving at response zero in convlv");
			ans[i-1]=(fft[i-1]*(dum=ans[i-1])+fft[i]*ans[i])/mag2/no2;
			ans[i]=(fft[i]*dum-fft[i-1]*ans[i])/mag2/no2;
		} else nrerror("No meaning for isign in convlv");
	}
	ans[2]=ans[n+1];
	realft(ans,n,-1);
	free_vector(fft,1,n<<1);
}


/* 
  correl()  -- correlation of two real data sets

  N.R.C  p. 433
*/



void correl(double data1[], double data2[], unsigned long n, double ans[])
{
	void realft(double data[], unsigned long n, int isign);
	void twofft(double data1[], double data2[], double fft1[], double fft2[],
		unsigned long n);
	unsigned long no2,i;
	double dum,*fft;

	fft=vector(1,n<<1);
	twofft(data1,data2,fft,ans,n);
	no2=n>>1;
	for (i=2;i<=n+2;i+=2) {
		ans[i-1]=(fft[i-1]*(dum=ans[i-1])+fft[i]*ans[i])/no2;
		ans[i]=(fft[i]*dum-fft[i-1]*ans[i])/no2;
	}
	ans[2]=ans[n+1];
	realft(ans,n,-1);
	free_vector(fft,1,n<<1);
}


/* 
  spctrm()  -- power spectrum using fourier transforms
  
  modified to take array rather than data stream from
  N.R.C  p. 446 
*/

#define WINDOW(j,a,b) (1.0-myfabs((((j)-1)-(a))*(b)))       /* Bartlett */

void spctrm(double data[], double p[], int m, int k)
{
  // assume overlap = 1
	void four1(double data[], unsigned long nn, int isign);
	int mm,m44,m43,m4,kk,joffn,joff,j2,j,c=0;
	double w,facp,facm,*w1,*w2,sumw=0.0,den=0.0;

	mm=m+m;
	m43=(m4=mm+mm)+3;
	m44=m43+1;
	w1=vector(1,m4);
	w2=vector(1,m);
	facm=m;
	facp=1.0/m;
	for (j=1;j<=mm;j++) sumw += SQR(WINDOW(j,facm,facp));
	for (j=1;j<=m;j++) p[j]=0.0;
	for (j=1;j<=m;j++) w2[j] = data[c++];
	for (kk=1;kk<=k;kk++) {
		for (joff = -1;joff<=0;joff++) {
			for (j=1;j<=m;j++) w1[joff+j+j]=w2[j];
			for (j=1;j<=m;j++) w2[j] = data[c++];
			joffn=joff+mm;
			for (j=1;j<=m;j++) w1[joffn+j+j]=w2[j];
		}
		for (j=1;j<=mm;j++) {
			j2=j+j;
			w=WINDOW(j,facm,facp);
			w1[j2] *= w;
			w1[j2-1] *= w;
		}
		four1(w1,mm,1);
		p[1] += (SQR(w1[1])+SQR(w1[2]));
		for (j=2;j<=m;j++) {
			j2=j+j;
			p[j] += (SQR(w1[j2])+SQR(w1[j2-1])
				+SQR(w1[m44-j2])+SQR(w1[m43-j2]));
		}
		den += sumw;
	}
	den *= m4;
	for (j=1;j<=m;j++) p[j] /= den;
	free_vector(w2,1,m);
	free_vector(w1,1,m4);
}
#undef WINDOW



