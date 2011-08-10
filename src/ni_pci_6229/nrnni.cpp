#include <../../nrnconf.h>

#include <stdio.h>
#include <InterViews/resource.h>
#include <classreg.h>
#include <nrnoc2iv.h>
#include <nrnrt.h>
#include <nrndaq.h>

NrnDAQ* nrndaq_;

extern "C" {
extern double t;
void nrn_daq_scanstart();
void nrn_daq_ai();
void nrn_daq_ao();
}

/*
Control loop. Hardware-Timed Single Point Acquisition.
*/

void NrnDAQ::nrndaq_init() {
	if (bus) { return; }
	// needs the devicePCI_ID which can be found from
	// nilsdev --verbose
	// to determine the PCI Bus and PCI Device values. From those look
	// at cat /proc/pci and from the Bus - device section the
	// devicePCI_ID is the 8 digit number after PCI device.
	int devicePCI_ID = 0x109370aa;
//printf("trying devicePCI_ID = 0x%x\n", devicePCI_ID);
	bus = acquireBoard(devicePCI_ID);

	if (bus == NULL) {
		hoc_execerror("Could not access PCI device.", 0);
		return;
	}
	// Initialize Mite Chip.
	initMite();

	// read eeprom for calibration information
	eeprom_read();

	// create register map
	bar1 = bus->createAddressSpace(kPCI_BAR1);
	board = new tMSeries(bar1);

	ai_reset();
//	ao_reset();
}

void NrnDAQ::nrndaq_final() {
	if (bus) {
		delete board;
		bus->destroyAddressSpace(bar1);
		releaseBoard(bus);
		bus = nil;
	}
}

// based on from niddk/100a2/examples/main.cpp
void NrnDAQ::initMite() {
	tAddressSpace bar0;
	u32 physicalBar1;

	//Skip MITE initialization for PCMCIA boards
	//(which do not have a MITE DMA controller)
	if(!bus->get(kIsPciPxiBus,0)) return;

	bar0 = bus->createAddressSpace(kPCI_BAR0);

	//Get the physical address of the DAQ board
	physicalBar1 = bus->get(kBusAddressPhysical,kPCI_BAR1);
	//Tell the MITE to enable BAR1, where the rest of the board's regist
	bar0.write32(0xC0, (physicalBar1 & 0xffffff00L) | 0x80);

	bus->destroyAddressSpace(bar0);
}

static double dio_write(void* v) {
	u8 value = (u8)chkarg(1, 0, 255);
	if (nrndaq_) {
		nrndaq_->dio_write(value);
	}
	return double(value);
}

static double aiset(void* v) {
	if (nrndaq_) {
		u32 nchan = (u32)chkarg(1, 0, 16);
		double tstep = chkarg(2, .025, 10000.);
		nrndaq_->ai_setup(nchan, tstep);
	}
	return 0.;
}

static double ai_ondemand(void* v) {
	tBoolean b = kTrue;
	if (nrndaq_) {
		if (ifarg(1)) {
			b = (chkarg(1, 0, 1) != 0.0) ? kTrue : kFalse;
			nrndaq_->ai_on_demand_ = b;
		}
		b = nrndaq_->ai_on_demand_;
	}
		
	return (b == kTrue) ? 1.0 : 0.0;
}

static double aistart(void* v) {
	if (nrndaq_) {
		nrndaq_->ai_start();
	}
	return 0.;
}
static double airecord(void* v) {
	if (nrndaq_) {
		u32 ichan = (u32)chkarg(1, 0, 16);
		double* pvar = hoc_pgetarg(2);
		double scl = 1., offset = 0.0;
		if (ifarg(3)) { scl = chkarg(3, -1e9, 1e9); }
		if (ifarg(4)) { offset = chkarg(4, -1e9, 1e9); }
		nrndaq_->ai_record(ichan, pvar, scl, offset);
	}
	return 0.;
}
static double airead(void* v) {
	if (nrndaq_) {
		nrndaq_->ai_scanstart();
		nrndaq_->ai_read();
	}
	return 0.;
}
static double aistop(void* v) {
	if (nrndaq_) {
		nrndaq_->ai_stop();
	}
	return 0.;
}
static double finalize(void* v) {
	if (nrndaq_) {
		nrndaq_->nrndaq_final();
		nrndaq_ = nil;
	}
	return 0.;
}
static double aosetup(void* v) {
	if (nrndaq_) {
		u32 nchan = (u32)chkarg(1, 0, 4);
		nrndaq_->ao_setup(nchan);
	}
	return 0.;
}
static double aoplay(void* v) {
	if (nrndaq_) {
		u32 ichan = (u32)chkarg(1, 0, 16);
		double* pvar = hoc_pgetarg(2);
		double scl = 1., offset = 0.0;
		if (ifarg(3)) { scl = chkarg(3, -1e9, 1e9); }
		if (ifarg(4)) { offset = chkarg(4, -1e9, 1e9); }
		nrndaq_->ao_play(ichan, pvar, scl, offset);
	}
	return 0.;
}
		
static double aostart(void* v) {
	if (nrndaq_) {
		nrndaq_->ao_start();
	}
	return 0.;
}
static double aowrite(void* v) {
	if (nrndaq_) {
		nrndaq_->ao_write();
	}
	return 0.;
}
static double aostop(void* v) {
	if (nrndaq_) {
		nrndaq_->ao_stop();
	}
	return 0.;
}


static Member_func members[] = {
	"dio_write", dio_write,
	"ai_setup", aiset,
	"ai_start", aistart,
	"ai_record", airecord,
	"ai_read", airead,
	"ai_stop", aistop,
	"ai_ondemand", ai_ondemand,
	"finalize", finalize,

	"ao_setup", aosetup,
	"ao_play", aoplay,
	"ao_start", aostart,
	"ao_write", aowrite,	
	"ao_stop", aostop,	
	0,0
};

static void* cons(Object*) {
	if (!nrndaq_) {
		nrndaq_ = new NrnDAQ();
		nrndaq_->nrndaq_init();
	}
	return nrndaq_;
}

static void destruct(void* v) {
}

void NrnDAQ_reg() {
	class2oc("NrnDAQ", cons, destruct, members);
}

NrnDAQ::NrnDAQ() {
	bus = 0;
	eepromMemory = 0;
	numberOfAIChannels = 0;
	numberOfAOChannels = 0;
	ai_pvalue = 0;
	ao_pvalue = 0;
	tstep = 1.;
	samplePeriodDivisor = 20000; // (1 ms steps)
	ai_on_demand_ = kTrue;
}

NrnDAQ::~NrnDAQ() {
	if (bus) {
		nrndaq_final();
	}
	if (eepromMemory) {
		delete [] eepromMemory;
	}
	if (ai_pvalue) {
		delete [] ai_pvalue;
	}
}

void NrnDAQ::dio_write(u8 value) {
	if (bus) {
		board->DIO_Direction.writeRegister (0xFF);
		board->Static_Digital_Output.writeRegister(value);
	}
}

void NrnDAQ::dio_toggle_on_ai_convert() {
	board->CDIO_Command.writeCDO_Reset(1);
	 // doesn't matter yet
	board->CDO_Mode.setCDO_FIFO_Mode(tMSeries::tCDO_Mode::kCDO_FIFO_ModeFIFO_Half_Full);

	board->CDO_Mode.setCDO_Polarity(tMSeries::tCDO_Mode::kCDO_PolarityRising);
	board->CDO_Mode.setCDO_Update_Source_Select(tMSeries::tCDO_Mode::kCDO_Update_Source_SelectAI_Convert);
	board->CDO_Mode.setCDO_Retransmit(kTrue);
	board->CDO_Mode.flush();

	board->CDO_Mask_Enable.writeRegister (0x3); // bits 0 and 1
	board->DIO_Direction.writeRegister (0x3); // bits 0 and 1 output

	board->CDO_FIFO_Data.writeRegister(0x02);
	board->CDO_FIFO_Data.writeRegister(0x01);

	board->CDIO_Command.writeCDO_Arm(1);
}

void nrn_daq_scanstart() {
	if (nrndaq_) {
		nrndaq_->ai_scanstart();
	}
}

void nrn_daq_ai() {
	if (nrndaq_) {
		nrndaq_->ai_read();
	}
}

void nrn_daq_ao() {
	if (nrndaq_) {
		nrndaq_->ao_write();
	}
}
