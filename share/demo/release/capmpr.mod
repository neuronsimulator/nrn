:  capump.mod plus a "reservoir" used to initialize cai to desired concentrations

UNITS {
	(mM) = (milli/liter)
	(mA) = (milliamp)
	(um) = (micron)
	(mol) = (1)
	PI = (pi) (1)
	FARADAY = (faraday) (coulomb)
}

NEURON {
	SUFFIX capmpr
	USEION ca READ cao, cai WRITE cai, ica
	GLOBAL k1, k2, k3, k4
	GLOBAL car, tau
}

STATE {
	pump	(mol/cm2)
	pumpca	(mol/cm2)
	cai	(mM)
}

PARAMETER {
	car = 5e-5 (mM) : ca in reservoir, used to initialize cai to desired concentrations
	tau = 1e9 (ms) : rate of equilibration between cai and car

	k1 = 5e8	(/mM-s)
	k2 = .25e6	(/s)
	k3 = .5e3	(/s)
	k4 = 5e0	(/mM-s)

	pumpdens = 3e-14 (mol/cm2)
}

CONSTANT {
	volo = 1 (liter)
}

ASSIGNED {
	diam	(um)
	cao	(mM)

	ica (mA/cm2)
	ipump (mA/cm2)
	ipump_last (mA/cm2)
	voli	(um3)
	area1	(um2)
	c1	(1+8 um5/ms)
	c2	(1-10 um2/ms)
	c3	(1-10 um2/ms)
	c4	(1+8 um5/ms)
}

BREAKPOINT {
	SOLVE pmp METHOD sparse
	ipump_last = ipump
	ica = ipump
}

KINETIC pmp {
	COMPARTMENT voli {cai}
	COMPARTMENT (1e10)*area1 {pump pumpca}
	COMPARTMENT volo*(1e15) {cao car}

	~ car <-> cai		(1(um3)/tau,1(um3)/tau)
	~ cai + pump <-> pumpca		(c1,c2)
	~ pumpca     <-> pump + cao	(c3,c4)

	: note that forward flux here is the outward flux
	ipump = (1e-4)*2*FARADAY*(f_flux - b_flux)/area1

        : ipump_last vs ipump needed because of STEADYSTATE calculation
        ~ cai << (-(ica - ipump_last)*area1/(2*FARADAY)*(1e4))

	CONSERVE pump + pumpca = (1e10)*area1*pumpdens
}

INITIAL {
	:cylindrical coordinates; actually vol and area1/unit length
	voli = PI*(diam/2)^2 * 1(um)
	area1 = 2*PI*(diam/2) * 1(um)
	c1 = (1e7)*area1 * k1
	c2 = (1e7)*area1 * k2
	c3 = (1e7)*area1 * k3
	c4 = (1e7)*area1 * k4

	SOLVE pmp STEADYSTATE sparse
}
