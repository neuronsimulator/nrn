NEURON {
	USEION na WRITE ina
	RANGE inabar
}

PARAMETER {inabar (milliamp/cm2)}
ASSIGNED {ina (milliamp/cm2)}
BREAKPOINT { ina=inabar}
