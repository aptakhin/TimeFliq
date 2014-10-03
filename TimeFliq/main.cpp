 
#include "timefliq.h"
#ifdef _WIN32
#	include "win32.h"
#endif

#ifdef _WIN32
int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prev_inst, LPSTR cmd, int show) {
#else
#	error "Win32 only!"
#endif
	gCtrl.init();
	int result = gCtrl.run();
	gCtrl.destroy();
	return result;
}
