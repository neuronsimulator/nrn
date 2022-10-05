
TITLE Calcium ion accumulation with longitudinal and radial diffusion

COMMENT
PROCEDURE factors_cadifus() sets up the scale factors 
needed to model radial diffusion.
These scale factors do not have to be recomputed
when diam or DFree is changed.
The amount of calcium in an annulus is ca[i]*diam^2*vol[i] 
with ca[0] being the 2nd order correct concentration at the exact edge
and ca[NANN-1] being the concentration at the exact center.
Buffer concentration and rates are based on Yamada et al. 1989
model of bullfrog sympathetic ganglion cell.
ENDCOMMENT

NEURON {
	SUFFIX cadifus
	USEION ca READ cai, ica WRITE cai
	GLOBAL vol, TotalBuffer
	RANGE cai0
	THREADSAFE
}

DEFINE NANN  4

UNITS {
	(molar) =	(1/liter)
	(mM) =	(millimolar)
	(um) =	(micron)
	(mA) =	(milliamp)
	FARADAY =	(faraday)	(10000 coulomb)
	PI = (pi)	(1)
}

PARAMETER {
	DCa = 0.6			(um2/ms)
	: to change rate of buffering without disturbing equilibrium
	: multiply the following two by the same factor
	k1buf	= 100			(/mM-ms)
	k2buf	= 0.1			(/ms)
	TotalBuffer = 0.003	(mM)
	cai0 = 50e-6 (mM)	: Requires explicit use in INITIAL block
}

ASSIGNED {
	diam		(um)
	ica		(mA/cm2)
	cai		(mM)
	vol[NANN]	(1)	: gets extra um2 when multiplied by diam^2
	Kd		(/mM)
	B0		(mM)
}

STATE {
	ca[NANN]		(mM) <1e-6>	: ca[0] is equivalent to cai
	CaBuffer[NANN]	(mM)
	Buffer[NANN]	(mM)
}

BREAKPOINT {
	SOLVE state METHOD sparse
}

LOCAL factors_done

INITIAL {
	MUTEXLOCK
	if (factors_done == 0) {
		factors_done = 1
		factors()
	}
	MUTEXUNLOCK

	cai = cai0
	Kd = k1buf/k2buf
	B0 = TotalBuffer/(1 + Kd*cai)

	FROM i=0 TO NANN-1 {
		ca[i] = cai
		Buffer[i] = B0
		CaBuffer[i] = TotalBuffer - B0
	}
}

COMMENT
factors() sets up factors needed for radial diffusion 
modeled by NANN concentric compartments.
The outermost shell is half as thick as the other shells 
so the concentration is spatially second order correct 
at the surface of the cell.
The radius of the cylindrical core 
equals the thickness of the outermost shell.
The intervening NANN-2 shells each have thickness = r/(NANN-1)
(NANN must be >= 2).

ca[0] is at the edge of the cell, 
ca[NANN-1] is at the center of the cell, 
and ca[i] for 0 < i < NANN-1 is 
midway through the thickness of each annulus.
ENDCOMMENT

LOCAL frat[NANN]

PROCEDURE factors() {
	LOCAL r, dr2
	r = 1/2			:starts at edge (half diam)
	dr2 = r/(NANN-1)/2	:half thickness of annulus
	vol[0] = 0
	frat[0] = 2*r
	FROM i=0 TO NANN-2 {
		vol[i] = vol[i] + PI*(r-dr2/2)*2*dr2	:interior half
		r = r - dr2
		frat[i+1] = 2*PI*r/(2*dr2)	:exterior edge of annulus
					: divided by distance between centers
		r = r - dr2
		vol[i+1] = PI*(r+dr2/2)*2*dr2	:outer half of annulus
	}
}

LOCAL dsq, dsqvol	: can't define local variable in KINETIC block 
			: or use in COMPARTMENT

KINETIC state {
	COMPARTMENT i, diam*diam*vol[i] {ca CaBuffer Buffer}
	LONGITUDINAL_DIFFUSION i, DCa*diam*diam*vol[i] {ca}
	~ ca[0] << (-ica*PI*diam/(2*FARADAY))
	FROM i=0 TO NANN-2 {
		~ ca[i] <-> ca[i+1] (DCa*frat[i+1], DCa*frat[i+1])
	}
	dsq = diam*diam
	FROM i=0 TO NANN-1 {
		dsqvol = dsq*vol[i]
		~ ca[i] + Buffer[i] <-> CaBuffer[i] (k1buf*dsqvol, k2buf*dsqvol)
	}
	cai = ca[0]
}

