NEURON{ SUFFIX nothing}

: in hoc, call with strpass(strdefout,stringin)

VERBATIM
extern char *gargstr(), **hoc_pgargstr();
ENDVERBATIM

PROCEDURE strpass() {
VERBATIM
{	
	char *in;
	char **out;
	in = gargstr(2);
	out = hoc_pgargstr(1);
	hoc_assign_str(out, in);	
}
ENDVERBATIM
}

COMMENT
//test with
strdef a
a = "hello"
strpass(a, "goodbye")
print a
ENDCOMMENT
