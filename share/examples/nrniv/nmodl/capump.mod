UNITS {
	(mM) = (milli/liter)
	(mA) = (milliamp)
	(um) = (micron)
	(mol) = (1)
	PI = (pi) (1)
	FARADAY = (faraday) (coulomb)
}

NEURON {
	SUFFIX capump
	USEION ca READ cao, cai WRITE cai, ica
	GLOBAL k1, k2, k3, k4
}

STATE {
	pump	(mol/cm2)
	pumpca	(mol/cm2)
	cai	(mM)
}

PARAMETER {
	cao		(mM) : 10
	diam		(um)

	k1 = 5e8	(/mM-s)
	k2 = .25e6	(/s)
	k3 = .5e3	(/s)
	k4 = 5e0	(/mM-s)
}

CONSTANT {
	volo = 1 (liter)
}

ASSIGNED {
	ica (mA/cm2)
	ipump (mA/cm2)
	voli	(um3)
	area1	(um2)
	c1	(1+8 um5/ms)
	c2	(1-10 um2/ms)
	c3	(1-10 um2/ms)
	c4	(1+8 um5/ms)
}

BREAKPOINT {
	if (t == 0) {parms()}
	SOLVE pmp METHOD sparse
	ica = ipump
}

KINETIC pmp {

	COMPARTMENT voli {cai}
	COMPARTMENT (1e10)*area1 {pump pumpca}
	COMPARTMENT volo*(1e15) {cao}

	~ cai + pump <-> pumpca		(c1,c2)
	~ pumpca     <-> pump + cao	(c3,c4)

	: note that forward flux here is the outward flux
	ipump = (1e-4)*2*FARADAY*(f_flux - b_flux)/area1
}

INITIAL {
	: since cai is a state it is set to 0 be default
	: thus make sure it is set properly to the external ion value
	VERBATIM
	cai = _ion_cai;
	ENDVERBATIM
}

PROCEDURE parms() { :cylindrical coordinates; actually vol and area1/unit length
	voli = PI*(diam/2)^2 * 1(um)
	area1 = 2*PI*(diam/2) * 1(um)
	c1 = (1e7)*area1 * k1
	c2 = (1e7)*area1 * k2
	c3 = (1e7)*area1 * k3
	c4 = (1e7)*area1 * k4
}

FUNCTION ss() (mM) {	: set states to their steady state values
	SOLVE pmp STEADYSTATE sparse
	ss = cai
	COMMENT
	This is probably confusing. First of all this function can only be
	called from HOC when the proper data has been set up with the
	setdata_capump(x) function. Second, only pump_capump and pumpca_capump
	states are properly set. The calculation of cai cannot be seen by
	hoc since hoc looks at the ionic species value and this function does
	not know that pointer so it does not connect the internal version
	of cai with the location that hoc looks at. Thus we return the
	value of the internal version of cai and this can be used at the
	hoc level to initialize cai if desired.
	ENDCOMMENT
}
