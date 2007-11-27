#include <windows.h>

int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	SetCurrentDirectory("\\nrn\\demo");
	WinExec("\\nrn\\bin\\neuron.exe -dll release/nrnmech.dll demo.hoc",
		SW_SHOW);
	return 0;
}
