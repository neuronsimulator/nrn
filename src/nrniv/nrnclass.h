#include "occlass.h"
#if EXTERNS
	,Shape_reg()
	,PlotShape_reg()
	,PPShape_reg()
	,RangeVarPlot_reg()
	,SectionBrowser_reg()
	,MechanismStandard_reg()
	,MechanismType_reg()
	,NetCon_reg()
	,LinearMechanism_reg()
	,KSChan_reg()
	,Impedance_reg()
	,SaveState_reg()
	,FInitializeHandler_reg()
	,StateTransitionEvent_reg()
	,nrnpython_reg()
#if USEDASPK
	,Daspk_reg()
#endif
#if USECVODE
	,Cvode_reg()
	,TQueue_reg()
	,SingleChan_reg()
#endif
#if USEDSP
	,DSP_reg()
#endif
#if USEBBS
	,ParallelContext_reg()
#endif
#if NRN_REALTIME
	, NrnRealTime_reg()
#endif
#if NRN_DAQ
	, NrnDAQ_reg()
#endif

#else //EXTERNS

	,Shape_reg
	,PlotShape_reg
	,PPShape_reg
	,RangeVarPlot_reg
	,SectionBrowser_reg
	,MechanismStandard_reg
	,MechanismType_reg
	,NetCon_reg
	,LinearMechanism_reg
	,KSChan_reg
	,Impedance_reg
	,SaveState_reg
	,FInitializeHandler_reg
	,StateTransitionEvent_reg
	,nrnpython_reg
#if USEDASPK
	,Daspk_reg
#endif
#if USECVODE
	,Cvode_reg
	,TQueue_reg
	,SingleChan_reg
#endif
#if USEDSP
	,DSP_reg
#endif
#if USEBBS
	,ParallelContext_reg
#endif
#if NRN_REALTIME
	, NrnRealTime_reg
#endif
#if NRN_DAQ
	, NrnDAQ_reg
#endif
#endif // !EXTERNS
