: simple first-order model of calcium dynamics

DEFINE FOO 1

NEURON {
        SUFFIX cadyn
        USEION ca READ cai,ica WRITE cai 
        RANGE ca 
        GLOBAL depth,cainf,taur
        RANGE var
        RANGE ainf
}

UNITS {
        (molar) = (1/liter)		
        (mM) = (milli/liter)
	(um)	= (micron) 
        (mA) = (milliamp)
	(msM)	= (ms mM)  
        FARADAY    = (faraday) (coul)
}

PARAMETER {
       depth	= .1	(um)		
        taur =  200 (ms)	: rate of calcium removal for stress conditions
	cainf	= 50e-6(mM)	:changed oct2
	cai		(mM)
}

ASSIGNED {
	ica		(mA/cm2)
	drive_channel	(mM/ms)
    var     (mV)
    ainf
}

STATE {
	ca		(mM) 
}

 
BREAKPOINT {
	SOLVE state METHOD euler : this comment is terminated by a \r only, and it should not break the parser}

INCLUDE "var_init.inc"

DERIVATIVE state {
    VERBATIM
    cai = 2 * _ion_cai;
    ENDVERBATIM
	drive_channel =  - (10000) * ica / (2 * FARADAY * depth)
	if (drive_channel <= 0.) { drive_channel = 0.  }   : cannot pump inward 
        ca' = drive_channel/18 + (cainf -ca)/taur*11
	cai = ca

    if (FOO == 0) { }
}

: to test code generation for TABLE statement
FUNCTION test_table_f(br) {
    TABLE FROM 0 TO FOO WITH 1
    test_table_f = 1
}

PROCEDURE test_table_p(br) {
    TABLE ainf FROM 0 TO FOO WITH 1
    ainf = 1
}

: to test \r only as newlinePROCEDURE test_r(){
} : add something that will breaks the parser

INITIAL {
    var_init(var)
    ca = cainf
}
