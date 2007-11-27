
#$Project$
#$Log$
#Revision 1.1  2003/02/11 18:36:09  hines
#Initial revision
#
#Revision 1.1.1.1  2003/01/01 17:46:35  hines
#NEURON --  Version 5.4 2002/12/23 from prospero as of 1-1-2003
#
#Revision 1.1.1.1  2002/06/26 16:23:07  hines
#NEURON 5.3 2002/06/04
#
# Revision 1.1.1.1  2001/01/01  20:30:36  hines
# preparation for general sparse matrix
#
# Revision 1.1.1.1  1994/10/12  17:22:17  hines
# NEURON 3.0 distribution
#
# Revision 1.1  91/10/11  11:17:14  hines
# Initial revision
# 
# Revision 3.46  90/01/24  06:37:52  mlh
# emalloc() and ecalloc() are macros which return null if out of space
# and then call execerror.  This ensures that pointers are set to null.
# If more cleanup necessary then use hoc_Emalloc() followed by hoc_malchk()
# 
# Revision 3.27  89/09/29  16:01:07  mlh
# ecalloc needed in some places to initialize allocated space
# 
# Revision 3.11  89/07/20  16:19:49  mlh
# hocdec.h constructed from hoc.h using hoc2dec.sh
# 

sed '
	/redef\.h/d
	/prog_parse_recover/c\
extern Inst *hoc_pc;
	s/xpop/hoc_xpop/
	s/spop/hoc_spop/
	s/lookup/hoc_lookup/
	s/install/hoc_install/
' hoc.h > hocdec.h

