TITLE Calcium ion accumulation and diffusion with pump
: The internal coordinate system is set up in PROCEDURE coord_cadifus()
: and must be executed before computing the concentrations.
: The scale factors set up in this procedure do not have to be recomputed
: when diam or DFree are changed.
: The amount of calcium in an annulus is ca[i]*diam^2*vol[i] with
: ca[0] being the second order correct concentration at the exact edge
: and ca[NANN-1] being the concentration at the exact center

? interface
NEURON {
	SUFFIX cadifpmp
	USEION ca READ cao, ica WRITE cai, ica
	RANGE ica_pmp, ica_pmp_last
	GLOBAL vol, pump0
}

DEFINE NANN  10

UNITS {
	(mV)	= (millivolt)
	(molar) = (1/liter)
	(mM)	= (millimolar)
	(um)	= (micron)
	(mA)	= (milliamp)
	(mol)	= (1)
	FARADAY = (faraday)	 (coulomb)
	PI	= (pi)		(1)
	R 	= (k-mole)	(joule/degC)
}

PARAMETER {
	DFree = .6	(um2/ms) <0,1e9>
	beta = 50		<0, 1e9>

        k1 = 5e8        (/mM-s) <0, 1e10>:optional mm formulation
        k2 = .25e6      (/s)	<0, 1e10>
        k3 = .5e3       (/s)	<0, 1e10>
        k4 = 5e0        (/mM-s)	<0, 1e10>
	pump0 = 3e-14 (mol/cm2) <0, 1e9> : set to 0 in hoc if this pump not wanted
}

ASSIGNED {
	celsius		(degC)
	diam		(um)
	v		(millivolt)
	cao		(mM)
	cai		(mM)
	ica		(mA/cm2)
	vol[NANN]	(1)	: gets extra cm2 when multiplied by diam^2
        ica_pmp (mA/cm2)
        area1    (um2)
        c1      (1+8 um5/ms)
        c2      (1-10 um2/ms)
        c3      (1-10 um2/ms)
        c4      (1+8 um5/ms)
	ica_pmp_last (mA/cm2)
}

CONSTANT {
	volo = 1 (liter)
}

STATE {
	ca[NANN]	(mM) <1e-6> : ca[0] is equivalent to cai
        pump    (mol/cm2)	<1e-15>
        pumpca  (mol/cm2)	<1e-15>
}

INITIAL {LOCAL total
	parms()
	FROM i=0 TO NANN-1 {
		ca[i] = cai
	}
	pumpca = cai*pump*c1/c2
	total = pumpca + pump
	if (total > 1e-9) {
		pump = pump*(pump/total)
		pumpca = pumpca*(pump/total)
	}
	ica_pmp = 0
	ica_pmp_last = 0
}

BREAKPOINT {
	SOLVE state METHOD sparse
	ica_pmp_last = ica_pmp
	ica = ica_pmp
:	printf("Breakpoint t=%g v=%g cai=%g ica=%g\n", t, v, cai, ica)
}

LOCAL frat[NANN] 	: gets extra cm when multiplied by diam

PROCEDURE coord() {
	LOCAL r, dr2
	: cylindrical coordinate system  with constant annuli thickness to
	: center of cell. Note however that the first annulus is half thickness
	: so that the concentration is second order correct spatially at
	: the membrane or exact edge of the cell.
	: note ca[0] is at edge of cell
	:      ca[NANN-1] is at center of cell
	r = 1/2					:starts at edge (half diam)
	dr2 = r/(NANN-1)/2			:half thickness of annulus
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

KINETIC state {
:	printf("Solve begin t=%g v=%g cai=%g ica_pmp=%g\n", t, v, cai, ica_pmp)
	COMPARTMENT i, (1+beta)*diam*diam*vol[i]*1(um) {ca}
	COMPARTMENT (1e10)*area1 {pump pumpca}
	COMPARTMENT volo*(1e15) {cao}
? kinetics
	~ pumpca <-> pump + cao		(c3, c4)
	ica_pmp = (1e-4)*2*FARADAY*(f_flux - b_flux)/area1
	: all currents except pump
	~ ca[0] << (-(ica-ica_pmp_last)*PI*diam*1(um)*(1e4)*frat[0]/(2*FARADAY))
	:diffusion
	FROM i=0 TO NANN-2 {
		~ ca[i] <-> ca[i+1] (DFree*frat[i+1]*1(um), DFree*frat[i+1]*1(um))
	}
	:pump
	~ ca[0] + pump <-> pumpca	(c1, c2)
	cai = ca[0] : this assignment statement is used specially by cvode
:	printf("Solve end cai=%g ica=%g ica_pmp=%g ica_pmp_last=%g\n",
:		 cai, ica, ica_pmp,ica_pmp_last)
}
	
PROCEDURE parms() {
	coord()
	area1 = 2*PI*(diam/2) * 1(um)
        c1 = (1e7)*area1 * k1
        c2 = (1e7)*area1 * k2
        c3 = (1e7)*area1 * k3
        c4 = (1e7)*area1 * k4  
}

FUNCTION ss() (mM) {
	SOLVE state STEADYSTATE sparse
	ss = cai
}

COMMENT
At this time, conductances (and channel states and currents are
calculated at the midpoint of a dt interval.  Membrane potential and
concentrations are calculated at the edges of a dt interval.  With
secondorder=2 everything turns out to be second order correct.
ENDCOMMENT
