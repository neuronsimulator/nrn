#include <../../nrnconf.h>
// derived from aoex4.cpp but in practice ends up as aoex2.cpp using the
// ON_DEMAND mode.

//
//  aoex4.cpp --
//
//      multi-channel hardware timed generation
//

#include <stdio.h>
#include <nrndaq.h>

#ifndef ___tMSeries_h___
    #include "tMSeries.h"
#endif

#ifndef ___ao_h___
    #include "ao.h"
#endif 

#ifndef ___common_h___
    #include "common.h"
#endif

#ifndef ___scale_h___
    #include "scale.h"
#endif

#define ON_DEMAND 1

static tScalingCoefficients* scale;

static tBoolean _continuous = kFalse; 
static tBoolean fifoRetransmit = kFalse; 
static u32 updatePeriodDivisor = 20000000;
static u32 _numberOfSamples = 1; 
    
void NrnDAQ::ao_reset() {
    // ---- AO Reset ----
    // 
    
// done in ai_reset
//    configureTimebase (board);
//    pllReset (board);
//    analogTriggerReset (board);
    
    aoReset (board);
    aoPersonalize (board);
    aoResetWaveformChannels (board);
    aoClearFifo (board);
    
    // unground AO reference
    board->AO_Calibration.writeAO_RefGround (kFalse);
        
    // ---- End of AO Reset ----
}

static void aoFifoModeEmpty(tMSeries* board) {
    board->Joint_Reset.setAO_Configuration_Start (1);
    board->Joint_Reset.flush ();

    board->AO_Mode_2.setAO_FIFO_Retransmit_Enable (fifoRetransmit);
    board->AO_Mode_2.setAO_FIFO_Mode (tMSeries::tAO_Mode_2::kAO_FIFO_ModeEmpty);
    board->AO_Mode_2.flush ();

    board->AO_Personal.setAO_FIFO_Enable (1);
    board->AO_Personal.flush ();

    board->Joint_Reset.setAO_Configuration_End (1);
    board->Joint_Reset.flush ();

    return;
}

void NrnDAQ::ao_setup(u32 nchan) {
    // ---- Write to FIFO ----
    
	if (nchan != numberOfAOChannels) {
		if (numberOfAOChannels) {
			delete [] ao_pvalue;
			ao_pvalue = 0;
			delete [] scale;
			scale = 0;
		}
		numberOfAOChannels = nchan;
		if (nchan) {
			ao_pvalue = new double*[nchan];
			scale = new tScalingCoefficients[nchan];
			u32 i;
			for (i=0; i < nchan; ++i) {
				ao_pvalue[i] = 0;
			}
		}
	}
    
    i32 value; 
    
    // ---- Start AO Task ---
    
	u32 i;
	for (i = 0; i < nchan; ++i) {
#if ON_DEMAND
		aoConfigureDAC (board, 
        	    i, 
                    0xF,
                    tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                    tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);
#else
		aoConfigureDAC (board, 
        	    i, 
                    i,  //waveform order
                    tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                    tMSeries::tAO_Config_Bank::kAO_Update_ModeTimed);
#endif
	}
    aoChannelSelect (board, numberOfAOChannels);

#if !ON_DEMAND
    aoTrigger (board, 
//               tMSeries::tAO_Trigger_Select::kAO_START1_SelectPulse,
//               tMSeries::tAO_Trigger_Select::kAO_START1_PolarityRising_Edge);
               tMSeries::tAO_Trigger_Select::kAO_START1_SelectAI_START_1,
               tMSeries::tAO_Trigger_Select::kAO_START1_PolarityRising_Edge);
    aoCount (board, _numberOfSamples, numberOfAOChannels, _continuous);
    aoUpdate (board, 
//              tMSeries::tAO_Mode_1::kAO_UPDATE_Source_SelectUI_TC, 
              tMSeries::tAO_Mode_1::kAO_UPDATE_Source_SelectPFI2, 
              tMSeries::tAO_Mode_1::kAO_UPDATE_Source_PolarityRising_Edge, 
//              updatePeriodDivisor);
              samplePeriodDivisor);
//    aoFifoMode (board, fifoRetransmit);
	aoFifoModeEmpty(board);
    aoStop (board);
#endif
}

void NrnDAQ::ao_start() {
#if !ON_DEMAND
//    ao_reset();
//    ao_setup(1);
//board->PFI_Output_Select_1.setPFI0_Output_Select(
//tMSeries::tPFI_Output_Select_1::kPFI0_Output_SelectAI_Convert);
    aoArm (board);
    aoStart (board);
#endif
}    

void NrnDAQ::ao_play(u32 ichan, double* pvar, double scl, double offset) {
	assert(ichan < numberOfAOChannels);
	ao_pvalue[ichan] = pvar;
	aoGetScalingCoefficients (eepromMemory, 0, 0, ichan, &scale[ichan]);    
	scale[ichan].c[0] += offset*scale[ichan].c[1];
	scale[ichan].c[1] *= scl;
}

void NrnDAQ::ao_write() {
	f32 fval;
	i32 value;
	u32 n = 0;
	while (n < numberOfAOChannels) {
		if (ao_pvalue[n]) {
			fval = *ao_pvalue[n];
		        aoLinearScaler (&value, &fval, &scale[n]);
//printf("%d %g\n", value, fval);
		}else{
			value = 0;
		}
#if ON_DEMAND
		board->DAC_Direct_Data[n].writeRegister (value);
#else
        	board->AO_FIFO_Data.writeRegister (value); 
#endif
		++n;
	}
}

void NrnDAQ::ao_stop() {
#if !ON_DEMAND
    // ---- Stop ---- 
    
    aoDisarm (board);    
    
    // reset the dacs
    aoConfigureDAC (board, 
                     0, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);
    aoConfigureDAC (board, 
                     1, 
                     0xF, 
                     tMSeries::tAO_Config_Bank::kAO_DAC_PolarityBipolar,
                     tMSeries::tAO_Config_Bank::kAO_Update_ModeImmediate);
    
    // cleanup
//    delete board;
//    bus->destroyAddressSpace(bar1);

    return; 
#endif
} 
