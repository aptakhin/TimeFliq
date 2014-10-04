 
#include "timefliq.h"
#ifdef _WIN32
#	include "win32.h"
#else
#	error "Win32 only!"
#endif

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prev_inst, LPSTR cmd, int show) {
	gCtrl.init(cmd);
	int result = gCtrl.run();
	gCtrl.destroy();
	return result;
}
