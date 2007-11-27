TITLE Calcium ion accumulation and diffusion with pump
: The internal coordinate system is set up in PROCEDURE coord_cadifus()
: and must be executed before computing the concentrations.
: The scale factors set up in this procedure do not have to be recomputed
: when diam or DFree are changed.
: The amount of calcium in an annulus is ca[i]*diam^2*vol[i] with
: ca[0] being the second order correct concentration at the exact edge
: and ca[NANN-1] being the concentration at the exact center

: Free internal calcium is in fast equilibrium with a non-saturable buffer.
: The ratio of bound to free is beta.

NEURON {
	SUFFIX cadifpmp
	USEION ca READ cao, ica, cai WRITE cai, ica
	RANGE ica_pmp, last_ica_pmp
	GLOBAL vol, pump0
}

DEFINE NANN  4

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
	celsius		(degC)
	DFree = .6	(um2/ms)
	diam		(um)
	cao		(mM)
	beta = 50

        k1 = 5e8        (/mM-s) :optional mm formulation
        k2 = .25e6      (/s)
        k3 = .5e3       (/s)
        k4 = 5e0        (/mM-s)
	pump0 = 3e-14 (mol/cm2)  : set to 0 in hoc if this pump not wanted
}

ASSIGNED {
	ica		(mA/cm2)
	cai		(mM)
	vol[NANN]	(1)	: gets extra cm2 when multiplied by diam^2
        ica_pmp (mA/cm2)
        last_ica_pmp (mA/cm2)
        area1    (um2)
        c1      (1+8 um5/ms)
        c2      (1-10 um2/ms)
        c3      (1-10 um2/ms)
        c4      (1+8 um5/ms)

}

CONSTANT {
	volo = 1 (liter)
}

STATE {
	ca[NANN]	(mM) : ca[0] is equivalent to cai
        pump    (mol/cm2)
        pumpca  (mol/cm2)
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
	last_ica_pmp = 0
}

BREAKPOINT {
	SOLVE state METHOD sparse
	last_ica_pmp = ica_pmp
	ica = ica_pmp
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
	COMPARTMENT i, (1+beta)*diam*diam*vol[i]*1(um) {ca}
	COMPARTMENT (1e10)*area1 {pump pumpca}
	COMPARTMENT volo*(1e15) {cao}

	: all currents except pump
	~ ca[0] << (-(ica-last_ica_pmp)*PI*diam*1(um)*(1e4)*frat[0]/(2*FARADAY))
	:diffusion
	FROM i=0 TO NANN-2 {
		~ ca[i] <-> ca[i+1] (DFree*frat[i+1]*1(um), DFree*frat[i+1]*1(um))
	}
	:pump
	~ ca[0] + pump <-> pumpca	(c1, c2)
	~ pumpca <-> pump + cao		(c3, c4)
	ica_pmp = (1e-4)*2*FARADAY*(f_flux - b_flux)/area1

	cai = ca[0]
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
The combination of voltage independent current and calcium
accumulation is more difficult because care must be taken not to count
the pump current twice in the computation of the change ca[0].  Hence
the usage of last_ica_pmp to subtract the pump portion of the total
calcium current in ica so that its effect can be calculated implicitly
via the reaction "pumpca <-> pump + cao".  This artifice makes the
pumping much more stable than the assumption of constant pump current
during the step.  Otherwise, ca[0] is prone to become negative and that
crashes the simulation (especially the automatic computation of eca). 
Calcium currents that are inward are generally safe to compute in
separate models. 

ENDCOMMENT
