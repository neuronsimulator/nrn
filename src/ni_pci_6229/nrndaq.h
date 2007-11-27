#ifndef nrndaq_h
#define nrndaq_h

#include <InterViews/resource.h>
#include <osiBus.h>
#include <tMSeries.h>

class NrnDAQ : public Resource {
public:
	NrnDAQ();
	virtual ~NrnDAQ();
	void nrndaq_init();
	void nrndaq_final();
	void dio_write(u8);
		
	void ai_setup(u32 nchan, double dt);
	void ai_record(u32 ichan, double* pvar, double scl = 1.0, double offset = 0.0);
	void ai_start();
	void ai_read();
	void ai_stop();
	void ai_scanstart();

	void dio_toggle_on_ai_convert();

	void ao_setup(u32 nchan);
	void ao_play(u32 ichan, double* pvar, double scl = 1.0, double offset = 0.0);
	void ao_start();
	void ao_write();
	void ao_stop();
private:
	void initMite();	
	void eeprom_read();
	void ai_reset();	
	void setup_ai_task();

	void ao_reset();
public:
	tBoolean ai_on_demand_;
private:
	iBus* bus;
	tAddressSpace bar1;
	tMSeries* board;
	u8* eepromMemory;
	u32 numberOfAIChannels;
	double **ai_pvalue;
	u32 numberOfAOChannels;
	double **ao_pvalue;
	double tstep;
	u32 samplePeriodDivisor;
};

extern NrnDAQ* nrndaq_;

#endif
