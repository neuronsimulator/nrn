#include <../../nrnconf.h>

// based on aiex2.cpp but after some experimentation it ended up mostly
// similar to aiex1.cpp and under rtai will use the ai_on_demand_

//
//  aiex2.cpp --
//
//      Hardware timed acquisition
//

#include <stdio.h>
#include <nrndaq.h>

#ifndef ___tMSeries_h___
    #include "tMSeries.h"
#endif

#ifndef ___ai_h___
    #include "ai.h"
#endif 

#ifndef ___common_h___
    #include "common.h"
#endif

#ifndef ___scale_h___
    #include "scale.h"
#endif

static tScalingCoefficients* scale;

//  read eeprom for calibration information
void NrnDAQ:: eeprom_read() {  
    const u32 kEepromSize = 1024;
    //u8 eepromMemory[kEepromSize];
    if (!eepromMemory) {
        eepromMemory = new u8[kEepromSize];
        eepromReadMSeries (bus, eepromMemory, kEepromSize);   
    }
}

void NrnDAQ::ai_reset() {
    // ---- AI Reset ----
    //

    configureTimebase (board);
    pllReset (board);
    analogTriggerReset (board);
    
    aiReset (board);
    aiPersonalize (board,tMSeries::tAI_Output_Control::kAI_CONVERT_Output_SelectActive_High);
    aiClearFifo (board);
    
    // ADC reset only applies to 625x boards
    // adcReset(board);

    // ---- End of AI Reset ----
}

void NrnDAQ::ai_setup(u32 nchan, double dt) {
	if (nchan != numberOfAIChannels) {
		if (numberOfAIChannels) {
			delete [] ai_pvalue;
			ai_pvalue = 0;
			delete [] scale;
			scale = 0;
		}
		numberOfAIChannels = nchan;
		if (nchan) {
			ai_pvalue = new double*[nchan];
			scale = new tScalingCoefficients[nchan];
			u32 i;
			for (i=0; i < nchan; ++i) { ai_pvalue[i] = 0; }
		}
	}
	tstep = dt;
	samplePeriodDivisor = u32(20000.*dt); // timebase*step => 20 MHz * dt(ms)
printf("ai_setup step=%g samplePeriodDivisor=%d\n", tstep, samplePeriodDivisor);
	setup_ai_task();
}

void NrnDAQ::setup_ai_task() {
    // ---- Start AI task ----
    
    aiDisarm (board);
    
	
    aiClearConfigurationMemory (board);
    
    for (u32 i = 0; i < numberOfAIChannels; i++)
    {        
        aiConfigureChannel (board, 
                            i,  // channel number
                            0,  // gain -- check ai.h for allowed values
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_PolarityBipolar,
                            tMSeries::tAI_Config_FIFO_Data::kAI_Config_Channel_TypeDifferential, 
                            (i == numberOfAIChannels-1)?kTrue:kFalse); // last channel?
    }

    aiSetFifoRequestMode (board);    
    
    aiEnvironmentalize (board);
    
    aiHardwareGating (board);
    
    aiTrigger (board,
               tMSeries::tAI_Trigger_Select::kAI_START1_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START1_PolarityRising_Edge,
               tMSeries::tAI_Trigger_Select::kAI_START2_SelectPulse,
               tMSeries::tAI_Trigger_Select::kAI_START2_PolarityRising_Edge);
    
    aiSampleStop (board, 
                  (numberOfAIChannels > 1)?kTrue:kFalse); // multi channel?
    
    aiNumberOfSamples (board,   
                       1, // posttrigger samples
                       0,               // pretrigger samples
                       kTrue);     // continuous?
    
    if (ai_on_demand_) {
	    aiSampleStart (board, 
                   0, 
                   0, 
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectPulse,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    }else{
	    aiSampleStart (board, 
                   samplePeriodDivisor, 
                   3, 
                   tMSeries::tAI_START_STOP_Select::kAI_START_SelectSI_TC,
                   tMSeries::tAI_START_STOP_Select::kAI_START_PolarityRising_Edge);
    }
    
    aiConvert (board, 
               280,     // convert period divisor
               3,       // convert delay divisor 
               kFalse); // external sample clock?
    
    aiClearFifo (board);
}

void NrnDAQ::ai_start() {
    setup_ai_task();
    dio_toggle_on_ai_convert();
    aiArm (board, kTrue);
    aiStart (board);
}
  
void NrnDAQ::ai_record(u32 ichan, double* pvar, double scl, double offset) {
	assert(ichan < numberOfAIChannels);
	ai_pvalue[ichan] = pvar;
	aiGetScalingCoefficients (eepromMemory, 0, 0, ichan, &scale[ichan]);
	scale[ichan].c[0] += offset*scale[ichan].c[1];
	scale[ichan].c[1] *= scl;
}

void NrnDAQ::ai_scanstart() {
    if (ai_on_demand_) {
        aiStartOnDemand (board);
    }
}

void NrnDAQ::ai_read() {
    // ---- Read FIFO ----
    
    i32 value;
    f32 scaled;
    
    u32 n = 0;  

    if (ai_on_demand_) {
        while ( board->Joint_Status_2.readAI_Scan_In_Progress_St())
        {
            // waiting for scan to complete
        }
    }

    while (n < numberOfAIChannels)
    { 
        if(!board->AI_Status_1.readAI_FIFO_Empty_St())
        {
            value = board->AI_FIFO_Data.readRegister ();
	    if (ai_pvalue[n]) {
	        aiPolynomialScaler (&value, &scaled, &scale[n]);
		*(ai_pvalue[n]) = scaled;
	    }
            //printf ("%e,\n", scaled);
            n++;
        }
    }
}

void NrnDAQ::ai_stop() {
    aiDisarm (board);
} 
